/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/*! @file
 * @brief Dual-mode admission policy for RDX and generic SATA media.
 */

#include "sata_media.h"

#include "rdx_unlock.h"
#include "reg_io.h"
#include "string.h"

#define SATA_IDENTIFY_SPECIFIC_CONFIGURATION_WORD  2U
#define SATA_PUIS_IDENTIFY_INCOMPLETE          0x37C8U
#define SATA_PUIS_IDENTIFY_COMPLETE            0x738CU
#define SATA_IDENTIFY_SECURITY_STATUS_WORD        128U
#define SATA_SECURITY_ENABLED_BIT               0x0002U
#define SATA_SECURITY_LOCKED_BIT                0x0004U
#define SATA_IDENTIFY_WORD_UNAVAILABLE          0xFFFFU
#define SATA_SET_FEATURES_PUIS_SPIN_UP             0x07U
#define SATA_MEDIA_COMMAND_TIMEOUT_MS              30000
#define SATA_LBA_MODE_BIT                           0x40U
#define SATA_ATA_SMART                              0xB0U
#define SATA_ATA_SMART_ENABLE                       0xD8U
#define SATA_ATA_SMART_LBA_MID                      0x4FU
#define SATA_ATA_SMART_LBA_HIGH                     0xC2U
#define SATA_IDENTIFY_COMMAND_SET_SUPPORT_WORD        82U
#define SATA_IDENTIFY_COMMAND_SET_ACTIVE_WORD         85U
#define SATA_SMART_FEATURE_BIT                    0x0001U

static volatile SATA_MEDIA_KIND_T sata_media_kind[NUM_AHCI_PORTS];
static volatile UINT32_T sata_media_epoch[NUM_AHCI_PORTS];
static BOOLEAN_T sata_media_smart_available[NUM_AHCI_PORTS];
static UINT32_T sata_media_failed_rdx_fingerprint[NUM_AHCI_PORTS];
static BOOLEAN_T sata_media_failed_rdx_attempt[NUM_AHCI_PORTS];

/** Completion class for one initialization-time ATA command. */
typedef enum _SATA_MEDIA_COMMAND_RESULT_T
{
    SATA_MEDIA_COMMAND_SUCCESS = 0,
    SATA_MEDIA_COMMAND_REJECTED,
    SATA_MEDIA_COMMAND_TRANSPORT_FAILURE
} SATA_MEDIA_COMMAND_RESULT_T;

/**
 * @brief Interpret the ATA security-locked state from IDENTIFY word 128.
 *
 * @param device Parsed ATA IDENTIFY information.
 * @return TRUE when a valid IDENTIFY word reports the locked state.
 */
static BOOLEAN_T sata_media_security_is_locked(
    const ATA_DEVICE_INFO_T *device)
{
    UINT16_T security_status = device->wIdentifyDeviceInfo[
        SATA_IDENTIFY_SECURITY_STATUS_WORD];

    if (security_status == SATA_IDENTIFY_WORD_UNAVAILABLE)
    {
        return FALSE;
    }
    return (security_status & SATA_SECURITY_LOCKED_BIT) != 0U;
}

/**
 * @brief Decide whether an ATA disk is currently accessible by security state.
 *
 * Direct exposure requires ATA security to be both disabled and unlocked.
 * Security-enabled media remains ambiguous because an accessible RDX
 * cartridge with damaged root metadata cannot otherwise be distinguished
 * from an ordinary disk without risking its reserved areas.
 *
 * @param device Parsed ATA IDENTIFY information.
 * @return TRUE only when direct generic admission is unambiguous.
 */
static BOOLEAN_T sata_media_generic_security_is_accessible(
    const ATA_DEVICE_INFO_T *device)
{
    UINT16_T security_status = device->wIdentifyDeviceInfo[
        SATA_IDENTIFY_SECURITY_STATUS_WORD];

    if (security_status == SATA_IDENTIFY_WORD_UNAVAILABLE)
    {
        return FALSE;
    }
    return (security_status &
            (SATA_SECURITY_ENABLED_BIT | SATA_SECURITY_LOCKED_BIT)) == 0U;
}

