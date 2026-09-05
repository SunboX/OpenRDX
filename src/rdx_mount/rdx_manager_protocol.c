/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : rdx_manager_protocol.c
//
// Description : RDX Manager discovery pages and firmware download receiver.
//=======================================================================================

#include "rdx_manager_protocol.h"

#include "ahci.h"
#include "rdx_hardware.h"
#include "rdx_unlock.h"
#include "sata_media.h"
#include "scsi.h"
#include "spi.h"
#include "string.h"
#include "system.h"
#include "usb_hal.h"

#define RDX_MEDIA_ID_MAX_LENGTH             0xBCU
#define RDX_MEDIA_IDENTIFY_LENGTH           0x204U
#define RDX_UPDATE_CONTAINER_LENGTH         0xF29EU
#define RDX_UPDATE_PAYLOAD_OFFSET           0x018CU
#define RDX_UPDATE_PAYLOAD_END              0xF29AU
#define RDX_UPDATE_FLASH_LENGTH             0xF10EU
#define RDX_UPDATE_CUSTOM_AUTH_OFFSET       0x0108U
#define RDX_UPDATE_CUSTOM_AUTH_LENGTH       0x0080U
#define RDX_UPDATE_CUSTOM_MAGIC_LENGTH      8U
#define RDX_UPDATE_CUSTOM_DIGEST_OFFSET     8U
#define RDX_UPDATE_CUSTOM_DIGEST_LENGTH     32U
#define RDX_UPDATE_MAX_CHUNK                 0x1000U
#define RDX_UPDATE_FLASH_SECTOR_SIZE         0x1000U
#define RDX_UPDATE_FLASH_PAGE_SIZE           0x0100U
#define RDX_WRITE_BUFFER_DOWNLOAD_MODE       0x04U
#define RDX_WRITE_BUFFER_ACTIVATE_MODE       0x05U
#define RDX_UPDATE_RESET_DELAY_TICKS          5U
#define RDX_DRIVE_ID_VPD_LENGTH              42U

typedef struct _RDX_SHA256_CONTEXT_T
{
    UINT32_T state[8];
    UINT32_T bit_count_high;
    UINT32_T bit_count_low;
    UINT8_T block[64];
    UINT32_T block_length;
} RDX_SHA256_CONTEXT_T;

typedef struct _RDX_UPDATE_CONTEXT_T
{
    UINT32_T next_container_offset;
    UINT8_T first_vector_word[4];
    BOOLEAN_T started;
    BOOLEAN_T image_valid;
    BOOLEAN_T failed;
    RDX_SHA256_CONTEXT_T hash;
    RDX_SHA256_CONTEXT_T payload_hash;
    UINT8_T custom_auth[RDX_UPDATE_CUSTOM_AUTH_LENGTH];
} RDX_UPDATE_CONTEXT_T;

static RDX_UPDATE_CONTEXT_T rdx_update;
static volatile UINT8_T rdx_update_reset_ticks;

/* SHA-256 allow-list entry for the supported 62,110-byte compatibility image.
 * The digest bytes remain part of the fixed update-wire compatibility contract. */
static const UINT8_T rdx_compatibility_image_sha256[32] =
{
    0x73, 0xD5, 0x28, 0x80, 0x1A, 0xEF, 0xC0, 0x32,
    0xD3, 0xA5, 0x36, 0x37, 0xB0, 0x35, 0xF6, 0x5D,
    0x72, 0x80, 0x91, 0x51, 0xB2, 0xF1, 0x27, 0xC6,
    0xB2, 0x05, 0x0E, 0x9B, 0xAC, 0xC7, 0x6F, 0x3B
};

/* OpenRDX images replace the vendor RSA bytes with this explicit marker and
 * the SHA-256 digest of the exact TI boot region that will be programmed. */
static const UINT8_T rdx_custom_update_magic[RDX_UPDATE_CUSTOM_MAGIC_LENGTH] =
{
    'O', 'P', 'E', 'N', 'R', 'D', 'X', '1'
};

