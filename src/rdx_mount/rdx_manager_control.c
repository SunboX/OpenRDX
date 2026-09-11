/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : rdx_manager_control.c
//
// Description : RDX Manager vendor mode pages and diagnostic controls.
//=======================================================================================

#include "rdx_manager_protocol.h"

#include "rdx_hardware.h"
#include "rdx_led.h"
#include "reg_io.h"
#include "scsi.h"
#include "spi.h"
#include "string.h"
#include "system.h"
#include "usb_hal.h"
#include "vim_nvic.h"

#define RDX_CONTROL_USB_INTERRUPT_MASK         0x00200000U
#define RDX_STATE_RECORD_ADDRESS              0x3F000U
#define RDX_STATE_RECORD_LENGTH               64U
#define RDX_STATE_LOAD_COUNT_OFFSET            4U
#define RDX_STATE_OPERATION_MODE_OFFSET        13U
#define RDX_STATE_CHECKSUM_SEED_0              0x78U
#define RDX_STATE_CHECKSUM_SEED_1              0x56U
#define RDX_STATE_CHECKSUM_SEED_2              0x34U
#define RDX_STATE_CHECKSUM_SEED_3              0x12U

#define RDX_MODE_SELECT_PF                     0x10U
#define RDX_MODE_SELECT_SP                     0x01U
#define RDX_MODE_PAGE_CODE_MASK                0x3FU
#define RDX_MODE_PAGE_CONTROL_CURRENT          0U
#define RDX_MODE_PAGE_CONTROL_CHANGEABLE       1U
#define RDX_MODE_PAGE_CONTROL_DEFAULT          2U
#define RDX_MODE_PAGE_CONTROL_SAVED            3U
#define RDX_DEFAULT_OPERATION_MODE             1U
#define RDX_DEFAULT_MAX_SATA_SPEED             2U
#define RDX_DRIVE_CONTROL_LOGICAL_UNLOAD        0x08U
#define RDX_DRIVE_CONTROL_AUTO_RELOAD           0x10U
#define RDX_DRIVE_CONTROL_POLICY_MASK           0x18U
#define RDX_DRIVE_CONTROL_REJECTED_MASK         0xE2U
#define RDX_PHYSICAL_EJECT_INHIBIT              0x01U

#define RDX_DIAGNOSTIC_LED_PAGE                0x80U
#define RDX_DIAGNOSTIC_EJECT_PAGE              0x85U
#define RDX_DIAGNOSTIC_UNIT_RESET_FLAGS        0x05U

typedef struct _RDX_CONTROL_STATE_T
{
    UINT32_T drive_load_count;
    UINT8_T operation_mode;
    UINT8_T saved_operation_mode;
    UINT8_T max_sata_speed;
    UINT8_T drive_control;
    BOOLEAN_T physical_eject_inhibited;
} RDX_CONTROL_STATE_T;

static RDX_CONTROL_STATE_T rdx_control;

/** Load a little-endian 32-bit value from the persistent state record. */
static UINT32_T rdx_control_load_le32(const UINT8_T *bytes)
{
    return (UINT32_T)bytes[0] |
           ((UINT32_T)bytes[1] << 8U) |
           ((UINT32_T)bytes[2] << 16U) |
           ((UINT32_T)bytes[3] << 24U);
}

/** Store a 32-bit value in the persistent record's little-endian format. */
static void rdx_control_store_le32(UINT8_T *bytes, UINT32_T value)
{
    bytes[0] = (UINT8_T)value;
    bytes[1] = (UINT8_T)(value >> 8U);
    bytes[2] = (UINT8_T)(value >> 16U);
    bytes[3] = (UINT8_T)(value >> 24U);
}

/** Calculate the four-lane checksum for a 64-byte state record. */
static void rdx_control_calculate_checksum(const UINT8_T *record,
                                           UINT8_T *checksum)
{
    UINT32_T index;

    checksum[0] = RDX_STATE_CHECKSUM_SEED_0;
    checksum[1] = RDX_STATE_CHECKSUM_SEED_1;
    checksum[2] = RDX_STATE_CHECKSUM_SEED_2;
    checksum[3] = RDX_STATE_CHECKSUM_SEED_3;
    for (index = 4U; index < RDX_STATE_RECORD_LENGTH; index++)
    {
        checksum[(index - 4U) & 3U] += record[index];
    }
}

