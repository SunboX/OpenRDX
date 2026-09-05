/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef TUSB9261_RDX_HARDWARE_H
#define TUSB9261_RDX_HARDWARE_H

/*! @file
 * @brief Interface between protocol services and the RDX mechanism runtime.
 */

#include "tusb9260_types.h"

/** Initialize the RDX mechanism and thermal runtime. */
void rdx_hardware_init(void);

/**
 * @brief Start boot-time mechanism handling after SATA discovery.
 *
 * Initialization first places every output in its safe state. This separate
 * foreground step may start timed motor motion, so callers must invoke it only
 * after all synchronous startup work has completed.
 */
void rdx_hardware_start(void);

/**
 * @brief Service debounced inputs, eject, temperature, fan, and LED state.
 *
 * This routine runs in the foreground because ADC and ATA operations may
 * wait for hardware. It must be called once per main-loop pass.
 */
void rdx_hardware_service(void);

/**
 * @brief Invalidate media-specific hardware state before ATA discovery.
 *
 * AHCI calls this immediately after making the port not-ready so write
 * protection remains fail-closed throughout every reinitialization.
 *
 * @param[in] port_num ATA port entering initialization.
 */
void rdx_hardware_media_reinitializing(UINT32_T port_num);

/**
 * @brief Cache the insertion-time lock-slider sample before publication.
 *
 * AHCI calls this synchronously before setting bDeviceInitComplete. Thus no
 * host command can observe ready media with a stale or provisional lock
 * state. Sampling failure leaves the cartridge write protected.
 *
 * @param[in] port_num ATA port about to become ready.
 */
void rdx_hardware_prepare_media_ready(UINT32_T port_num);

/**
 * @brief Report whether the cartridge lock slider prohibits media writes.
 *
 * The slider is sampled once when media initialization completes and
 * caches that result until removal or reinitialization. The cache remains
 * fail-closed when the hardware sample is unavailable.
 *
 * @return TRUE when the cached slider sample prohibits writes, otherwise
 *         FALSE.
 */
BOOLEAN_T rdx_hardware_is_write_protected(void);

/**
 * @brief Queue a cartridge eject request for the mechanism runtime.
 *
 * The request is asynchronous. The runtime waits for outstanding ATA work to
 * quiesce before operating the mechanism.
 */
void rdx_hardware_request_eject(void);

/**
 * @brief Hide an online cartridge without moving or stopping it.
 *
 * This changes only the logical-media lifecycle used by SCSI command routing.
 * ATA readiness, cartridge-presence state, LEDs, fan control, the mechanism,
 * and the persistent load count remain unchanged.
 *
 * @return TRUE when an online cartridge entered logical-unloaded state;
 *         FALSE when that transition was not applicable.
 */
BOOLEAN_T rdx_hardware_logical_unload(void);

/**
 * @brief Return a logically unloaded cartridge to command visibility.
 *
 * Reload is valid only for a cartridge hidden by
 * rdx_hardware_logical_unload(). It clears that logical gate without running
 * ATA initialization or changing physical-device state.
 *
 * @return TRUE when logical-unloaded state was cleared; FALSE when no logical
 *         unload was active.
 */
BOOLEAN_T rdx_hardware_logical_reload(void);

/**
 * @brief Report whether SCSI access is hidden by logical unload.
 *
 * @return TRUE while logical unload hides an otherwise present cartridge.
 */
BOOLEAN_T rdx_hardware_is_logically_unloaded(void);

/**
 * @brief Record cartridge I/O activity from the periodic interrupt service.
 *
 * The foreground service converts this low-latency flag into the normal
 * six-phase cartridge/activity LED sequence.
 */
void rdx_hardware_note_activity(void);

/**
 * @brief Return the last valid SMART cartridge temperature.
 *
 * @return Temperature in degrees Celsius, or 0xFF until no reading is
 *         available.
 */
UINT8_T rdx_hardware_get_temperature_celsius(void);

/**
 * @brief Report whether the asynchronous mechanism sequence is active.
 *
 * @return TRUE while an eject request is waiting, pulsing, or settling.
 */
BOOLEAN_T rdx_hardware_eject_in_progress(void);

#endif /* TUSB9261_RDX_HARDWARE_H */
