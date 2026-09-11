/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : rdx_manager_protocol.h
//
// Description : Tandberg RDX Manager SCSI compatibility and firmware update support.
//=======================================================================================

#ifndef _RDX_MANAGER_PROTOCOL_H_
#define _RDX_MANAGER_PROTOCOL_H_

#include "rdx_manager_identity.h"
#include "tusb9260.h"
#include "tusb9260_types.h"

#define RDX_VPD_MEDIA_ID_PAGE_CODE              0xC0
#define RDX_VPD_MEDIA_IDENTIFY_PAGE_CODE        0xC2
#define RDX_LOG_SENSE_DRIVE_PAGE_CODE           0x20
#define RDX_LOG_SENSE_CARTRIDGE_PAGE_CODE       0x21
#define RDX_LOG_SENSE_TEMPERATURE_PAGE_CODE     0x0D
#define RDX_MODE_PAGE_DRIVE_CONTROL             0x31
#define RDX_MODE_PAGE_VENDOR_SHORT              0x33
#define RDX_MODE_PAGE_OPERATION                 0x34
#define RDX_SECURITY_PROTOCOL_TANDBERG           0x20

/** Reset the in-RAM state of the RDX firmware download receiver. */
void rdx_manager_protocol_init(void);

/** Advance a deferred firmware-activation reset from the 100-ms RTI service. */
void rdx_manager_protocol_tick(void);

/** Initialize the Manager-visible control pages from persistent flash state. */
void rdx_manager_control_init(void);

/** Advance the OpenRDX LED controllers from the 100-ms RTI service. */
void rdx_manager_control_tick(void);

/**
 * Persist and publish one operation mode selected by the eject gesture.
 *
 * Modes one and two are the only actionable menu entries. The new value is
 * published immediately and written to the checksum-protected state record.
 *
 * @param[in] operation_mode operation mode 1 or 2 selected by the gesture.
 * @return STATUS_OK on success, or a SCSI-layer error status.
 */
STATUS_T rdx_manager_set_button_operation_mode(UINT8_T operation_mode);

/**
 * @brief Increment and persist the accepted-eject load counter.
 *
 * Both host LOEJ and physical-button requests reach this common counter after
 * acceptance. The in-memory count is incremented before the checksum-protected
 * record is rewritten and before mechanism success is known.
 *
 * @return STATUS_OK on success, or a SCSI-layer error status.
 */
STATUS_T rdx_manager_increment_drive_load_count(void);

/**
 * @brief Apply the configured host-eject policy.
 *
 * Drive-control page 31h bit 3 changes an accepted host LOEJ request from a
 * physical mechanism operation into a reversible logical unload.  Bit 4 asks
 * that logical unload to reload immediately and resets both policy bits after
 * that one-shot transition.  A set logical-unload bit handles the request even
 * when the current media lifecycle cannot enter the unloaded state; callers
 * must therefore not fall through to the mechanism in that case.
 *
 * @return TRUE when the request is completely handled by the logical policy;
 *         FALSE when the caller must use the physical eject coordinator.
 */
BOOLEAN_T rdx_manager_apply_host_eject_policy(void);

/**
 * @brief Clear page 31h policy after a physical media teardown.
 *
 * Logical unload deliberately retains its policy so a later explicit reload
 * can complete the reversible lifecycle.  Physical removal and mechanism
 * teardown instead reset both bits before another cartridge can be admitted.
 */
void rdx_manager_clear_host_eject_policy(void);

/**
 * @brief Report whether a physical eject-button press may request eject.
 *
 * Vendor-short page 33h bit 0 is the physical-button inhibit.  The setting
 * affects only the button request boundary; host LOEJ and the no-cartridge
 * configuration gesture retain their independent behavior.
 *
 * @return TRUE when physical button eject is enabled, otherwise FALSE.
 */
BOOLEAN_T rdx_manager_physical_eject_is_enabled(void);

/**
 * Build the variable-width RDX cartridge identity VPD page C0h.
 *
 * @param[out] buffer response destination.
 * @param[in] buffer_size response destination capacity.
 * @param[in] lun ATA logical unit whose identity is exposed.
 * @return number of response bytes produced, or zero on error.
 */
UINT32_T rdx_manager_build_media_id_vpd(UINT8_T *buffer, UINT32_T buffer_size,
                                        UINT8_T lun);

/**
 * Build RDX media IDENTIFY DEVICE VPD page C2h.
 *
 * @param[out] buffer response destination.
 * @param[in] buffer_size response destination capacity.
 * @param[in] lun ATA logical unit whose IDENTIFY DEVICE data is exposed.
 * @return number of response bytes produced, or zero on error.
 */
UINT32_T rdx_manager_build_media_identify_vpd(UINT8_T *buffer,
                                              UINT32_T buffer_size,
                                              UINT8_T lun);