/**
 * @brief Build a stable identity fingerprint for failed access throttling.
 *
 * @param device Parsed ATA IDENTIFY information.
 * @return FNV-1a fingerprint over model, serial, and native capacity.
 */
static UINT32_T sata_media_device_fingerprint(
    const ATA_DEVICE_INFO_T *device)
{
    const UINT8_T *model = (const UINT8_T *)device->wModelNum;
    const UINT8_T *serial = (const UINT8_T *)device->wSerialNum;
    const UINT8_T *capacity = (const UINT8_T *)&device->ddTrueMaxLBA;
    UINT32_T hash = 2166136261U;
    UINT32_T index;

    for (index = 0U; index < sizeof(device->wModelNum); index++)
    {
        hash = (hash ^ model[index]) * 16777619U;
    }
    for (index = 0U; index < sizeof(device->wSerialNum); index++)
    {
        hash = (hash ^ serial[index]) * 16777619U;
    }
    for (index = 0U; index < sizeof(device->ddTrueMaxLBA); index++)
    {
        hash = (hash ^ capacity[index]) * 16777619U;
    }
    return hash;
}

/**
 * @brief Attempt RDX access without repeating a failed password on one disk.
 *
 * @param port_num SATA port number.
 * @param device Parsed ATA IDENTIFY information.
 * @return TRUE only when access and metadata validation both succeed.
 */
static BOOLEAN_T sata_media_try_rdx_access(
    UINT32_T port_num,
    ATA_DEVICE_INFO_T *device)
{
    UINT32_T fingerprint = sata_media_device_fingerprint(device);
    RDX_ACCESS_RESULT_T access_result;

    if (sata_media_failed_rdx_attempt[port_num] &&
        (sata_media_failed_rdx_fingerprint[port_num] == fingerprint))
    {
        return FALSE;
    }
    access_result = rdx_unlock_media(port_num, device);
    if (access_result == RDX_ACCESS_RESULT_READY)
    {
        sata_media_failed_rdx_attempt[port_num] = FALSE;
        return TRUE;
    }
    if (access_result == RDX_ACCESS_RESULT_REJECTED)
    {
        sata_media_failed_rdx_fingerprint[port_num] = fingerprint;
        sata_media_failed_rdx_attempt[port_num] = TRUE;
    }
    return FALSE;
}

/**
 * @brief Execute one non-queued initialization command in AHCI slot zero.
 *
 * @param port_num SATA port number.
 * @param ata_cmd Fully populated ATA command descriptor.
 * @return Completion class separating a completed ATA rejection from a
 *         transport failure that may still own slot zero.
 */
static SATA_MEDIA_COMMAND_RESULT_T sata_media_execute_sync_command(
    UINT32_T port_num,
    ATA_COMMAND_T *ata_cmd)
{
    STATUS_T status;

    status = ahci_build_cmd(port_num, ata_cmd, 0U);
    if (status != STATUS_OK)
    {
        return SATA_MEDIA_COMMAND_TRANSPORT_FAILURE;
    }
    status = ahci_issue_cmd(port_num, 0U, FALSE);
    if (status != STATUS_OK)
    {
        return SATA_MEDIA_COMMAND_TRANSPORT_FAILURE;
    }
    status = ahci_wait_complete(
        PxCI(port_num), 0x01U, 0U, SATA_MEDIA_COMMAND_TIMEOUT_MS);
    if (status != STATUS_OK)
    {
        return SATA_MEDIA_COMMAND_TRANSPORT_FAILURE;
    }

    if ((READ32(PxTFD(port_num)) & PTFD_STS_FAILURE_MASK) != 0U)
    {
        return SATA_MEDIA_COMMAND_REJECTED;
    }
    return SATA_MEDIA_COMMAND_SUCCESS;
}

/**
 * @brief Verify that generic SATA sector zero is directly accessible.
 *
 * @param port_num SATA port number.
 * @param device Parsed ATA IDENTIFY information for that port.
 * @return TRUE when a no-data ATA READ VERIFY of sector zero succeeds.
 */