static const UINT32_T rdx_sha256_constants[64] =
{
    0x428A2F98UL, 0x71374491UL, 0xB5C0FBCFUL, 0xE9B5DBA5UL,
    0x3956C25BUL, 0x59F111F1UL, 0x923F82A4UL, 0xAB1C5ED5UL,
    0xD807AA98UL, 0x12835B01UL, 0x243185BEUL, 0x550C7DC3UL,
    0x72BE5D74UL, 0x80DEB1FEUL, 0x9BDC06A7UL, 0xC19BF174UL,
    0xE49B69C1UL, 0xEFBE4786UL, 0x0FC19DC6UL, 0x240CA1CCUL,
    0x2DE92C6FUL, 0x4A7484AAUL, 0x5CB0A9DCUL, 0x76F988DAUL,
    0x983E5152UL, 0xA831C66DUL, 0xB00327C8UL, 0xBF597FC7UL,
    0xC6E00BF3UL, 0xD5A79147UL, 0x06CA6351UL, 0x14292967UL,
    0x27B70A85UL, 0x2E1B2138UL, 0x4D2C6DFCUL, 0x53380D13UL,
    0x650A7354UL, 0x766A0ABBUL, 0x81C2C92EUL, 0x92722C85UL,
    0xA2BFE8A1UL, 0xA81A664BUL, 0xC24B8B70UL, 0xC76C51A3UL,
    0xD192E819UL, 0xD6990624UL, 0xF40E3585UL, 0x106AA070UL,
    0x19A4C116UL, 0x1E376C08UL, 0x2748774CUL, 0x34B0BCB5UL,
    0x391C0CB3UL, 0x4ED8AA4AUL, 0x5B9CCA4FUL, 0x682E6FF3UL,
    0x748F82EEUL, 0x78A5636FUL, 0x84C87814UL, 0x8CC70208UL,
    0x90BEFFFAUL, 0xA4506CEBUL, 0xBEF9A3F7UL, 0xC67178F2UL
};

/** Rotate a 32-bit SHA-256 value right. */
static UINT32_T rdx_rotr32(UINT32_T value, UINT32_T count)
{
    return (value >> count) | (value << (32U - count));
}