/** Return TRUE when the persistent state record has its exact checksum. */
static BOOLEAN_T rdx_control_record_is_valid(const UINT8_T *record)
{
    UINT8_T checksum[4];

    rdx_control_calculate_checksum(record, checksum);
    return (record[0] == checksum[0]) && (record[1] == checksum[1]) &&
           (record[2] == checksum[2]) && (record[3] == checksum[3]);
}

/** Save the operation mode using the 3F000h state-record layout. */
static STATUS_T rdx_control_save_operation_mode(UINT8_T operation_mode)
{
    UINT8_T record[RDX_STATE_RECORD_LENGTH];
    UINT8_T checksum[4];
    UINT32_T usb_interrupt_mask;
    STATUS_T status = STATUS_ERROR;

    /* USB commands share the SPI flash. Exclude them across the entire
     * read/modify/write transaction, including both write-enable operations. */
    usb_interrupt_mask = READ_REG32(VIM_REQMASKSET0) &
                         RDX_CONTROL_USB_INTERRUPT_MASK;
    WRITE_REG32(VIM_REQMASKCLR0, RDX_CONTROL_USB_INTERRUPT_MASK);
    if (SpiOps(OpcodeReadData, RDX_STATE_RECORD_ADDRESS, record,
               sizeof(record), 0U) != STATUS_OK)
    {
        goto restore_usb;
    }
    if (!rdx_control_record_is_valid(record))
    {
        ti_memset(record, 0, sizeof(record));
    }
    record[RDX_STATE_OPERATION_MODE_OFFSET] = operation_mode & 3U;
    rdx_control_calculate_checksum(record, checksum);
    ti_memcpy(record, checksum, sizeof(checksum));

    /* Rewrite only the 64-byte primary record after erasing its 4-KiB sector.
     * Bootstrap bytes in the rest of the sector are intentionally not copied
     * back, matching the persistent-record update contract. */
    if ((SpiOps(OpcodeWriteEnable, 0U, NULL, 0U, 0U) != STATUS_OK) ||
        (SpiOps(OpcodeSectorErase, RDX_STATE_RECORD_ADDRESS, NULL, 0U, 0U) !=
         STATUS_OK) ||
        (SpiOps(OpcodeWriteEnable, 0U, NULL, 0U, 0U) != STATUS_OK) ||
        (SpiOps(OpcodePageProgram, RDX_STATE_RECORD_ADDRESS, record,
                sizeof(record), 0U) != STATUS_OK))
    {
        goto restore_usb;
    }
    status = STATUS_OK;

restore_usb:
    /* Leave an already masked caller masked; never restore unrelated IRQs. */
    if (usb_interrupt_mask != 0U)
    {
        WRITE_REG32(VIM_REQMASKSET0, usb_interrupt_mask);
    }
    return status;
}

/** Save the accepted-eject count in bytes four through seven. */
static STATUS_T rdx_control_save_drive_load_count(UINT32_T drive_load_count)
{
    UINT8_T record[RDX_STATE_RECORD_LENGTH];
    UINT8_T checksum[4];
    UINT32_T usb_interrupt_mask;
    STATUS_T status = STATUS_ERROR;

    /* The foreground eject path can otherwise be interrupted by a USB flash
     * command between write-enable and erase/program, or during the SPI read. */
    usb_interrupt_mask = READ_REG32(VIM_REQMASKSET0) &
                         RDX_CONTROL_USB_INTERRUPT_MASK;
    WRITE_REG32(VIM_REQMASKCLR0, RDX_CONTROL_USB_INTERRUPT_MASK);
    if (SpiOps(OpcodeReadData, RDX_STATE_RECORD_ADDRESS, record,
               sizeof(record), 0U) != STATUS_OK)
    {
        goto restore_usb;
    }
    if (!rdx_control_record_is_valid(record))
    {
        ti_memset(record, 0, sizeof(record));
    }
    rdx_control_store_le32(&record[RDX_STATE_LOAD_COUNT_OFFSET],
                           drive_load_count);
    rdx_control_calculate_checksum(record, checksum);
    ti_memcpy(record, checksum, sizeof(checksum));

    /* Use the same primary-record rewrite as operation-mode persistence:
     * write-enable, sector erase, write-enable, then one 64-byte page program
     * at 3F000h. Keeping both fields in one checksum domain prevents one
     * update from silently invalidating the other. */
    if ((SpiOps(OpcodeWriteEnable, 0U, NULL, 0U, 0U) != STATUS_OK) ||
        (SpiOps(OpcodeSectorErase, RDX_STATE_RECORD_ADDRESS, NULL, 0U, 0U) !=
         STATUS_OK) ||
        (SpiOps(OpcodeWriteEnable, 0U, NULL, 0U, 0U) != STATUS_OK) ||
        (SpiOps(OpcodePageProgram, RDX_STATE_RECORD_ADDRESS, record,
                sizeof(record), 0U) != STATUS_OK))
    {
        goto restore_usb;
    }
    status = STATUS_OK;

restore_usb:
    /* Both success and failure preserve the caller's USB interrupt state. */
    if (usb_interrupt_mask != 0U)
    {
        WRITE_REG32(VIM_REQMASKSET0, usb_interrupt_mask);
    }
    return status;
}

