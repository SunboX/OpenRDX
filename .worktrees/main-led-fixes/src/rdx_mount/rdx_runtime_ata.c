/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : rdx_runtime_ata.c
//
// Description : Runtime SMART and safe-eject ATA commands.
//=======================================================================================

/*! @file
 * @brief Runtime polled ATA ownership for AHCI slot zero.
 */

#include "rdx_runtime_ata.h"

#include "reg_io.h"
#include "sata_media.h"
#include "string.h"
#include "vim_nvic.h"

#define RDX_ATA_SMART                       0xB0U
#define RDX_ATA_SMART_READ_DATA             0xD0U
#define RDX_ATA_SMART_LBA_MID               0x4FU
#define RDX_ATA_SMART_LBA_HIGH              0xC2U
#define RDX_SMART_ATTRIBUTE_OFFSET             2U
#define RDX_SMART_ATTRIBUTE_SIZE              12U
#define RDX_SMART_ATTRIBUTE_COUNT             30U
#define RDX_SMART_RAW_VALUE_OFFSET             5U
#define RDX_SMART_TEMPERATURE_ID             0xC2U
#define RDX_SMART_AIRFLOW_TEMP_ID            0xBEU
#define RDX_RUNTIME_DATA_BYTES                512U
#define RDX_RUNTIME_SYNC_TIMEOUT_MS          9000
#define RDX_SATA_RX_ERROR_INTERRUPT_MASK 0x00000020U
#define RDX_AHCI_INTERRUPT_MASK          0x00000040U
#define RDX_RUNTIME_CONTROLLER_INTERRUPT_MASK \
    (RDX_SATA_RX_ERROR_INTERRUPT_MASK | RDX_AHCI_INTERRUPT_MASK)

/*
 * Temperature polling and eject preparation use synchronous AHCI slot zero.
 * A command is built as non-NCQ, issued, awaited until PxCI bit zero clears,
 * and accepted only when both PxTFD.ERR and PxTFD.DF remain clear.
 *
 * These calls run after the normal AHCI ISR is live.  Therefore the runtime
 * owner saves/masks PxIE and consumes only its command-generated PxIS bits.
 * ATA failure can set TFES along with its completion FIS; restoring PxIE with
 * TFES latched would make ahci_port_intr_handler() misclassify the already
 * consumed failure as PORT_FATAL_ERROR_INTR.  Connection-change and transport
 * fatal bits are deliberately left for the ordinary ISR.
 */
#define RDX_RUNTIME_COMMAND_ERROR_STATUS TASK_FILE_ERROR_STATUS
#define RDX_PIO_COMPLETION_STATUS \
    (PIO_SETUP_FIS_INTR | D2H_REGISTER_FIS_INTR | \
     RDX_RUNTIME_COMMAND_ERROR_STATUS)
#define RDX_NONDATA_COMPLETION_STATUS \
    (D2H_REGISTER_FIS_INTR | RDX_RUNTIME_COMMAND_ERROR_STATUS)
#define RDX_RUNTIME_OWNED_INTERRUPT_STATUS \
    (PIO_SETUP_FIS_INTR | D2H_REGISTER_FIS_INTR | \
     RDX_RUNTIME_COMMAND_ERROR_STATUS)

/** Media identity and interrupt state owned by one runtime command session. */
typedef struct _RDX_RUNTIME_SESSION_T
{
    UINT32_T saved_pxie;
    UINT32_T media_epoch;
    SATA_MEDIA_KIND_T media_kind;
} RDX_RUNTIME_SESSION_T;

/**
 * @brief Test whether a runtime session still refers to admitted media.
 *
 * @param port_num SATA port number.
 * @param session Session token captured before command issue.
 * @return TRUE only while readiness, media kind, and epoch all still match.
 */
static BOOLEAN_T rdx_runtime_session_is_current(
    UINT32_T port_num,
    const RDX_RUNTIME_SESSION_T *session)
{
    return (ata_dev[port_num].bDeviceInitComplete &&
            (session->media_kind != SATA_MEDIA_KIND_NONE) &&
            (sata_media_get_kind(port_num) == session->media_kind) &&
            (sata_media_get_epoch(port_num) == session->media_epoch)) ?
           TRUE : FALSE;
}

/** Mask both controller interrupt sources for a short ownership transition. */
static void rdx_runtime_enter_critical_section(void)
{
    WRITE_REG32(VIM_REQMASKCLR0,
                RDX_RUNTIME_CONTROLLER_INTERRUPT_MASK);
}

/**
 * @brief End a short ownership transition without reviving invalid media.
 *
 * The AHCI interrupt is safe once PxIE has been finalized. Receiver-error
 * recovery remains masked when another handler invalidated the media because
 * foreground discovery owns its later re-enable.
 *
 * @param media_is_current TRUE while the captured media epoch is still valid.
 */