/**
 * Build the adapter T10 device-identification VPD page expected by Manager.
 *
 * @param[out] buffer response destination.
 * @param[in] buffer_size response destination capacity.
 * @return number of response bytes produced, or zero on error.
 */
UINT32_T rdx_manager_build_drive_id_vpd(UINT8_T *buffer,
                                        UINT32_T buffer_size);

/**
 * Build one RDX LOG SENSE page used by RDX Manager.
 *
 * @param[out] buffer response destination.
 * @param[in] buffer_size response destination capacity.
 * @param[in] lun ATA logical unit represented by the page.
 * @param[in] page_code requested LOG SENSE page code.
 * @return number of response bytes produced, or zero for an unsupported page.
 */
UINT32_T rdx_manager_build_log_sense(UINT8_T *buffer, UINT32_T buffer_size,
                                     UINT8_T lun, UINT8_T page_code);

/**
 * Build one vendor RDX MODE SENSE page without the mode parameter header.
 *
 * @param[out] buffer response destination.
 * @param[in] buffer_size response destination capacity.
 * @param[in] page_code requested mode page.
 * @param[in] page_control MODE SENSE page-control value.
 * @return number of page bytes produced, or zero for an unsupported page.
 */
UINT32_T rdx_manager_build_mode_page(UINT8_T *buffer, UINT32_T buffer_size,
                                    UINT8_T page_code, UINT8_T page_control);

/**
 * Apply a Manager vendor MODE SELECT parameter list.
 *
 * @param[in] cdb six- or ten-byte MODE SELECT command descriptor block.
 * @param[in] parameter_list complete data-out parameter list.
 * @param[in] parameter_list_length number of received parameter bytes.
 * @return STATUS_NOT_SUPPORTED when the list is not an RDX vendor page,
 *         otherwise the SCSI-layer completion status.
 */
STATUS_T rdx_manager_handle_mode_select(const UINT8_T *cdb,
                                        const UINT8_T *parameter_list,
                                        UINT32_T parameter_list_length);

/**
 * Handle an RDX vendor SEND DIAGNOSTIC command.
 *
 * @param[in] cdb six-byte SEND DIAGNOSTIC command descriptor block.
 * @param[in] parameter_list data-out parameter list, or NULL for no data.
 * @param[in] parameter_list_length number of received parameter bytes.
 * @return STATUS_NOT_SUPPORTED for the standard self-test form, otherwise the
 *         SCSI-layer completion status.
 */
STATUS_T rdx_manager_handle_send_diagnostic(
    const UINT8_T *cdb, const UINT8_T *parameter_list,
    UINT32_T parameter_list_length);

/**
 * @brief Build a bounded read-only LOG SENSE status response.
 * @param buffer Response destination.
 * @param buffer_size Capacity of the destination.
 * @param lun SATA port index.
 * @param page_code Requested LOG SENSE page.
 * @return Written response length, or zero for invalid/unsupported requests.
 */
UINT32_T rdx_manager_build_status_log(UINT8_T *buffer, UINT32_T buffer_size,
                                     UINT8_T lun, UINT8_T page_code);

/** Return the checksum-validated persistent drive load count. */
UINT32_T rdx_manager_get_drive_load_count(void);

/**
 * Build the Tandberg SECURITY PROTOCOL IN response expected by Manager.
 *
 * This firmware has no cartridge-security processor. The returned records
 * explicitly report that subsystem as absent; they are not ATA Trusted-command
 * responses forwarded to the ordinary SATA disk.
 *
 * @param[out] buffer response destination.
 * @param[in] buffer_size response destination capacity.
 * @param[in] protocol_specific SECURITY PROTOCOL SPECIFIC selector.
 * @return number of response bytes produced, or zero for an unsupported selector.
 */
UINT32_T rdx_manager_build_security_protocol_in(
    UINT8_T *buffer, UINT32_T buffer_size, UINT16_T protocol_specific);

/**
 * Receive one RDX Manager WRITE BUFFER firmware-download command.
 *
 * The supported compatibility container and explicitly marked OpenRDX
 * containers are accepted. Download data is streamed into SPI flash while its
 * first vector word is withheld; the vector word is committed only after the
 * complete vendor-container hash or OpenRDX boot-region digest matches. Mode
 * 05h is acknowledged before a short deferred reset so the Manager can receive
 * its final SCSI GOOD status.
 *
 * @param[in] cdb ten-byte WRITE BUFFER command descriptor block.
 * @param[in] host_length data length declared by the USB transport.
 * @param[in] payload received data for mode 04h, or NULL for mode 05h.
 * @return SCSI-layer status for the BOT command.
 */
STATUS_T rdx_manager_handle_write_buffer(const UINT8_T *cdb,
                                         UINT32_T host_length,
                                         const UINT8_T *payload);

#endif /* _RDX_MANAGER_PROTOCOL_H_ */