/** Initialize the Manager-visible control pages from persistent flash state. */
void rdx_manager_control_init(void)
{
    UINT8_T record[RDX_STATE_RECORD_LENGTH];
    UINT8_T saved_mode;

    ti_memset(&rdx_control, 0, sizeof(rdx_control));
    rdx_led_init();
    rdx_control.operation_mode = RDX_DEFAULT_OPERATION_MODE;
    rdx_control.saved_operation_mode = RDX_DEFAULT_OPERATION_MODE;
    rdx_control.max_sata_speed = RDX_DEFAULT_MAX_SATA_SPEED;

    if ((SpiOps(OpcodeReadData, RDX_STATE_RECORD_ADDRESS, record,
                sizeof(record), 0U) == STATUS_OK) &&
        rdx_control_record_is_valid(record))
    {
        rdx_control.drive_load_count = rdx_control_load_le32(
            &record[RDX_STATE_LOAD_COUNT_OFFSET]);
        saved_mode = record[RDX_STATE_OPERATION_MODE_OFFSET] & 3U;
        if (saved_mode != 0U)
        {
            rdx_control.operation_mode = saved_mode;
            rdx_control.saved_operation_mode = saved_mode;
        }
    }
}

/** Advance the two OpenRDX LED controllers from the 100-ms service. */
void rdx_manager_control_tick(void)
{
    rdx_led_tick();
}

/** Persist and publish the operation mode selected by the button menu. */
STATUS_T rdx_manager_set_button_operation_mode(UINT8_T operation_mode)
{
    if ((operation_mode != 1U) && (operation_mode != 2U))
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }

    /* Publish the new mode before updating flash. The active runtime mode must
     * therefore remain visible even if persistence reports a write failure. */
    rdx_control.operation_mode = operation_mode;
    if (rdx_control_save_operation_mode(operation_mode) != STATUS_OK)
    {
        return STATUS_SCSI_INTERNAL_TARGET_FAILURE;
    }
    rdx_control.saved_operation_mode = operation_mode;
    return STATUS_OK;
}

/**
 * @brief Count and persist one accepted eject request.
 *
 * The common eject coordinator increments this value before media teardown or
 * mechanism startup. It therefore counts accepted requests, including ones
 * followed by mechanism failure; it is not a successful-ejection counter.
 * Host LOEJ and physical-button requests each increment it exactly once after
 * passing their request-boundary checks.
 *
 * @return STATUS_OK on success, or a SCSI-layer error status.
 */
STATUS_T rdx_manager_increment_drive_load_count(void)
{
    rdx_control.drive_load_count++;
    if (rdx_control_save_drive_load_count(
            rdx_control.drive_load_count) != STATUS_OK)
    {
        /* Preserve the already-published RAM count when flash persistence
         * fails; callers can report the error without losing the event. */
        return STATUS_SCSI_INTERNAL_TARGET_FAILURE;
    }
    return STATUS_OK;
}

/** Return the checksum-validated persistent drive load count. */
UINT32_T rdx_manager_get_drive_load_count(void)
{
    return rdx_control.drive_load_count;
}