static void rdx_runtime_leave_critical_section(
    BOOLEAN_T media_is_current)
{
    WRITE_REG32(VIM_REQMASKSET0, RDX_AHCI_INTERRUPT_MASK);
    if (media_is_current)
    {
        WRITE_REG32(VIM_REQMASKSET0,
                    RDX_SATA_RX_ERROR_INTERRUPT_MASK);
    }
}

/**
 * @brief Lock a still-current session for a short register transition.
 *
 * @param port_num SATA port number.
 * @param session Captured runtime session.
 * @return TRUE with controller interrupts masked when the session is current;
 *         FALSE after preserving the discovery isolation state.
 */
static BOOLEAN_T rdx_runtime_lock_current_session(
    UINT32_T port_num,
    const RDX_RUNTIME_SESSION_T *session)
{
    rdx_runtime_enter_critical_section();
    if (!rdx_runtime_session_is_current(port_num, session))
    {
        WRITE32(PxIE(port_num), 0U);
        rdx_runtime_leave_critical_section(FALSE);
        return FALSE;
    }
    return TRUE;
}

/** Restore the saved PxIE value and release a locked current session. */
static void rdx_runtime_unlock_current_session(
    UINT32_T port_num,
    const RDX_RUNTIME_SESSION_T *session)
{
    WRITE32(PxIE(port_num), session->saved_pxie);
    rdx_runtime_leave_critical_section(TRUE);
}

/**
 * @brief Execute one polled ATA command in AHCI slot zero.
 *
 * Build slot zero, issue it as non-NCQ, wait only for PxCI bit zero to clear,
 * then inspect the PxTFD failure bits.  This preserves one clear completion
 * owner without adding a second BSY/DRQ wait after command issue.
 *
 * @param port_num SATA port number.
 * @param ata_cmd fully populated ATA command descriptor.
 * @param session Captured runtime media session.
 * @return TRUE only when build, issue, completion, and ATA status succeed.
 */
static BOOLEAN_T rdx_runtime_execute_sync_command(
    UINT32_T port_num,
    ATA_COMMAND_T *ata_cmd,
    const RDX_RUNTIME_SESSION_T *session)
{
    STATUS_T status;
    BOOLEAN_T successful;

    if (!rdx_runtime_lock_current_session(port_num, session))
    {
        return FALSE;
    }
    status = ahci_build_cmd(port_num, ata_cmd, 0U);
    if (status == STATUS_OK)
    {
        status = ahci_issue_cmd(port_num, 0U, FALSE);
    }
    rdx_runtime_leave_critical_section(TRUE);
    if (status == STATUS_OK)
    {
        status = ahci_wait_complete(PxCI(port_num), 0x01U, 0U,
                                    RDX_RUNTIME_SYNC_TIMEOUT_MS);
    }
    if ((status != STATUS_OK) &&
        ((READ32(PxCI(port_num)) & 0x01U) != 0U))
    {
        /* Completion still belongs to this polled command. Keep its PxIE
         * sources masked, retire the slot with COMRESET, and invalidate the
         * epoch so the outer session cannot restore the saved mask. */
        ahci_recover_local_command(port_num, TRUE);
        return FALSE;
    }
    if (!rdx_runtime_lock_current_session(port_num, session))
    {
        return FALSE;
    }
    successful = ((status == STATUS_OK) &&
                  ((READ32(PxTFD(port_num)) &
                    PTFD_STS_FAILURE_MASK) == 0U)) ? TRUE : FALSE;
    rdx_runtime_leave_critical_section(TRUE);
    return successful;
}

/**
 * @brief Reserve an idle initialized port for synchronous runtime commands.
 *
 * The caller masks USB submission. Testing PxCI/PxSACT before and after the
 * command-owned PxIE bits are masked closes the observation window before
 * slot zero is reused. The complete interrupt-enable register is saved instead
 * of synthesizing a default mask, preserving the live per-port policy.
 *
 * @param port_num SATA port number.
 * @param session receives the media epoch and interrupt-enable value.
 * @return TRUE when the port was reserved; FALSE with ownership unchanged.
 */