/** Process one complete 64-byte SHA-256 block. */
static void rdx_sha256_transform(RDX_SHA256_CONTEXT_T *context,
                                 const UINT8_T *block)
{
    UINT32_T schedule[64];
    UINT32_T a;
    UINT32_T b;
    UINT32_T c;
    UINT32_T d;
    UINT32_T e;
    UINT32_T f;
    UINT32_T g;
    UINT32_T h;
    UINT32_T choice;
    UINT32_T majority;
    UINT32_T sigma0;
    UINT32_T sigma1;
    UINT32_T temp1;
    UINT32_T temp2;
    UINT32_T index;

    for (index = 0; index < 16U; index++)
    {
        schedule[index] = ((UINT32_T)block[index * 4U] << 24) |
                          ((UINT32_T)block[index * 4U + 1U] << 16) |
                          ((UINT32_T)block[index * 4U + 2U] << 8) |
                          (UINT32_T)block[index * 4U + 3U];
    }
    for (index = 16U; index < 64U; index++)
    {
        sigma0 = rdx_rotr32(schedule[index - 15U], 7U) ^
                 rdx_rotr32(schedule[index - 15U], 18U) ^
                 (schedule[index - 15U] >> 3U);
        sigma1 = rdx_rotr32(schedule[index - 2U], 17U) ^
                 rdx_rotr32(schedule[index - 2U], 19U) ^
                 (schedule[index - 2U] >> 10U);
        schedule[index] = schedule[index - 16U] + sigma0 +
                          schedule[index - 7U] + sigma1;
    }

    a = context->state[0];
    b = context->state[1];
    c = context->state[2];
    d = context->state[3];
    e = context->state[4];
    f = context->state[5];
    g = context->state[6];
    h = context->state[7];

    for (index = 0; index < 64U; index++)
    {
        sigma1 = rdx_rotr32(e, 6U) ^ rdx_rotr32(e, 11U) ^
                 rdx_rotr32(e, 25U);
        choice = (e & f) ^ ((~e) & g);
        temp1 = h + sigma1 + choice + rdx_sha256_constants[index] +
                schedule[index];
        sigma0 = rdx_rotr32(a, 2U) ^ rdx_rotr32(a, 13U) ^
                 rdx_rotr32(a, 22U);
        majority = (a & b) ^ (a & c) ^ (b & c);
        temp2 = sigma0 + majority;

        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
    context->state[4] += e;
    context->state[5] += f;
    context->state[6] += g;
    context->state[7] += h;
}

/** Initialize a SHA-256 stream. */
static void rdx_sha256_init(RDX_SHA256_CONTEXT_T *context)
{
    ti_memset(context, 0, sizeof(*context));
    context->state[0] = 0x6A09E667UL;
    context->state[1] = 0xBB67AE85UL;
    context->state[2] = 0x3C6EF372UL;
    context->state[3] = 0xA54FF53AUL;
    context->state[4] = 0x510E527FUL;
    context->state[5] = 0x9B05688CUL;
    context->state[6] = 0x1F83D9ABUL;
    context->state[7] = 0x5BE0CD19UL;
}

/** Add bytes to a SHA-256 stream. */
static void rdx_sha256_update(RDX_SHA256_CONTEXT_T *context,
                              const UINT8_T *data, UINT32_T length)
{
    UINT32_T old_count;
    UINT32_T copy_length;

    old_count = context->bit_count_low;
    context->bit_count_low += length << 3U;
    if (context->bit_count_low < old_count)
    {
        context->bit_count_high++;
    }
    context->bit_count_high += length >> 29U;

    while (length != 0U)
    {
        copy_length = 64U - context->block_length;
        if (copy_length > length)
        {
            copy_length = length;
        }
        ti_memcpy(&context->block[context->block_length], data, copy_length);
        context->block_length += copy_length;
        data += copy_length;
        length -= copy_length;
        if (context->block_length == 64U)
        {
            rdx_sha256_transform(context, context->block);
            context->block_length = 0U;
        }
    }
}

/** Finish a SHA-256 stream and write its 32-byte digest. */
static void rdx_sha256_final(RDX_SHA256_CONTEXT_T *context, UINT8_T *digest)
{
    UINT32_T index;

    context->block[context->block_length++] = 0x80U;
    if (context->block_length > 56U)
    {
        ti_memset(&context->block[context->block_length], 0,
                  64U - context->block_length);
        rdx_sha256_transform(context, context->block);
        context->block_length = 0U;
    }
    ti_memset(&context->block[context->block_length], 0,
              56U - context->block_length);
    context->block[56] = (UINT8_T)(context->bit_count_high >> 24U);
    context->block[57] = (UINT8_T)(context->bit_count_high >> 16U);
    context->block[58] = (UINT8_T)(context->bit_count_high >> 8U);
    context->block[59] = (UINT8_T)context->bit_count_high;
    context->block[60] = (UINT8_T)(context->bit_count_low >> 24U);
    context->block[61] = (UINT8_T)(context->bit_count_low >> 16U);
    context->block[62] = (UINT8_T)(context->bit_count_low >> 8U);
    context->block[63] = (UINT8_T)context->bit_count_low;
    rdx_sha256_transform(context, context->block);

    for (index = 0; index < 8U; index++)
    {
        digest[index * 4U] = (UINT8_T)(context->state[index] >> 24U);
        digest[index * 4U + 1U] = (UINT8_T)(context->state[index] >> 16U);
        digest[index * 4U + 2U] = (UINT8_T)(context->state[index] >> 8U);
        digest[index * 4U + 3U] = (UINT8_T)context->state[index];
    }
}

/** Compare two byte arrays without an early exit. */
static BOOLEAN_T rdx_bytes_equal(const UINT8_T *left, const UINT8_T *right,
                                 UINT32_T length)
{
    UINT8_T difference = 0U;
    UINT32_T index;

    for (index = 0U; index < length; index++)
    {
        difference |= left[index] ^ right[index];
    }
    return difference == 0U;
}

/** Append a NUL-terminated literal to a variable-width C0 VPD payload. */
static BOOLEAN_T rdx_append_literal(UINT8_T *buffer, UINT32_T buffer_size,
                                    UINT32_T *offset, const char *text)
{
    UINT32_T length = 0U;

    while (text[length] != '\0')
    {
        length++;
    }
    if ((*offset + length + 1U) > buffer_size)
    {
        return FALSE;
    }
    ti_memcpy(&buffer[*offset], text, length);
    *offset += length;
    buffer[(*offset)++] = 0U;
    return TRUE;
}

/** Append a fixed ASCII field exactly until its first NUL or width limit. */
static BOOLEAN_T rdx_append_ata_text(UINT8_T *buffer, UINT32_T buffer_size,
                                     UINT32_T *offset, const UINT8_T *text,
                                     UINT32_T text_length,
                                     UINT32_T maximum_length)
{
    UINT32_T length = 0U;

    if (text_length > maximum_length)
    {
        text_length = maximum_length;
    }
    while ((length < text_length) && (text[length] != 0U))
    {
        length++;
    }
    if ((*offset + length + 1U) > buffer_size)
    {
        return FALSE;
    }
    ti_memcpy(&buffer[*offset], text, length);
    *offset += length;
    buffer[(*offset)++] = 0U;
    return TRUE;
}

/** Append an eight-character uppercase hexadecimal field. */
static BOOLEAN_T rdx_append_hex32(UINT8_T *buffer, UINT32_T buffer_size,
                                  UINT32_T *offset, UINT32_T value)
{
    static const UINT8_T digits[16] =
    {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };
    UINT32_T index;

    if ((*offset + 9U) > buffer_size)
    {
        return FALSE;
    }
    for (index = 0U; index < 8U; index++)
    {
        buffer[*offset + index] = digits[(value >> (28U - index * 4U)) & 0x0FU];
    }
    *offset += 8U;
    buffer[(*offset)++] = 0U;
    return TRUE;
}

/** Store one big-endian 16-bit value. */
static void rdx_store_be16(UINT8_T *buffer, UINT16_T value)
{
    buffer[0] = (UINT8_T)(value >> 8U);
    buffer[1] = (UINT8_T)value;
}

/** Store one big-endian 32-bit value. */
static void rdx_store_be32(UINT8_T *buffer, UINT32_T value)
{
    buffer[0] = (UINT8_T)(value >> 24U);
    buffer[1] = (UINT8_T)(value >> 16U);
    buffer[2] = (UINT8_T)(value >> 8U);
    buffer[3] = (UINT8_T)value;
}

/** Append one LOG SENSE integer parameter record. */
static UINT32_T rdx_append_log_parameter(UINT8_T *buffer, UINT32_T offset,
                                         UINT16_T code, UINT32_T value,
                                         UINT8_T value_length)
{
    rdx_store_be16(&buffer[offset], code);
    buffer[offset + 2U] = 0x60U;
    buffer[offset + 3U] = value_length;
    if (value_length == 2U)
    {
        rdx_store_be16(&buffer[offset + 4U], (UINT16_T)value);
    }
    else
    {
        rdx_store_be32(&buffer[offset + 4U], value);
    }
    return offset + 4U + value_length;
}

/** Erase every 4 KiB sector occupied by the bootable RDX image. */
static STATUS_T rdx_erase_boot_image(void)
{
    UINT32_T address;

    for (address = 0U; address < 0x10000U;
         address += RDX_UPDATE_FLASH_SECTOR_SIZE)
    {
        if ((SpiOps(OpcodeWriteEnable, 0U, NULL, 0U, 0U) != STATUS_OK) ||
            (SpiOps(OpcodeSectorErase, address, NULL, 0U, 0U) != STATUS_OK))
        {
            return STATUS_ERROR;
        }
    }
    return STATUS_OK;
}

/** Program arbitrary bytes while respecting the SPI flash page boundary. */
static STATUS_T rdx_program_flash(UINT32_T address, const UINT8_T *data,
                                  UINT32_T length)
{
    UINT32_T page_remaining;
    UINT32_T write_length;

    while (length != 0U)
    {
        page_remaining = RDX_UPDATE_FLASH_PAGE_SIZE -
                         (address & (RDX_UPDATE_FLASH_PAGE_SIZE - 1U));
        write_length = (length < page_remaining) ? length : page_remaining;
        if ((SpiOps(OpcodeWriteEnable, 0U, NULL, 0U, 0U) != STATUS_OK) ||
            (SpiOps(OpcodePageProgram, address, (UINT8_T *)data,
                    write_length, 0U) != STATUS_OK))
        {
            return STATUS_ERROR;
        }
        address += write_length;
        data += write_length;
        length -= write_length;
    }
    return STATUS_OK;
}

/** Program the portion of a container chunk belonging to the boot image. */
static STATUS_T rdx_program_container_payload(UINT32_T container_offset,
                                              const UINT8_T *data,
                                              UINT32_T length)
{
    UINT32_T start = container_offset;
    UINT32_T end = container_offset + length;
    UINT32_T payload_start;
    UINT32_T payload_end;
    UINT32_T flash_address;
    UINT32_T source_offset;

    if ((end <= RDX_UPDATE_PAYLOAD_OFFSET) ||
        (start >= RDX_UPDATE_PAYLOAD_END))
    {
        return STATUS_OK;
    }
    payload_start = (start < RDX_UPDATE_PAYLOAD_OFFSET) ?
                    RDX_UPDATE_PAYLOAD_OFFSET : start;
    payload_end = (end > RDX_UPDATE_PAYLOAD_END) ?
                  RDX_UPDATE_PAYLOAD_END : end;
    flash_address = payload_start - RDX_UPDATE_PAYLOAD_OFFSET;
    source_offset = payload_start - container_offset;

    if (flash_address < 4U)
    {
        UINT32_T withheld = 4U - flash_address;
        if (withheld > (payload_end - payload_start))
        {
            withheld = payload_end - payload_start;
        }
        ti_memcpy(&rdx_update.first_vector_word[flash_address],
                  &data[source_offset], withheld);
        flash_address += withheld;
        source_offset += withheld;
        payload_start += withheld;
    }
    if (payload_start < payload_end)
    {
        return rdx_program_flash(flash_address, &data[source_offset],
                                 payload_end - payload_start);
    }
    return STATUS_OK;
}

/** Capture the OpenRDX authentication block from a streamed container chunk. */
static void rdx_capture_custom_auth(UINT32_T container_offset,
                                    const UINT8_T *data, UINT32_T length)
{
    UINT32_T start = container_offset;
    UINT32_T end = container_offset + length;
    UINT32_T auth_start;
    UINT32_T auth_end;

    if ((end <= RDX_UPDATE_CUSTOM_AUTH_OFFSET) ||
        (start >= (RDX_UPDATE_CUSTOM_AUTH_OFFSET +
                   RDX_UPDATE_CUSTOM_AUTH_LENGTH)))
    {
        return;
    }
    auth_start = (start < RDX_UPDATE_CUSTOM_AUTH_OFFSET) ?
                 RDX_UPDATE_CUSTOM_AUTH_OFFSET : start;
    auth_end = end;
    if (auth_end > (RDX_UPDATE_CUSTOM_AUTH_OFFSET +
                    RDX_UPDATE_CUSTOM_AUTH_LENGTH))
    {
        auth_end = RDX_UPDATE_CUSTOM_AUTH_OFFSET +
                   RDX_UPDATE_CUSTOM_AUTH_LENGTH;
    }
    ti_memcpy(&rdx_update.custom_auth[
                  auth_start - RDX_UPDATE_CUSTOM_AUTH_OFFSET],
              &data[auth_start - container_offset], auth_end - auth_start);
}

/** Hash only the TI boot region represented by a streamed container chunk. */
static void rdx_hash_container_payload(UINT32_T container_offset,
                                       const UINT8_T *data, UINT32_T length)
{
    UINT32_T start = container_offset;
    UINT32_T end = container_offset + length;
    UINT32_T payload_start;
    UINT32_T payload_end;

    if ((end <= RDX_UPDATE_PAYLOAD_OFFSET) ||
        (start >= RDX_UPDATE_PAYLOAD_END))
    {
        return;
    }
    payload_start = (start < RDX_UPDATE_PAYLOAD_OFFSET) ?
                    RDX_UPDATE_PAYLOAD_OFFSET : start;
    payload_end = (end > RDX_UPDATE_PAYLOAD_END) ?
                  RDX_UPDATE_PAYLOAD_END : end;
    rdx_sha256_update(&rdx_update.payload_hash,
                      &data[payload_start - container_offset],
                      payload_end - payload_start);
}

/** Validate the explicit OpenRDX marker and boot-region integrity digest. */
static BOOLEAN_T rdx_custom_update_is_valid(const UINT8_T *payload_digest)
{
    UINT32_T index;

    if (!rdx_bytes_equal(rdx_update.custom_auth, rdx_custom_update_magic,
                         RDX_UPDATE_CUSTOM_MAGIC_LENGTH) ||
        !rdx_bytes_equal(
            &rdx_update.custom_auth[RDX_UPDATE_CUSTOM_DIGEST_OFFSET],
            payload_digest, RDX_UPDATE_CUSTOM_DIGEST_LENGTH))
    {
        return FALSE;
    }
    for (index = RDX_UPDATE_CUSTOM_DIGEST_OFFSET +
                 RDX_UPDATE_CUSTOM_DIGEST_LENGTH;
         index < RDX_UPDATE_CUSTOM_AUTH_LENGTH; index++)
    {
        if (rdx_update.custom_auth[index] != 0U)
        {
            return FALSE;
        }
    }
    return TRUE;
}

/** Begin a new sequential compatibility or OpenRDX firmware download. */
static STATUS_T rdx_begin_update(void)
{
    ti_memset(&rdx_update, 0, sizeof(rdx_update));
    rdx_sha256_init(&rdx_update.hash);
    rdx_sha256_init(&rdx_update.payload_hash);
    if (rdx_erase_boot_image() != STATUS_OK)
    {
        rdx_update.failed = TRUE;
        return STATUS_ERROR;
    }
    rdx_update.started = TRUE;
    return STATUS_OK;
}

/** Parse a three-byte big-endian integer from a SCSI CDB. */
static UINT32_T rdx_load_be24(const UINT8_T *bytes)
{
    return ((UINT32_T)bytes[0] << 16U) |
           ((UINT32_T)bytes[1] << 8U) |
           (UINT32_T)bytes[2];
}

/** Reset the in-RAM state of the RDX firmware download receiver. */
void rdx_manager_protocol_init(void)
{
    ti_memset(&rdx_update, 0, sizeof(rdx_update));
    rdx_update_reset_ticks = 0U;
    rdx_manager_identity_init();
    rdx_manager_control_init();
}

/** Reset after the successful mode-5 CSW has had time to reach the host. */
void rdx_manager_protocol_tick(void)
{
    if (rdx_update_reset_ticks != 0U)
    {
        rdx_update_reset_ticks--;
        if (rdx_update_reset_ticks == 0U)
        {
            usb_hal_disconnect();
            system_reset();
        }
    }
}

/** Build the variable-width RDX cartridge identity VPD page C0h. */
UINT32_T rdx_manager_build_media_id_vpd(UINT8_T *buffer, UINT32_T buffer_size,
                                        UINT8_T lun)
{
    UINT32_T offset = 5U;
    UINT32_T capacity_blocks;
    const UINT8_T *model = (const UINT8_T *)&ata_dev[lun].wModelNum[0];
    const UINT8_T *serial = (const UINT8_T *)&ata_dev[lun].wSerialNum[0];
    const RDX_MEDIA_IDENTITY_T *media_identity;

    if ((buffer == NULL) || (buffer_size < RDX_MEDIA_ID_MAX_LENGTH))
    {
        return 0U;
    }
    ti_memset(buffer, 0, RDX_MEDIA_ID_MAX_LENGTH);
    buffer[1] = RDX_VPD_MEDIA_ID_PAGE_CODE;
    /* Manager consumes the low 32 bits of the physical block count as eight
     * hexadecimal digits. A 320-GB cartridge therefore returns 2542EAB0 here,
     * not a MiB conversion or the smaller user-visible extent capacity. */
    capacity_blocks = (UINT32_T)ata_dev[lun].ddTrueMaxLBA;
    media_identity = rdx_get_media_identity(lun);

    if (!rdx_append_literal(buffer, buffer_size, &offset, "Cartridge:") ||
        !rdx_append_hex32(buffer, buffer_size, &offset, capacity_blocks) ||
        !rdx_append_ata_text(buffer, buffer_size, &offset, model, 40U, 40U) ||
        !rdx_append_ata_text(buffer, buffer_size, &offset, serial, 20U, 20U))
    {
        return 0U;
    }
    if (media_identity != NULL)
    {
        if (!rdx_append_ata_text(
                buffer, buffer_size, &offset, media_identity->vendor,
                sizeof(media_identity->vendor),
                sizeof(media_identity->vendor)) ||
            !rdx_append_ata_text(
                buffer, buffer_size, &offset, media_identity->model,
                sizeof(media_identity->model),
                sizeof(media_identity->model)) ||
            !rdx_append_ata_text(
                buffer, buffer_size, &offset, media_identity->serial,
                sizeof(media_identity->serial),
                sizeof(media_identity->serial)) ||
            !rdx_append_ata_text(
                buffer, buffer_size, &offset, media_identity->barcode,
                sizeof(media_identity->barcode),
                sizeof(media_identity->barcode)))
        {
            return 0U;
        }
    }
    else if (sata_media_get_kind(lun) == SATA_MEDIA_KIND_GENERIC)
    {
        /* ATA IDENTIFY has no separate manufacturer property. Publish its
         * standardized model text as the generic media label so Manager can
         * identify the disk without inventing a vendor. The remaining three
         * fields describe cartridge metadata and stay empty. */
        if (!rdx_append_ata_text(
                buffer, buffer_size, &offset, model, 40U,
                RDX_MEDIA_VENDOR_LENGTH) ||
            !rdx_append_literal(buffer, buffer_size, &offset, "") ||
            !rdx_append_literal(buffer, buffer_size, &offset, "") ||
            !rdx_append_literal(buffer, buffer_size, &offset, ""))
        {
            return 0U;
        }
    }
    else
    {
        /* Emit empty cartridge-property fields when neither authenticated
         * metadata nor an admitted generic disk is available. */
        if (!rdx_append_literal(buffer, buffer_size, &offset, "") ||
            !rdx_append_literal(buffer, buffer_size, &offset, "") ||
            !rdx_append_literal(buffer, buffer_size, &offset, "") ||
            !rdx_append_literal(buffer, buffer_size, &offset, ""))
        {
            return 0U;
        }
    }
    rdx_store_be16(&buffer[2], (UINT16_T)(offset - 4U));
    buffer[4] = (UINT8_T)(offset - 5U);
    return offset;
}

/** Build RDX media IDENTIFY DEVICE VPD page C2h. */
UINT32_T rdx_manager_build_media_identify_vpd(UINT8_T *buffer,
                                              UINT32_T buffer_size,
                                              UINT8_T lun)
{
    UINT32_T index;
    UINT16_T word;

    if ((buffer == NULL) || (buffer_size < RDX_MEDIA_IDENTIFY_LENGTH))
    {
        return 0U;
    }
    ti_memset(buffer, 0, RDX_MEDIA_IDENTIFY_LENGTH);
    buffer[1] = RDX_VPD_MEDIA_IDENTIFY_PAGE_CODE;
    rdx_store_be16(&buffer[2], 0x0200U);
    for (index = 0U; index < 256U; index++)
    {
        word = ata_dev[lun].wIdentifyDeviceInfo[index];
        rdx_store_be16(&buffer[4U + index * 2U], word);
    }
    return RDX_MEDIA_IDENTIFY_LENGTH;
}

/** Build the adapter identity VPD layout decoded by SCInqDevIdPg. */
UINT32_T rdx_manager_build_drive_id_vpd(UINT8_T *buffer,
                                        UINT32_T buffer_size)
{
    const RDX_DRIVE_IDENTITY_T *identity;

    if ((buffer == NULL) || (buffer_size < RDX_DRIVE_ID_VPD_LENGTH))
    {
        return 0U;
    }
    ti_memset(buffer, 0, RDX_DRIVE_ID_VPD_LENGTH);
    buffer[1] = 0x83U;
    buffer[3] = RDX_DRIVE_ID_VPD_LENGTH - 4U;
    buffer[4] = 0x02U; /* ASCII code set. */
    buffer[5] = 0x01U; /* T10 vendor identification designator. */
    buffer[7] = RDX_DRIVE_ID_VPD_LENGTH - 8U;
    identity = rdx_manager_get_drive_identity();
    ti_memcpy(&buffer[8], identity->vendor, sizeof(identity->vendor));
    ti_memcpy(&buffer[16], identity->product, sizeof(identity->product));
    ti_memcpy(&buffer[32], identity->serial, sizeof(identity->serial));
    return RDX_DRIVE_ID_VPD_LENGTH;
}

/** Build one RDX LOG SENSE page used by RDX Manager. */
UINT32_T rdx_manager_build_log_sense(UINT8_T *buffer, UINT32_T buffer_size,
                                     UINT8_T lun, UINT8_T page_code)
{
    UINT32_T offset;
    UINT32_T capacity_mib = (UINT32_T)(ata_dev[lun].ddTrueMaxLBA >> 11U);
    UINT32_T code;

    if (buffer == NULL)
    {
        return 0U;
    }
    if (page_code == RDX_LOG_SENSE_DRIVE_PAGE_CODE)
    {
        if (buffer_size < 56U)
        {
            return 0U;
        }
        ti_memset(buffer, 0, 56U);
        buffer[0] = page_code;
        rdx_store_be16(&buffer[2], 52U);
        offset = 4U;
        offset = rdx_append_log_parameter(buffer, offset, 0U, 0U, 2U);
        offset = rdx_append_log_parameter(
            buffer, offset, 1U, rdx_manager_get_drive_load_count(), 4U);
        offset = rdx_append_log_parameter(buffer, offset, 2U, 0U, 4U);
        offset = rdx_append_log_parameter(buffer, offset, 3U, 0U, 2U);
        offset = rdx_append_log_parameter(buffer, offset, 4U, 1U, 2U);
        offset = rdx_append_log_parameter(buffer, offset, 5U, 0U, 2U);
        offset = rdx_append_log_parameter(buffer, offset, 6U, 7U, 2U);
        offset = rdx_append_log_parameter(buffer, offset, 7U, 0U, 2U);
        return offset;
    }
    if (page_code == RDX_LOG_SENSE_CARTRIDGE_PAGE_CODE)
    {
        if (buffer_size < 116U)
        {
            return 0U;
        }
        ti_memset(buffer, 0, 116U);
        buffer[0] = page_code;
        rdx_store_be16(&buffer[2], 112U);
        offset = 4U;
        offset = rdx_append_log_parameter(buffer, offset, 0U, 0U, 2U);
        for (code = 1U; code <= 13U; code++)
        {
            UINT32_T value = 0U;
            if ((code == 2U) || (code == 3U))
            {
                value = capacity_mib;
            }
            else if (code == 10U)
            {
                value = ata_dev[lun].bSATA_Gen;
            }
            offset = rdx_append_log_parameter(buffer, offset,
                                              (UINT16_T)code, value, 4U);
        }
        return 116U;
    }
    if (page_code == RDX_LOG_SENSE_TEMPERATURE_PAGE_CODE)
    {
        if (buffer_size < 16U)
        {
            return 0U;
        }
        ti_memset(buffer, 0, 16U);
        buffer[0] = page_code;
        rdx_store_be16(&buffer[2], 12U);
        rdx_store_be16(&buffer[4], 0U);
        buffer[6] = 0x43U;
        buffer[7] = 2U;
        rdx_store_be16(&buffer[8], 0U);
        rdx_store_be16(&buffer[10], 1U);
        buffer[12] = 0x43U;
        buffer[13] = 2U;
        buffer[15] = rdx_hardware_get_temperature_celsius();
        return 16U;
    }
    return 0U;
}

/** Build one security-protocol record using Manager's big-endian fields. */
UINT32_T rdx_manager_build_security_protocol_in(
    UINT8_T *buffer, UINT32_T buffer_size, UINT16_T protocol_specific)
{
    UINT32_T length;

    switch (protocol_specific)
    {
        case 0x0000U:
            length = 18U;
            break;
        case 0x0001U:
            length = 6U;
            break;
        case 0x0010U:
            length = 44U;
            break;
        case 0x0011U:
            length = 5U;
            break;
        case 0x0012U:
            length = 16U;
            break;
        case 0x0020U:
            length = 24U;
            break;
        case 0x0021U:
            length = 16U;
            break;
        default:
            return 0U;
    }
    if ((buffer == NULL) || (buffer_size < length))
    {
        return 0U;
    }

    ti_memset(buffer, 0, length);
    rdx_store_be16(&buffer[0], protocol_specific);
    rdx_store_be16(&buffer[2], (UINT16_T)(length - 4U));

    if (protocol_specific == 0x0000U)
    {
        rdx_store_be16(&buffer[6], 0x0001U);
        rdx_store_be16(&buffer[8], 0x0010U);
        rdx_store_be16(&buffer[10], 0x0011U);
        rdx_store_be16(&buffer[12], 0x0012U);
        rdx_store_be16(&buffer[14], 0x0020U);
        rdx_store_be16(&buffer[16], 0x0021U);
    }
    else if (protocol_specific == 0x0001U)
    {
        rdx_store_be16(&buffer[4], 0x0001U);
    }
    else if (protocol_specific == 0x0010U)
    {
        /* With no security processor present, report one supported mechanism
         * and no active encrypted session. */
        buffer[20] = 1U;
        rdx_store_be16(&buffer[22], 0x0014U);
        buffer[24] = 0x20U;
        buffer[25] = 0x04U;
        rdx_store_be16(&buffer[26], 0x0200U);
        rdx_store_be16(&buffer[30], 0x0200U);
        rdx_store_be32(&buffer[40], 0x80010400UL);
    }
    else if (protocol_specific == 0x0011U)
    {
        buffer[4] = 1U;
    }
    else if (protocol_specific == 0x0012U)
    {
        buffer[7] = 4U;
    }
    else if (protocol_specific == 0x0020U)
    {
        /* Report protection inactive and cartridge access granted. */
        buffer[6] = 1U;
        buffer[7] = 1U;
    }
    else
    {
        /* Return the 16-byte no-security table response. */
        buffer[12] = 1U;
    }
    return length;
}

/** Receive one RDX Manager WRITE BUFFER firmware-download command. */
STATUS_T rdx_manager_handle_write_buffer(const UINT8_T *cdb,
                                         UINT32_T host_length,
                                         const UINT8_T *payload)
{
    UINT8_T digest[32];
    UINT8_T payload_digest[32];
    UINT32_T mode;
    UINT32_T offset;
    UINT32_T length;

    if (cdb == NULL)
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }
    mode = cdb[1] & 0x1FU;
    offset = rdx_load_be24(&cdb[3]);
    length = rdx_load_be24(&cdb[6]);
    if ((cdb[1] & 0xE0U) || (cdb[2] != 0U) || (cdb[9] != 0U) ||
        (length != host_length))
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }

    if (mode == RDX_WRITE_BUFFER_DOWNLOAD_MODE)
    {
        if ((length == 0U) || (length > RDX_UPDATE_MAX_CHUNK) ||
            (payload == NULL) || ((offset + length) > RDX_UPDATE_CONTAINER_LENGTH))
        {
            return STATUS_SCSI_INVALID_CMD_FIELD;
        }
        if (offset == 0U)
        {
            if (rdx_begin_update() != STATUS_OK)
            {
                return STATUS_SCSI_INTERNAL_TARGET_FAILURE;
            }
        }
        if (!rdx_update.started || rdx_update.failed ||
            (offset != rdx_update.next_container_offset))
        {
            return STATUS_SCSI_INVALID_CMD_FIELD;
        }
        rdx_sha256_update(&rdx_update.hash, payload, length);
        rdx_capture_custom_auth(offset, payload, length);
        rdx_hash_container_payload(offset, payload, length);
        if (rdx_program_container_payload(offset, payload, length) != STATUS_OK)
        {
            rdx_update.failed = TRUE;
            return STATUS_SCSI_INTERNAL_TARGET_FAILURE;
        }
        rdx_update.next_container_offset += length;
        if (rdx_update.next_container_offset == RDX_UPDATE_CONTAINER_LENGTH)
        {
            rdx_sha256_final(&rdx_update.hash, digest);
            rdx_sha256_final(&rdx_update.payload_hash, payload_digest);
            rdx_update.image_valid =
                rdx_bytes_equal(digest,
                                rdx_compatibility_image_sha256,
                                sizeof(digest)) ||
                rdx_custom_update_is_valid(payload_digest);
            if (!rdx_update.image_valid)
            {
                rdx_update.failed = TRUE;
                return STATUS_SCSI_AUTHENTICATION_FAILURE;
            }
        }
        return STATUS_OK;
    }

    if (mode == RDX_WRITE_BUFFER_ACTIVATE_MODE)
    {
        if ((length != 0U) || (host_length != 0U) ||
            (offset != RDX_UPDATE_CONTAINER_LENGTH) ||
            !rdx_update.started || !rdx_update.image_valid || rdx_update.failed)
        {
            return STATUS_SCSI_INVALID_CMD_FIELD;
        }
        if (rdx_program_flash(0U, rdx_update.first_vector_word,
                              sizeof(rdx_update.first_vector_word)) != STATUS_OK)
        {
            return STATUS_SCSI_INTERNAL_TARGET_FAILURE;
        }
        /* Returning GOOD lets BOT transmit the CSW before the periodic service
         * performs the reset.  Resetting here makes RDX Manager report a
         * communication failure even though the image was already committed. */
        rdx_update_reset_ticks = RDX_UPDATE_RESET_DELAY_TICKS;
        return STATUS_OK;
    }
    return STATUS_SCSI_INVALID_CMD_FIELD;
}