static BOOLEAN_T sata_media_verify_generic_access(
    UINT32_T port_num,
    const ATA_DEVICE_INFO_T *device)
{
    ATA_COMMAND_T ata_cmd;

    ti_memset(&ata_cmd, 0U, sizeof(ata_cmd));
    ata_cmd.fis.FIS_type = H2D_REGISTER_FIS_TYPE;
    ata_cmd.fis.CRRR_PMP = 0x80U;
    ata_cmd.fis.command = device->bLBA48 ?
        ATA_CMD_READ_VERIFY_SECTORS_EXT : ATA_CMD_READ_VERIFY_SECTORS;
    ata_cmd.fis.device = SATA_LBA_MODE_BIT;
    ata_cmd.fis.sector_cnt = 1U;

    return (sata_media_execute_sync_command(port_num, &ata_cmd) ==
            SATA_MEDIA_COMMAND_SUCCESS) ? TRUE : FALSE;
}

/**
 * @brief Request SMART telemetry support for an admitted SATA disk.
 *
 * The result is advisory because storage access does not depend on SMART.
 *
 * @param port_num SATA port number.
 * @param device Parsed ATA IDENTIFY information for that port.
 */
static BOOLEAN_T sata_media_enable_smart(
    UINT32_T port_num,
    const ATA_DEVICE_INFO_T *device)
{
    ATA_COMMAND_T ata_cmd;
    UINT16_T support;
    UINT16_T active;

    support = device->wIdentifyDeviceInfo[
        SATA_IDENTIFY_COMMAND_SET_SUPPORT_WORD];
    active = device->wIdentifyDeviceInfo[
        SATA_IDENTIFY_COMMAND_SET_ACTIVE_WORD];
    if ((support == SATA_IDENTIFY_WORD_UNAVAILABLE) ||
        ((support & SATA_SMART_FEATURE_BIT) == 0U))
    {
        sata_media_smart_available[port_num] = FALSE;
        return TRUE;
    }
    if ((active != SATA_IDENTIFY_WORD_UNAVAILABLE) &&
        ((active & SATA_SMART_FEATURE_BIT) != 0U))
    {
        sata_media_smart_available[port_num] = TRUE;
        return TRUE;
    }

    ti_memset(&ata_cmd, 0U, sizeof(ata_cmd));
    ata_cmd.fis.FIS_type = H2D_REGISTER_FIS_TYPE;
    ata_cmd.fis.CRRR_PMP = 0x80U;
    ata_cmd.fis.command = SATA_ATA_SMART;
    ata_cmd.fis.features = SATA_ATA_SMART_ENABLE;
    ata_cmd.fis.LBA_mid = SATA_ATA_SMART_LBA_MID;
    ata_cmd.fis.LBA_high = SATA_ATA_SMART_LBA_HIGH;
    switch (sata_media_execute_sync_command(port_num, &ata_cmd))
    {
        case SATA_MEDIA_COMMAND_SUCCESS:
            sata_media_smart_available[port_num] = TRUE;
            return TRUE;

        case SATA_MEDIA_COMMAND_REJECTED:
            sata_media_smart_available[port_num] = FALSE;
            return TRUE;

        default:
            sata_media_smart_available[port_num] = FALSE;
            return FALSE;
    }
}

/**
 * @brief Commit the common policy for an admitted RDX cartridge.
 *
 * RDX LBAs are protected by metadata translation, so ATA DSM passthrough is
 * disabled unless every descriptor is translated by a future implementation.
 *
 * @param port_num SATA port number.
 * @param device Parsed ATA IDENTIFY information for that port.
 */
static BOOLEAN_T sata_media_select_rdx(
    UINT32_T port_num,
    ATA_DEVICE_INFO_T *device)
{
    device->bTRIMSupport = FALSE;
    device->wDataSetMgmtMaxBlocks = 0U;
    if (!sata_media_enable_smart(port_num, device))
    {
        return FALSE;
    }
    sata_media_kind[port_num] = SATA_MEDIA_KIND_RDX;
    return TRUE;
}