static BOOLEAN_T rdx_runtime_begin_sync_commands(
    UINT32_T port_num,
    RDX_RUNTIME_SESSION_T *session)
{
    if ((port_num >= NUM_AHCI_PORTS) || (session == NULL) ||
        !ata_dev[port_num].bDeviceInitComplete ||
        ((READ32(PxCI(port_num)) | READ32(PxSACT(port_num))) != 0U))
    {
        return FALSE;
    }
    session->media_kind = sata_media_get_kind(port_num);
    session->media_epoch = sata_media_get_epoch(port_num);
    if (!rdx_runtime_lock_current_session(port_num, session))
    {
        return FALSE;
    }
    session->saved_pxie = READ32(PxIE(port_num));
    WRITE32(PxIE(port_num),
            session->saved_pxie & ~RDX_RUNTIME_OWNED_INTERRUPT_STATUS);
    if (!rdx_runtime_session_is_current(port_num, session) ||
        ((READ32(PxCI(port_num)) | READ32(PxSACT(port_num))) != 0U))
    {
        if (rdx_runtime_session_is_current(port_num, session))
        {
            WRITE32(PxIE(port_num), session->saved_pxie);
            rdx_runtime_leave_critical_section(TRUE);
        }
        else
        {
            WRITE32(PxIE(port_num), 0U);
            rdx_runtime_leave_critical_section(FALSE);
        }
        return FALSE;
    }
    rdx_runtime_leave_critical_section(TRUE);
    return TRUE;
}

/**
 * @brief Restore interrupt ownership only for the same admitted media epoch.
 *
 * @param port_num SATA port number.
 * @param session Captured runtime session.
 * @return TRUE when the saved PxIE value was safely restored.
 */
static BOOLEAN_T rdx_runtime_finish_sync_commands(
    UINT32_T port_num,
    const RDX_RUNTIME_SESSION_T *session)
{
    if (!rdx_runtime_lock_current_session(port_num, session))
    {
        return FALSE;
    }
    rdx_runtime_unlock_current_session(port_num, session);
    return TRUE;
}

/**
 * @brief Read and parse a one-sector ATA SMART temperature table.
 *
 * SMART READ DATA uses command B0h, feature D0h, and mandatory 4Fh/C2h task-
 * file signature.  The parser walks 30 twelve-byte attributes from offset
 * two, preferring ID C2h and retaining BEh only as a fallback; raw temperature
 * is byte five.  A null output, ownership/ATA failure, or missing attribute
 * returns FALSE with the 0xFF unavailable sentinel.
 *
 * PxIS is W1C. PIO-setup, D2H-completion, and TFES are cleared immediately
 * before and after the owned command, then the saved PxIE value is restored
 * only if the same admitted-media epoch remains current. Link-change and
 * transport-fatal status is intentionally not consumed here.
 *
 * @param[in] port_num SATA port number.
 * @param[out] temperature_celsius receives C2h, BEh fallback, or 0xFF.
 * @return TRUE only when a supported temperature attribute was read.
 */
BOOLEAN_T rdx_read_smart_temperature(UINT32_T port_num,
                                     UINT8_T *temperature_celsius)
{
    ATA_COMMAND_T ata_cmd;
    const volatile UINT8_T *attribute;
    UINT32_T attribute_index;
    RDX_RUNTIME_SESSION_T session;
    UINT8_T airflow_temperature = 0xFFU;
    BOOLEAN_T airflow_temperature_found = FALSE, successful;

    if (temperature_celsius == NULL)
    {
        return FALSE;
    }
    *temperature_celsius = 0xFFU;
    if (!rdx_runtime_begin_sync_commands(port_num, &session))
    {
        return FALSE;
    }

    ti_memset(&ata_cmd, 0U, sizeof(ata_cmd));
    ata_cmd.fis.FIS_type = H2D_REGISTER_FIS_TYPE;
    ata_cmd.fis.CRRR_PMP = 0x80U;
    ata_cmd.fis.command = RDX_ATA_SMART;
    ata_cmd.fis.features = RDX_ATA_SMART_READ_DATA;
    ata_cmd.fis.LBA_mid = RDX_ATA_SMART_LBA_MID;
    ata_cmd.fis.LBA_high = RDX_ATA_SMART_LBA_HIGH;
    ata_cmd.fis.sector_cnt = 1U;
    ata_cmd.dDataByteCnt = RDX_RUNTIME_DATA_BYTES;
    WRITE32(PxIS(port_num), RDX_PIO_COMPLETION_STATUS);
    successful = rdx_runtime_execute_sync_command(
        port_num, &ata_cmd, &session);
    WRITE32(PxIS(port_num), RDX_PIO_COMPLETION_STATUS);

    /* Hold the short controller critical section through parsing so a reset
     * cannot replace the media epoch while the shared data is consumed. */
    if (!rdx_runtime_lock_current_session(port_num, &session))
    {
        successful = FALSE;
    }
    else
    {
        if (successful)
        {
            successful = FALSE;
            attribute = datapath_ram->normal_data_buffer +
                        RDX_SMART_ATTRIBUTE_OFFSET;
            for (attribute_index = 0U;
                 attribute_index < RDX_SMART_ATTRIBUTE_COUNT;
                 attribute_index++, attribute += RDX_SMART_ATTRIBUTE_SIZE)
            {
                if (attribute[0] == RDX_SMART_TEMPERATURE_ID)
                {
                    *temperature_celsius =
                        attribute[RDX_SMART_RAW_VALUE_OFFSET];
                    successful = TRUE;
                    break;
                }
                if ((attribute[0] == RDX_SMART_AIRFLOW_TEMP_ID) &&
                    !airflow_temperature_found)
                {
                    airflow_temperature =
                        attribute[RDX_SMART_RAW_VALUE_OFFSET];
                    airflow_temperature_found = TRUE;
                }
            }
            if (!successful && airflow_temperature_found)
            {
                *temperature_celsius = airflow_temperature;
                successful = TRUE;
            }
        }
        rdx_runtime_unlock_current_session(port_num, &session);
    }
    return successful;
}