/** Apply page 31h logical-unload policy to one accepted host LOEJ request. */
BOOLEAN_T rdx_manager_apply_host_eject_policy(void)
{
    if ((rdx_control.drive_control &
         RDX_DRIVE_CONTROL_LOGICAL_UNLOAD) == 0U)
    {
        return FALSE;
    }

    /* Logical unload is intentionally best-effort at this boundary.  The
     * policy bit itself consumes LOEJ even if the media is already unavailable
     * or another lifecycle transition makes the state change inapplicable;
     * falling through would turn a configured non-mechanical request into an
     * unexpected motor operation. */
    (void)rdx_hardware_logical_unload();
    if ((rdx_control.drive_control & RDX_DRIVE_CONTROL_AUTO_RELOAD) != 0U)
    {
        (void)rdx_hardware_logical_reload();
        /* Auto-reload is a one-shot combination.  Once unload/reload has been
         * attempted, both bits return to their inactive state and MODE SENSE
         * reports that cleared value. */
        rdx_control.drive_control &=
            (UINT8_T)~RDX_DRIVE_CONTROL_POLICY_MASK;
    }
    return TRUE;
}

/** Clear both page 31h policy bits after physical media teardown. */
void rdx_manager_clear_host_eject_policy(void)
{
    rdx_control.drive_control &= (UINT8_T)~RDX_DRIVE_CONTROL_POLICY_MASK;
}

/** Return the page 33h physical-button gate in positive-enable form. */
BOOLEAN_T rdx_manager_physical_eject_is_enabled(void)
{
    return rdx_control.physical_eject_inhibited ? FALSE : TRUE;
}

/** Build one vendor RDX MODE SENSE page without its parameter header. */
UINT32_T rdx_manager_build_mode_page(UINT8_T *buffer, UINT32_T buffer_size,
                                    UINT8_T page_code, UINT8_T page_control)
{
    UINT32_T length;

    if (page_code == RDX_MODE_PAGE_VENDOR_SHORT)
    {
        length = 4U;
    }
    else if ((page_code == RDX_MODE_PAGE_DRIVE_CONTROL) ||
             (page_code == RDX_MODE_PAGE_OPERATION))
    {
        length = 16U;
    }
    else
    {
        return 0U;
    }
    if ((buffer == NULL) || (buffer_size < length) || (page_control > 3U))
    {
        return 0U;
    }

    ti_memset(buffer, 0, length);
    buffer[0] = page_code;
    buffer[1] = (UINT8_T)(length - 2U);

    if (page_control == RDX_MODE_PAGE_CONTROL_CHANGEABLE)
    {
        if (page_code == RDX_MODE_PAGE_DRIVE_CONTROL)
        {
            buffer[4] = RDX_DRIVE_CONTROL_POLICY_MASK;
        }
        else if (page_code == RDX_MODE_PAGE_VENDOR_SHORT)
        {
            /* Byte three contains the page's only writable bit. */
            buffer[3] = RDX_PHYSICAL_EJECT_INHIBIT;
        }
        else if (page_code == RDX_MODE_PAGE_OPERATION)
        {
            buffer[4] = 0x03U;
            buffer[6] = 0x03U;
        }
    }
    else if (page_code == RDX_MODE_PAGE_DRIVE_CONTROL)
    {
        if ((page_control == RDX_MODE_PAGE_CONTROL_CURRENT) ||
            (page_control == RDX_MODE_PAGE_CONTROL_SAVED))
        {
            buffer[4] = rdx_control.drive_control;
        }
    }
    else if (page_code == RDX_MODE_PAGE_VENDOR_SHORT)
    {
        if ((page_control == RDX_MODE_PAGE_CONTROL_CURRENT) ||
            (page_control == RDX_MODE_PAGE_CONTROL_SAVED))
        {
            /* Current and saved views both expose the active runtime gate.
             * The default view stays zero because initialization enables the
             * physical button; the changeable view above reports bit zero. */
            buffer[3] = rdx_control.physical_eject_inhibited ? 1U : 0U;
        }
    }
    else if (page_control == RDX_MODE_PAGE_CONTROL_DEFAULT)
    {
        buffer[4] = RDX_DEFAULT_OPERATION_MODE;
        buffer[6] = RDX_DEFAULT_MAX_SATA_SPEED;
    }
    else
    {
        buffer[4] = (page_control == RDX_MODE_PAGE_CONTROL_SAVED) ?
                    rdx_control.saved_operation_mode :
                    rdx_control.operation_mode;
        buffer[6] = rdx_control.max_sata_speed;
    }
    return length;
}

