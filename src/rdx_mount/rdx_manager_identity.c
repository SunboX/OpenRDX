/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : rdx_manager_identity.c
//
// Description : Checksum-protected adapter identity and hardware profile.
//=======================================================================================

/*! @file
 * @brief Manufacturing-record validation and safe OpenRDX defaults.
 */

#include "rdx_manager_identity.h"

#include "spi.h"
#include "string.h"

#define RDX_MANUFACTURING_RECORD_ADDRESS     0x3E000U
#define RDX_MANUFACTURING_RECORD_LENGTH      0x0100U
#define RDX_MANUFACTURING_SERIAL_OFFSET      0x0008U
#define RDX_MANUFACTURING_VENDOR_OFFSET      0x0012U
#define RDX_MANUFACTURING_PRODUCT_OFFSET     0x001AU
#define RDX_MANUFACTURING_PROFILE_OFFSET     0x00A4U
#define RDX_DEFAULT_HARDWARE_PROFILE         0x0038U

static RDX_DRIVE_IDENTITY_T rdx_drive_identity;
static UINT16_T rdx_hardware_profile;

/* These defaults keep the dock addressable when its 256-byte manufacturing
 * object is unreadable or fails checksum validation. Profile 0x38 selects the
 * normal fan and mechanism path rather than the profile-0x36 fan-disable path. */
static const RDX_DRIVE_IDENTITY_T rdx_default_drive_identity =
{
    { '7', '3', '6', '0', '2', '4', '2', '8', '8', '9' },
    { 'T', 'A', 'N', 'D', 'B', 'E', 'R', 'G' },
    {
        'R', 'D', 'X', ' ', ' ', ' ', ' ', ' ',
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '
    }
};

/** Validate the four-lane checksum used by the 3E000h record. */
static BOOLEAN_T rdx_manufacturing_record_is_valid(const UINT8_T *record)
{
    UINT8_T lanes[4] = { 0x78U, 0x56U, 0x34U, 0x12U };
    UINT32_T index;

    for (index = 4U; index < RDX_MANUFACTURING_RECORD_LENGTH; index++)
    {
        lanes[(index - 4U) & 3U] += record[index];
    }
    return (record[0] == lanes[0]) && (record[1] == lanes[1]) &&
           (record[2] == lanes[2]) && (record[3] == lanes[3]);
}

/** Load adapter identity and profile while preserving safe defaults. */
void rdx_manager_identity_init(void)
{
    static UINT8_T record[RDX_MANUFACTURING_RECORD_LENGTH];

    ti_memcpy(&rdx_drive_identity, &rdx_default_drive_identity,
              sizeof(rdx_drive_identity));
    rdx_hardware_profile = RDX_DEFAULT_HARDWARE_PROFILE;
    if ((SpiOps(OpcodeReadData, RDX_MANUFACTURING_RECORD_ADDRESS, record,
                sizeof(record), 0U) == STATUS_OK) &&
        rdx_manufacturing_record_is_valid(record))
    {
        ti_memcpy(rdx_drive_identity.serial,
                  &record[RDX_MANUFACTURING_SERIAL_OFFSET],
                  sizeof(rdx_drive_identity.serial));
        ti_memcpy(rdx_drive_identity.vendor,
                  &record[RDX_MANUFACTURING_VENDOR_OFFSET],
                  sizeof(rdx_drive_identity.vendor));
        ti_memcpy(rdx_drive_identity.product,
                  &record[RDX_MANUFACTURING_PRODUCT_OFFSET],
                  sizeof(rdx_drive_identity.product));

        /* The raw 256-byte flash layout stores this little-endian profile at
         * offset 0xA4. Offset 0x78 belongs to a separate decoded wire object;
         * using it here would select an unrelated field and could incorrectly
         * enable or disable profile-specific fan/mechanism behavior. */
        rdx_hardware_profile =
            (UINT16_T)record[RDX_MANUFACTURING_PROFILE_OFFSET] |
            ((UINT16_T)record[RDX_MANUFACTURING_PROFILE_OFFSET + 1U] << 8U);
    }
}

/** Return the initialized OpenRDX adapter identity. */
const RDX_DRIVE_IDENTITY_T *rdx_manager_get_drive_identity(void)
{
    return &rdx_drive_identity;
}

/** Return the normalized hardware profile used by runtime consumers. */
UINT16_T rdx_manager_get_hardware_profile(void)
{
    /* Raw values 0, 1, and 0xFFFF are unset sentinels and share effective
     * profile 0x37. Other values pass through, including fan-disabled 0x36
     * and checksum-failure default 0x38. Centralizing normalization keeps fan
     * and mechanism consumers from interpreting sentinel values differently. */
    if ((rdx_hardware_profile == 0U) ||
        (rdx_hardware_profile == 0xFFFFU) ||
        (rdx_hardware_profile == 1U))
    {
        return 0x0037U;
    }
    return rdx_hardware_profile;
}