/**
 * @brief Admit one directly accessible generic SATA disk.
 *
 * @param port_num SATA port number.
 * @param device Parsed ATA IDENTIFY information for that port.
 * @return TRUE when the complete native disk can be exposed safely.
 */
static BOOLEAN_T sata_media_prepare_generic(
    UINT32_T port_num,
    ATA_DEVICE_INFO_T *device)
{
    if ((port_num >= NUM_AHCI_PORTS) || (device == NULL) ||
        device->bPacketDevice)
    {
        return FALSE;
    }

    if (!sata_media_generic_security_is_accessible(device))
    {
        return FALSE;
    }

    if ((device->ddTrueMaxLBA == 0U) || (device->dTrueSectorSize == 0U) ||
        !sata_media_verify_generic_access(port_num, device))
    {
        return FALSE;
    }

    device->ddMaxLBA = device->ddTrueMaxLBA;
    return sata_media_enable_smart(port_num, device);
}

/** Clear media policy state before inspecting a new SATA link. */
void sata_media_reset(UINT32_T port_num)
{
    if (port_num >= NUM_AHCI_PORTS)
    {
        return;
    }

    sata_media_kind[port_num] = SATA_MEDIA_KIND_NONE;
    sata_media_epoch[port_num]++;
    sata_media_smart_available[port_num] = FALSE;
    rdx_reset_media_context(port_num);
}

/** Forget failed access throttling once the physical SATA link is gone. */
void sata_media_link_disconnected(UINT32_T port_num)
{
    if (port_num >= NUM_AHCI_PORTS)
    {
        return;
    }
    sata_media_failed_rdx_fingerprint[port_num] = 0U;
    sata_media_failed_rdx_attempt[port_num] = FALSE;
}

/** Start a disk that advertises either standardized PUIS signature. */
BOOLEAN_T sata_media_start_after_identify(
    UINT32_T port_num,
    ATA_DEVICE_INFO_T *device)
{
    ATA_COMMAND_T ata_cmd;
    SATA_MEDIA_COMMAND_RESULT_T spin_up_result;
    UINT16_T configuration;

    if ((port_num >= NUM_AHCI_PORTS) || (device == NULL) ||
        device->bPacketDevice)
    {
        return FALSE;
    }

    configuration = device->wIdentifyDeviceInfo[
        SATA_IDENTIFY_SPECIFIC_CONFIGURATION_WORD];
    if ((configuration != SATA_PUIS_IDENTIFY_INCOMPLETE) &&
        (configuration != SATA_PUIS_IDENTIFY_COMPLETE))
    {
        return TRUE;
    }

    ti_memset(&ata_cmd, 0U, sizeof(ata_cmd));
    ata_cmd.fis.FIS_type = H2D_REGISTER_FIS_TYPE;
    ata_cmd.fis.CRRR_PMP = 0x80U;
    ata_cmd.fis.command = ATA_CMD_SET_FEATURES;
    ata_cmd.fis.features = SATA_SET_FEATURES_PUIS_SPIN_UP;

    spin_up_result = sata_media_execute_sync_command(port_num, &ata_cmd);
    if (spin_up_result != SATA_MEDIA_COMMAND_SUCCESS)
    {
        /* Some disks return 738Ch after leaving PUIS and abort a redundant
         * wake command even though the supplied IDENTIFY data is complete.
         * Only a completed ATA rejection is safe to tolerate; timeouts and
         * transport failures can still own command slot zero. */
        return ((configuration == SATA_PUIS_IDENTIFY_COMPLETE) &&
                (spin_up_result == SATA_MEDIA_COMMAND_REJECTED)) ?
               TRUE : FALSE;
    }

    if (configuration == SATA_PUIS_IDENTIFY_INCOMPLETE)
    {
        return (ahci_identify_device(port_num) == STATUS_OK) ? TRUE : FALSE;
    }
    return TRUE;
}