/** Apply the writable fields of one RDX vendor mode page. */
static STATUS_T rdx_control_apply_mode_page(const UINT8_T *page,
                                            BOOLEAN_T save_pages)
{
    UINT8_T page_code = page[0] & RDX_MODE_PAGE_CODE_MASK;
    UINT8_T operation_mode;
    UINT8_T max_sata_speed;
    UINT8_T requested_drive_control;

    if (page_code == RDX_MODE_PAGE_DRIVE_CONTROL)
    {
        if (page[1] != 14U)
        {
            return STATUS_SCSI_INVALID_CMD_FIELD;
        }
        requested_drive_control = page[4];
        if ((requested_drive_control & RDX_DRIVE_CONTROL_REJECTED_MASK) != 0U)
        {
            return STATUS_SCSI_INVALID_CMD_FIELD;
        }

        if (((requested_drive_control & RDX_DRIVE_CONTROL_AUTO_RELOAD) != 0U) &&
            ((requested_drive_control &
              RDX_DRIVE_CONTROL_LOGICAL_UNLOAD) == 0U))
        {
            /* Bit 4 without bit 3 is the explicit reload action.  It is valid
             * only while page 31h has previously placed visible media in the
             * logical-unloaded state.  The action consumes bit 4 instead of
             * leaving a stale auto-reload policy behind. */
            if (!rdx_hardware_logical_reload())
            {
                return STATUS_SCSI_INVALID_CMD_FIELD;
            }
            requested_drive_control &=
                (UINT8_T)~RDX_DRIVE_CONTROL_AUTO_RELOAD;
        }

        /* Bits zero and two are accepted reserved inputs but are not stateful
         * controls.  Preserve unrelated runtime fields and update only the two
         * logical-eject policy bits advertised as changeable. */
        rdx_control.drive_control =
            (rdx_control.drive_control &
             (UINT8_T)~RDX_DRIVE_CONTROL_POLICY_MASK) |
            (requested_drive_control & RDX_DRIVE_CONTROL_POLICY_MASK);
        return STATUS_OK;
    }
    if (page_code == RDX_MODE_PAGE_VENDOR_SHORT)
    {
        if ((page[1] != 2U) || (page[2] != 0U) ||
            ((page[3] & (UINT8_T)~RDX_PHYSICAL_EJECT_INHIBIT) != 0U))
        {
            return STATUS_SCSI_INVALID_CMD_FIELD;
        }
        /* Store the normalized bit so subsequent MODE SENSE requests and the
         * physical button path observe exactly the same runtime setting. */
        rdx_control.physical_eject_inhibited =
            ((page[3] & RDX_PHYSICAL_EJECT_INHIBIT) != 0U) ? TRUE : FALSE;
        return STATUS_OK;
    }
    if ((page_code != RDX_MODE_PAGE_OPERATION) || (page[1] != 14U))
    {
        return STATUS_NOT_SUPPORTED;
    }

    operation_mode = page[4] & 7U;
    max_sata_speed = page[6];
    if ((operation_mode > 3U) ||
        ((max_sata_speed != 0U) && (max_sata_speed != 2U) &&
         (max_sata_speed != 3U)))
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }
    if (operation_mode == 0U)
    {
        operation_mode = RDX_DEFAULT_OPERATION_MODE;
    }
    rdx_control.operation_mode = operation_mode;
    if (max_sata_speed != 0U)
    {
        rdx_control.max_sata_speed = max_sata_speed;
    }
    if (save_pages)
    {
        if (rdx_control_save_operation_mode(operation_mode) != STATUS_OK)
        {
            return STATUS_SCSI_INTERNAL_TARGET_FAILURE;
        }
        rdx_control.saved_operation_mode = operation_mode;
    }
    return STATUS_OK;
}

