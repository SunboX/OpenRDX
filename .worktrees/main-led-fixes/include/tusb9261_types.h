/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef TUSB9261_TYPES_H
#define TUSB9261_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint32_t (*firmware_callback_t)();

/* Fixed target addresses used by the firmware runtime. */
#define DATAPATH_RAM_OFFSET UINT32_C(0xC0000000)
#define ATA_DEVICES_ADDRESS UINT32_C(0x0800F438)
#define ATA_CALLBACK_QUEUE_ADDRESS UINT32_C(0x0800F4E0)
#define ATA_CALLBACK_DATA_ADDRESS UINT32_C(0x0800F520)

/* Bit-concatenation helpers used by the firmware implementation. */
#define CONCAT11(a, b) ((((uint16_t)(a)) << 8) | (uint8_t)(b))
#define CONCAT12(a, b) ((((uint32_t)(a)) << 16) | (uint16_t)(b))
#define CONCAT13(a, b) ((((uint32_t)(a)) << 24) | ((uint32_t)(b) & 0x00ffffffu))
#define CONCAT14(a, b) ((((uint64_t)(a)) << 32) | (uint32_t)(b))
#define CONCAT21(a, b) ((((uint32_t)(a) & 0xffffu) << 8) | (uint8_t)(b))
#define CONCAT22(a, b) ((((uint32_t)(a) & 0xffffu) << 16) | (uint16_t)(b))
#define CONCAT31(a, b) ((((uint32_t)(a) & 0x00ffffffu) << 8) | (uint8_t)(b))
#define CONCAT44(a, b) ((((uint64_t)(a)) << 32) | (uint32_t)(b))
#define CARRY4(a, b) ((uint32_t)(a) > (UINT32_MAX - (uint32_t)(b)))

#endif