/** Select RDX metadata mapping or direct generic SATA access. */
BOOLEAN_T sata_media_prepare(
    UINT32_T port_num,
    ATA_DEVICE_INFO_T *device)
{
    RDX_MEDIA_INSPECTION_T inspection;

    if (port_num >= NUM_AHCI_PORTS)
    {
        return FALSE;
    }

    sata_media_reset(port_num);
    if ((device == NULL) || device->bPacketDevice)
    {
        return FALSE;
    }
    if ((device->ddTrueMaxLBA == 0U) ||
        (device->dTrueSectorSize == 0U))
    {
        return FALSE;
    }
    device->ddMaxLBA = device->ddTrueMaxLBA;

    /* The validated RDX cartridge reports ATA security locked before each
     * access sequence. A locked generic disk is intentionally indistinguishable
     * here and therefore remains unavailable if RDX authentication fails. */
    if (sata_media_security_is_locked(device))
    {
        if (!sata_media_try_rdx_access(port_num, device))
        {
            return FALSE;
        }
        return sata_media_select_rdx(port_num, device);
    }

    /* An already accessible RDX cartridge is recognized from its checksummed
     * mirrored metadata before direct-LBA admission is considered. */
    inspection = rdx_inspect_accessible_media(port_num, device);
    if (inspection == RDX_MEDIA_INSPECTION_READY)
    {
        /* Checksummed metadata plus a valid extent proves that this already
         * accessible cartridge needs no additional access transition. */
        return sata_media_select_rdx(port_num, device);
    }
    if (inspection == RDX_MEDIA_INSPECTION_UNREADABLE)
    {
        /* Some RDX controllers report a clear conventional security word
         * while still requiring their access transition. Make one bounded
         * attempt for unreadable non-packet media, then require the complete
         * metadata walk. A completed rejection is fingerprint-throttled. */
        if (!sata_media_try_rdx_access(port_num, device))
        {
            return FALSE;
        }
        return sata_media_select_rdx(port_num, device);
    }
    if (inspection != RDX_MEDIA_INSPECTION_NOT_RECOGNIZED)
    {
        /* I/O failures and recognized-but-invalid layouts must never fall
         * through to direct-LBA access. */
        return FALSE;
    }

    if (!sata_media_generic_security_is_accessible(device))
    {
        return FALSE;
    }

    if (!sata_media_prepare_generic(port_num, device))
    {
        return FALSE;
    }
    sata_media_kind[port_num] = SATA_MEDIA_KIND_GENERIC;
    return TRUE;
}

/** Return the selected per-port media layout. */
SATA_MEDIA_KIND_T sata_media_get_kind(UINT32_T port_num)
{
    if (port_num >= NUM_AHCI_PORTS)
    {
        return SATA_MEDIA_KIND_NONE;
    }
    return sata_media_kind[port_num];
}

/** Return the current per-port media discovery epoch. */
UINT32_T sata_media_get_epoch(UINT32_T port_num)
{
    if (port_num >= NUM_AHCI_PORTS)
    {
        return 0U;
    }
    return sata_media_epoch[port_num];
}

/** Return whether the selected media can service runtime SMART reads. */
BOOLEAN_T sata_media_smart_is_available(UINT32_T port_num)
{
    if (port_num >= NUM_AHCI_PORTS)
    {
        return FALSE;
    }
    return sata_media_smart_available[port_num];
}

/** Apply the selected media layout to one host-visible LBA. */
BOOLEAN_T sata_media_translate_lba(
    UINT32_T port_num,
    UINT64_T logical_lba,
    UINT64_T *physical_lba)
{
    if ((port_num >= NUM_AHCI_PORTS) || (physical_lba == NULL))
    {
        return FALSE;
    }

    if (sata_media_kind[port_num] == SATA_MEDIA_KIND_RDX)
    {
        *physical_lba = rdx_translate_media_lba(port_num, logical_lba);
        return TRUE;
    }
    if (sata_media_kind[port_num] == SATA_MEDIA_KIND_GENERIC)
    {
        *physical_lba = logical_lba;
        return TRUE;
    }

    *physical_lba = 0U;
    return FALSE;
}