/**
 * @brief Flush the cartridge and issue STANDBY IMMEDIATE for a STOP request.
 *
 * EAh FLUSH CACHE EXT is selected for LBA48 and E7h FLUSH CACHE otherwise;
 * E0h STANDBY IMMEDIATE is issued only after the flush succeeds.  One saved-
 * PxIE reservation spans both commands so the ISR cannot take ownership
 * between those ordered media-safety operations.  D2H and TFES W1C bits are
 * consumed around each command; asynchronous link/fatal bits remain pending
 * for the normal ISR.
 *
 * @param[in] port_num SATA port number.
 * @return TRUE only when both commands complete without an ATA error.
 */
BOOLEAN_T rdx_prepare_media_stop(UINT32_T port_num)
{
    ATA_COMMAND_T ata_cmd;
    RDX_RUNTIME_SESSION_T session;
    BOOLEAN_T successful;

    if (!rdx_runtime_begin_sync_commands(port_num, &session))
    {
        return FALSE;
    }
    ti_memset(&ata_cmd, 0U, sizeof(ata_cmd));
    ata_cmd.fis.FIS_type = H2D_REGISTER_FIS_TYPE;
    ata_cmd.fis.CRRR_PMP = 0x80U;
    ata_cmd.fis.command = ata_dev[port_num].bLBA48 ?
        ATA_CMD_FLUSH_CACHE_EXT : ATA_CMD_FLUSH_CACHE;

    WRITE32(PxIS(port_num), RDX_NONDATA_COMPLETION_STATUS);
    successful = rdx_runtime_execute_sync_command(
        port_num, &ata_cmd, &session);
    WRITE32(PxIS(port_num), RDX_NONDATA_COMPLETION_STATUS);
    if (successful)
    {
        ata_cmd.fis.command = ATA_CMD_STANDBY_IMMEDIATE;
        WRITE32(PxIS(port_num), RDX_NONDATA_COMPLETION_STATUS);
        successful = rdx_runtime_execute_sync_command(
            port_num, &ata_cmd, &session);
        WRITE32(PxIS(port_num), RDX_NONDATA_COMPLETION_STATUS);
    }
    if (!rdx_runtime_finish_sync_commands(port_num, &session))
    {
        successful = FALSE;
    }
    return successful;
}

/**
 * @brief Issue only STANDBY IMMEDIATE before mechanism motion.
 *
 * Mechanical eject is distinct from the ordinary STOP path: it does not add
 * FLUSH CACHE/EXT. The port is reserved with command-owned PxIE bits masked,
 * stale D2H/TFES W1C status is consumed before issue, completion status is
 * consumed afterward, and the saved PxIE value is restored after ATA failure
 * only while the same admitted-media epoch remains current.
 *
 * @param[in] port_num SATA port number.
 * @return TRUE when STANDBY IMMEDIATE completes without an ATA error.
 */
BOOLEAN_T rdx_prepare_mechanism_eject(UINT32_T port_num)
{
    ATA_COMMAND_T ata_cmd;
    RDX_RUNTIME_SESSION_T session;
    BOOLEAN_T successful;

    if (!rdx_runtime_begin_sync_commands(port_num, &session))
    {
        return FALSE;
    }
    ti_memset(&ata_cmd, 0U, sizeof(ata_cmd));
    ata_cmd.fis.FIS_type = H2D_REGISTER_FIS_TYPE;
    ata_cmd.fis.CRRR_PMP = 0x80U;
    ata_cmd.fis.command = ATA_CMD_STANDBY_IMMEDIATE;

    WRITE32(PxIS(port_num), RDX_NONDATA_COMPLETION_STATUS);
    successful = rdx_runtime_execute_sync_command(
        port_num, &ata_cmd, &session);
    WRITE32(PxIS(port_num), RDX_NONDATA_COMPLETION_STATUS);
    if (!rdx_runtime_finish_sync_commands(port_num, &session))
    {
        successful = FALSE;
    }
    return successful;
}