/** Apply a Manager vendor MODE SELECT parameter list. */
STATUS_T rdx_manager_handle_mode_select(const UINT8_T *cdb,
                                        const UINT8_T *parameter_list,
                                        UINT32_T parameter_list_length)
{
    UINT32_T header_length;
    UINT32_T block_descriptor_length;
    UINT32_T page_offset;
    UINT32_T page_length;

    if ((cdb == NULL) || (parameter_list == NULL))
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }
    if ((cdb[1] & RDX_MODE_SELECT_PF) == 0U)
    {
        return STATUS_NOT_SUPPORTED;
    }
    if (cdb[0] == SCSI_MODE_SELECT6)
    {
        if ((cdb[1] & ~(RDX_MODE_SELECT_PF | RDX_MODE_SELECT_SP)) ||
            cdb[2] || cdb[3] || cdb[5])
        {
            return STATUS_SCSI_INVALID_CMD_FIELD;
        }
        header_length = 4U;
        if (parameter_list_length < header_length)
        {
            return STATUS_SCSI_INVALID_CMD_FIELD;
        }
        block_descriptor_length = parameter_list[3];
    }
    else if (cdb[0] == SCSI_MODE_SELECT10)
    {
        if ((cdb[1] & ~(RDX_MODE_SELECT_PF | RDX_MODE_SELECT_SP)) ||
            cdb[2] || cdb[3] || cdb[4] || cdb[5] || cdb[6] || cdb[9])
        {
            return STATUS_SCSI_INVALID_CMD_FIELD;
        }
        header_length = 8U;
        if (parameter_list_length < header_length)
        {
            return STATUS_SCSI_INVALID_CMD_FIELD;
        }
        block_descriptor_length = ((UINT32_T)parameter_list[6] << 8U) |
                                  parameter_list[7];
    }
    else
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }

    page_offset = header_length + block_descriptor_length;
    if ((page_offset + 2U) > parameter_list_length)
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }
    if (((parameter_list[page_offset] & RDX_MODE_PAGE_CODE_MASK) !=
         RDX_MODE_PAGE_DRIVE_CONTROL) &&
        ((parameter_list[page_offset] & RDX_MODE_PAGE_CODE_MASK) !=
         RDX_MODE_PAGE_VENDOR_SHORT) &&
        ((parameter_list[page_offset] & RDX_MODE_PAGE_CODE_MASK) !=
         RDX_MODE_PAGE_OPERATION))
    {
        return STATUS_NOT_SUPPORTED;
    }
    page_length = (UINT32_T)parameter_list[page_offset + 1U] + 2U;
    if ((page_offset + page_length) != parameter_list_length)
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }
    return rdx_control_apply_mode_page(
        &parameter_list[page_offset],
        (cdb[1] & RDX_MODE_SELECT_SP) != 0U);
}

/** Handle an RDX vendor SEND DIAGNOSTIC command. */
STATUS_T rdx_manager_handle_send_diagnostic(
    const UINT8_T *cdb, const UINT8_T *parameter_list,
    UINT32_T parameter_list_length)
{
    if (cdb == NULL)
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }
    if (cdb[2] || cdb[5])
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }
    if ((cdb[1] == RDX_DIAGNOSTIC_UNIT_RESET_FLAGS) &&
        (parameter_list_length == 0U))
    {
        usb_hal_disconnect();
        system_reset();
        return STATUS_OK;
    }
    if ((cdb[1] != 0U) || (parameter_list == NULL))
    {
        return STATUS_NOT_SUPPORTED;
    }
    if ((parameter_list_length == 8U) &&
        (parameter_list[0] == RDX_DIAGNOSTIC_LED_PAGE) &&
        (parameter_list[1] == 0U) && (parameter_list[2] == 0U) &&
        (parameter_list[3] == 4U) && (parameter_list[4] == 0U) &&
        (parameter_list[5] == 0U) && (parameter_list[6] == 0U) &&
        (parameter_list[7] <= 1U))
    {
        rdx_led_set_diagnostic(parameter_list[7] == 0U);
        return STATUS_OK;
    }
    if ((parameter_list_length == 6U) &&
        (parameter_list[0] == RDX_DIAGNOSTIC_EJECT_PAGE) &&
        (parameter_list[1] == 0U) && (parameter_list[2] == 0U) &&
        (parameter_list[3] == 4U) && (parameter_list[4] <= 1U) &&
        (parameter_list[5] == 0U))
    {
        /* Page 85h is an acknowledgement-only control frame.  Its payload
         * does not modify mechanism, visibility, page 31h policy, or the page
         * 33h physical-button gate. */
        return STATUS_OK;
    }
    return STATUS_SCSI_INVALID_CMD_FIELD;
}
