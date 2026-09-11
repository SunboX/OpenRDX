/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/*! @file
 * @brief Authenticated RDX cartridge mount and ATA access initialization.
 */

#include "rdx_unlock.h"

#include "reg_io.h"
#include "string.h"

#define RDX_ATA_SECURITY_UNLOCK      0xF2U
#define RDX_SECURITY_BLOCK_SIZE      512U
#define RDX_PASSWORD_SIZE            32U
#define RDX_SYNC_TIMEOUT_MS          9000
#define RDX_TEA_DELTA                0x9E3779B9U
#define RDX_METADATA_BYTES           512U
#define RDX_METADATA_DATA_BYTES      508U
#define RDX_METADATA_FIRST_OFFSET    4U
#define RDX_METADATA_FRONT_SEED      0x12345678U
#define RDX_METADATA_REAR_SEED       0x87654321U
#define RDX_METADATA_REAR_DISTANCE   0x0FFFU
#define RDX_METADATA_TYPE_LBA32      0x02U
#define RDX_METADATA_TYPE_LBA64      0x1BU
#define RDX_METADATA_TYPE_CONTEXT3   0x03U
#define RDX_METADATA_LINK_LIMIT      16U

static UINT64_T rdx_media_lba_offset[NUM_AHCI_PORTS];
static BOOLEAN_T rdx_metadata_root_readable[NUM_AHCI_PORTS];
static BOOLEAN_T rdx_metadata_layout_detected[NUM_AHCI_PORTS];
static BOOLEAN_T rdx_metadata_io_failed[NUM_AHCI_PORTS];

/** Completion class for one initialization-time ATA command. */
typedef enum _RDX_INITIALIZATION_COMMAND_RESULT_T
{
    RDX_INITIALIZATION_COMMAND_SUCCESS = 0,
    RDX_INITIALIZATION_COMMAND_REJECTED,
    RDX_INITIALIZATION_COMMAND_TRANSPORT_FAILURE
} RDX_INITIALIZATION_COMMAND_RESULT_T;

typedef struct _RDX_METADATA_RECORD_T
{
    UINT8_T type;
    UINT8_T flags;
    UINT16_T length;
    UINT64_T first;
    UINT64_T second;
} RDX_METADATA_RECORD_T;

static const UINT8_T rdx_metadata_pointer_types[3] =
{
    0x04U, 0x06U, 0x0BU
};

static const UINT8_T rdx_metadata_identity_link_types[4] =
{
    RDX_METADATA_TYPE_CONTEXT3, 0x04U, 0x06U, 0x0BU
};

/* These four little-endian words are the clear model-key seed. They are
 * algorithm data, not a password tied to one cartridge, model, or capacity. */
static const UINT32_T rdx_model_seed[4] =
{
    0xFBA7FB42U, 0x0032092BU, 0xC2BE1B5EU, 0x164582DBU
};

/**
 * @brief Execute one initialization-time ATA command synchronously.
 *
 * Build slot zero, issue a non-queued command, wait only for PxCI bit zero to
 * clear, and then reject either PxTFD error or device-fault status. Do not add
 * a second BSY/DRQ wait or manipulate PxIE/PxIS: ahci_init_port() keeps port
 * interrupts disabled for this initialization sequence. Runtime polling uses
 * the separately synchronized runtime-ATA module after interrupts are live.
 *
 * @param port_num SATA port number.
 * @param ata_cmd Fully populated ATA command descriptor.
 * @return Completion class separating ATA rejection from transport failure.
 */
static RDX_INITIALIZATION_COMMAND_RESULT_T
rdx_execute_initialization_sync_command(
    UINT32_T port_num,
    ATA_COMMAND_T *ata_cmd)
{
    STATUS_T status;

    status = ahci_build_cmd(port_num, ata_cmd, 0U);
    if (status == STATUS_OK)
    {
        status = ahci_issue_cmd(port_num, 0U, FALSE);
    }
    if (status == STATUS_OK)
    {
        status = ahci_wait_complete(
            PxCI(port_num), 0x01U, 0U, RDX_SYNC_TIMEOUT_MS);
    }

    if (status != STATUS_OK)
    {
        return RDX_INITIALIZATION_COMMAND_TRANSPORT_FAILURE;
    }
    if ((READ32(PxTFD(port_num)) & PTFD_STS_FAILURE_MASK) != 0U)
    {
        return RDX_INITIALIZATION_COMMAND_REJECTED;
    }
    return RDX_INITIALIZATION_COMMAND_SUCCESS;
}

/**
 * @brief Read a little-endian 16-bit value from cartridge metadata.
 *
 * @param data Source bytes.
 * @return Decoded value.
 */
static UINT16_T rdx_read_le16(const volatile UINT8_T *data)
{
    return (UINT16_T)data[0] | ((UINT16_T)data[1] << 8);
}

/**
 * @brief Read a little-endian 32-bit value from cartridge metadata.
 *
 * @param data Source bytes.
 * @return Decoded value.
 */
static UINT32_T rdx_read_le32(const volatile UINT8_T *data)
{
    return (UINT32_T)data[0] |
           ((UINT32_T)data[1] << 8) |
           ((UINT32_T)data[2] << 16) |
           ((UINT32_T)data[3] << 24);
}

/**
 * @brief Read a little-endian 64-bit value from cartridge metadata.
 *
 * @param data Source bytes.
 * @return Decoded value.
 */
static UINT64_T rdx_read_le64(const volatile UINT8_T *data)
{
    return (UINT64_T)rdx_read_le32(data) |
           ((UINT64_T)rdx_read_le32(data + 4) << 32);
}

/**
 * @brief Apply the variable-width Block-TEA-like key transform.
 *
 * @param words Mutable little-endian word array.
 * @param word_count Number of words; must be greater than one.
 * @param key Four-word encryption key.
 */
static void rdx_block_tea_encrypt(UINT32_T *words, UINT32_T word_count,
                                  const UINT32_T key[4])
{
    UINT32_T previous = words[word_count - 1U];
    UINT32_T rounds = (52U / word_count) + 6U;
    UINT32_T sum = 0U;
    UINT32_T position;

    /* Use unsigned 32-bit wraparound and carry the newly updated word into the
     * next position. The encoded delta bytes 0D CE 8D F5 require a fixed 54h
     * plus byte-index adjustment and therefore become B9 79 37 9E, or
     * little-endian 9E3779B9h. Using the encoded bytes directly breaks the
     * model-key and password vectors. */
    while (rounds != 0U)
    {
        sum += RDX_TEA_DELTA;

        for (position = 0U; position < word_count; position++)
        {
            UINT32_T key_index = (((sum & 0x0FU) >> 2U) ^
                                  (position & 0x03U));
            UINT32_T mixed = (sum + key[key_index]) ^
                             (previous + ((previous << 4U) ^
                                          (previous >> 5U)));

            words[position] += mixed;
            previous = words[position];
        }

        rounds--;
    }
}

/**
 * @brief Derive the intermediate 128-bit key from an ATA model string.
 *
 * @param device Parsed ATA IDENTIFY data.
 * @param key Receives the four-word derived key.
 */
static void rdx_derive_model_key(const ATA_DEVICE_INFO_T *device,
                                 UINT32_T key[4])
{
    UINT8_T material[48];
    UINT32_T block;

    /* ahci_save_device_info() has already swapped ATA string words into normal
     * ASCII order.  Preserve all 40 model bytes, including space padding. */
    ti_memcpy(material, device->wModelNum, sizeof(device->wModelNum));
    /* Append the little-endian ATA maximum-LBA value, not the addresses of its
     * in-memory fields. The value comes from IDENTIFY words 60-61 or 100-103;
     * using the parsed value keeps derivation valid across cartridge models
     * and capacities. */
    ti_memcpy(material + sizeof(device->wModelNum), &device->ddTrueMaxLBA,
              sizeof(device->ddTrueMaxLBA));
    ti_memcpy(key, rdx_model_seed, sizeof(rdx_model_seed));

    /* Encrypt the three 16-byte chunks in sequence. Each encrypted chunk is
     * the key for the next chunk. */
    for (block = 0U; block < 3U; block++)
    {
        UINT32_T encrypted[4];

        ti_memcpy(encrypted, material + (block * sizeof(encrypted)),
                  sizeof(encrypted));
        rdx_block_tea_encrypt(encrypted, 4U, key);
        ti_memcpy(key, encrypted, sizeof(encrypted));
    }
}

/**
 * @brief Generate the 32-byte password used by ATA SECURITY UNLOCK.
 *
 * @param device Parsed ATA IDENTIFY data.
 * @param password Receives eight encrypted little-endian words.
 */
static void rdx_generate_password(const ATA_DEVICE_INFO_T *device,
                                  UINT32_T password[RDX_PASSWORD_SIZE /
                                                    sizeof(UINT32_T)])
{
    static const UINT8_T prefix[9] =
    {
        'P', 'a', 's', 's', 'w', 'o', 'r', 'd', ':'
    };
    UINT32_T key[4];
    UINT8_T *password_bytes = (UINT8_T *)password;

    rdx_derive_model_key(device, key);

    /* Construct exactly 32 bytes: the nine-byte prefix, all 20 byte-swapped
     * ATA serial bytes, and three zero bytes. */
    ti_memcpy(password_bytes, prefix, sizeof(prefix));
    ti_memcpy(password_bytes + sizeof(prefix), device->wSerialNum,
              sizeof(device->wSerialNum));
    ti_memset(password_bytes + sizeof(prefix) + sizeof(device->wSerialNum), 0U,
              RDX_PASSWORD_SIZE - sizeof(prefix) -
              sizeof(device->wSerialNum));
    rdx_block_tea_encrypt(password, 8U, key);
    ti_memset(key, 0U, sizeof(key));
}

/**
 * @brief Send the synchronous ATA F2h data-out command.
 *
 * @param port_num SATA port number.
 * @param password Generated cartridge password.
 * @return Completion class for the access command.
 */
static RDX_INITIALIZATION_COMMAND_RESULT_T rdx_issue_security_unlock(
    UINT32_T port_num,
    const UINT32_T password[RDX_PASSWORD_SIZE / sizeof(UINT32_T)])
{
    ATA_COMMAND_T ata_cmd;

    /* ATA SECURITY UNLOCK always transfers one 512-byte parameter block.
     * Clear reserved bytes so previous datapath traffic cannot leak into it. */
    ti_memset((void *)datapath_ram->normal_data_buffer, 0U,
              RDX_SECURITY_BLOCK_SIZE);

    /* Word zero remains zero to select the user password.  ATA defines the
     * 32-byte password at byte offset two (word one). */
    ti_memcpy((void *)(datapath_ram->normal_data_buffer + 2),
              password, RDX_PASSWORD_SIZE);

    ti_memset(&ata_cmd, 0U, sizeof(ata_cmd));
    ata_cmd.fis.FIS_type = H2D_REGISTER_FIS_TYPE;
    ata_cmd.fis.CRRR_PMP = 0x80U;
    ata_cmd.fis.command = RDX_ATA_SECURITY_UNLOCK;
    ata_cmd.bIsWriteCmd = TRUE;
    ata_cmd.dDataByteCnt = RDX_SECURITY_BLOCK_SIZE;

    return rdx_execute_initialization_sync_command(port_num, &ata_cmd);
}

/**
 * @brief Read one physical SATA sector without using a memory-wrap window.
 *
 * Build READ DMA or READ DMA EXT for the cartridge-metadata walk and execute
 * it synchronously through the ordinary data buffer. Host block transfers
 * continue to use the streaming wrap windows.
 *
 * @param port_num SATA port number.
 * @param device Parsed ATA device information.
 * @param lba Physical sector to read.
 * @return TRUE when transport and ATA status both report success.
 */
static BOOLEAN_T rdx_read_raw_sector(UINT32_T port_num,
                                     const ATA_DEVICE_INFO_T *device,
                                     UINT64_T lba)
{
    ATA_COMMAND_T ata_cmd;

    ti_memset(&ata_cmd, 0U, sizeof(ata_cmd));
    ata_cmd.fis.FIS_type = H2D_REGISTER_FIS_TYPE;
    ata_cmd.fis.CRRR_PMP = 0x80U;
    ata_cmd.fis.LBA_low = (UINT8_T)lba;
    ata_cmd.fis.LBA_mid = (UINT8_T)(lba >> 8);
    ata_cmd.fis.LBA_high = (UINT8_T)(lba >> 16);
    ata_cmd.fis.device = 0x40U;
    ata_cmd.fis.sector_cnt = 1U;

    if (device->bLBA48)
    {
        ata_cmd.fis.command = ATA_CMD_READ_DMA_EXT;
        ata_cmd.fis.LBA_low_exp = (UINT8_T)(lba >> 24);
        ata_cmd.fis.LBA_mid_exp = (UINT8_T)(lba >> 32);
        ata_cmd.fis.LBA_high_exp = (UINT8_T)(lba >> 40);
    }
    else
    {
        ata_cmd.fis.command = ATA_CMD_READ_DMA;
        ata_cmd.fis.device |= (UINT8_T)(lba >> 24) & 0x0FU;
    }

    ata_cmd.dDataByteCnt = device->dTrueSectorSize;
    ata_cmd.bCheckCondition = TRUE;

    return (rdx_execute_initialization_sync_command(port_num, &ata_cmd) ==
            RDX_INITIALIZATION_COMMAND_SUCCESS) ? TRUE : FALSE;
}

/**
 * @brief Validate one front or rear RDX metadata checksum.
 *
 * Sum the first 508 bytes independently in four byte lanes, starting at
 * 12345678h for the front copy and 87654321h for the rear copy, then compare
 * the result with the little-endian word at byte 508.
 *
 * @param data 512-byte metadata sector.
 * @param seed Copy-specific checksum seed.
 * @return TRUE when the stored checksum matches.
 */
static BOOLEAN_T rdx_metadata_checksum_valid(
    const volatile UINT8_T *data,
    UINT32_T seed)
{
    UINT8_T checksum[4];
    UINT32_T index;
    UINT32_T calculated;

    checksum[0] = (UINT8_T)seed;
    checksum[1] = (UINT8_T)(seed >> 8);
    checksum[2] = (UINT8_T)(seed >> 16);
    checksum[3] = (UINT8_T)(seed >> 24);

    for (index = 0U; index < RDX_METADATA_DATA_BYTES; index++)
    {
        checksum[index & 3U] =
            (UINT8_T)(checksum[index & 3U] + data[index]);
    }

    calculated = (UINT32_T)checksum[0] |
                 ((UINT32_T)checksum[1] << 8) |
                 ((UINT32_T)checksum[2] << 16) |
                 ((UINT32_T)checksum[3] << 24);
    return calculated == rdx_read_le32(data + RDX_METADATA_DATA_BYTES);
}

/**
 * @brief Decode one typed record from an RDX metadata sector.
 *
 * @param data Metadata sector.
 * @param wanted_type Record type to locate.
 * @param record Receives the decoded header.
 * @return TRUE when a valid active record of the requested type was found.
 */
static BOOLEAN_T rdx_find_metadata_record(
    const volatile UINT8_T *data,
    UINT8_T wanted_type,
    RDX_METADATA_RECORD_T *record)
{
    UINT32_T offset = RDX_METADATA_FIRST_OFFSET;

    while (offset < RDX_METADATA_DATA_BYTES)
    {
        UINT8_T type = data[offset];
        UINT8_T flags = data[offset + 1U];
        UINT16_T length = rdx_read_le16(data + offset + 2U);
        UINT32_T header_size;

        if ((flags & 0x04U) != 0U)
        {
            header_size = 20U;
        }
        else if ((flags & 0x01U) != 0U)
        {
            header_size = 12U;
        }
        else
        {
            header_size = 4U;
        }

        /* Reject zero, short, oversized, and end-crossing records. Without
         * these strict bounds a malformed length could loop forever or move
         * the parser beyond the checksummed metadata bytes. */
        if ((length < header_size) ||
            (length >= RDX_METADATA_DATA_BYTES) ||
            ((offset + length) >= RDX_METADATA_DATA_BYTES))
        {
            return FALSE;
        }

        if ((type == wanted_type) && ((flags & 0x05U) != 0U))
        {
            record->type = type;
            record->flags = flags;
            record->length = length;

            if ((flags & 0x04U) != 0U)
            {
                record->first = rdx_read_le64(data + offset + 4U);
                record->second = rdx_read_le64(data + offset + 12U);
            }
            else if ((flags & 0x01U) != 0U)
            {
                record->first = rdx_read_le32(data + offset + 4U);
                record->second = rdx_read_le32(data + offset + 8U);
            }
            else
            {
                return FALSE;
            }
            return TRUE;
        }

        offset += length;
    }

    return FALSE;
}

/**
 * @brief Recognize a structurally plausible RDX sector despite bad checksum.
 *
 * This does not make the sector usable. It only prevents a damaged RDX
 * layout from falling through to generic direct-LBA admission.
 *
 * @param data Readable 512-byte candidate sector.
 * @return TRUE when a known active pointer or extent record is present.
 */
static BOOLEAN_T rdx_metadata_structure_recognized(
    const volatile UINT8_T *data)
{
    RDX_METADATA_RECORD_T record;
    UINT32_T index;

    for (index = 0U; index < sizeof(rdx_metadata_pointer_types); index++)
    {
        if (rdx_find_metadata_record(
                data, rdx_metadata_pointer_types[index], &record))
        {
            return TRUE;
        }
    }
    return rdx_find_metadata_record(
               data, RDX_METADATA_TYPE_CONTEXT3, &record) ||
           rdx_find_metadata_record(
               data, RDX_METADATA_TYPE_LBA32, &record) ||
           rdx_find_metadata_record(
               data, RDX_METADATA_TYPE_LBA64, &record);
}

/**
 * @brief Load and select a checksummed front/rear metadata-sector copy.
 *
 * @param port_num SATA port number.
 * @param device Parsed ATA device information.
 * @param relative_lba Offset from the front/rear metadata anchors.
 * @param selected Receives the selected sector address in datapath RAM.
 * @return TRUE when at least one copy is readable and valid.
 */
static BOOLEAN_T rdx_load_metadata_sector(
    UINT32_T port_num,
    const ATA_DEVICE_INFO_T *device,
    UINT64_T relative_lba,
    const volatile UINT8_T **selected)
{
    volatile UINT8_T *rear = datapath_ram->normal_data_buffer;
    volatile UINT8_T *front_saved =
        datapath_ram->normal_data_buffer + RDX_METADATA_BYTES;
    UINT64_T last_lba;
    UINT64_T rear_lba;
    BOOLEAN_T front_valid = FALSE;
    BOOLEAN_T rear_valid = FALSE;
    BOOLEAN_T front_read;
    BOOLEAN_T rear_read;

    /* The rear-copy formula is:
     *
     *     raw_last_lba - 0x0FFF + relative_lba
     *
     * RDX therefore reserves matching 4,096-sector metadata areas at the
     * front and rear of the SATA medium. */
    if ((device->dTrueSectorSize != RDX_METADATA_BYTES) ||
        (device->ddTrueMaxLBA == 0U) ||
        (relative_lba > RDX_METADATA_REAR_DISTANCE))
    {
        return FALSE;
    }
    last_lba = device->ddTrueMaxLBA - 1U;
    if (last_lba < RDX_METADATA_REAR_DISTANCE)
    {
        return FALSE;
    }
    rear_lba = last_lba - RDX_METADATA_REAR_DISTANCE + relative_lba;

    front_read = rdx_read_raw_sector(port_num, device, relative_lba);
    if (front_read)
    {
        if (relative_lba == 0U)
        {
            rdx_metadata_root_readable[port_num] = TRUE;
        }
        front_valid = rdx_metadata_checksum_valid(
            datapath_ram->normal_data_buffer, RDX_METADATA_FRONT_SEED);
        if (!front_valid && rdx_metadata_structure_recognized(
                                datapath_ram->normal_data_buffer))
        {
            rdx_metadata_layout_detected[port_num] = TRUE;
        }
        if (front_valid)
        {
            ti_memcpy((void *)front_saved,
                      (const void *)datapath_ram->normal_data_buffer,
                      RDX_METADATA_BYTES);
            rdx_metadata_layout_detected[port_num] = TRUE;
            *selected = front_saved;
            return TRUE;
        }
    }
    else
    {
        rdx_metadata_io_failed[port_num] = TRUE;
        if ((READ32(PxCI(port_num)) & 0x01U) != 0U)
        {
            return FALSE;
        }
    }

    rear_read = rdx_read_raw_sector(port_num, device, rear_lba);
    if (rear_read)
    {
        if (relative_lba == 0U)
        {
            rdx_metadata_root_readable[port_num] = TRUE;
        }
        rear_valid = rdx_metadata_checksum_valid(
            rear, RDX_METADATA_REAR_SEED);
        if (!rear_valid && rdx_metadata_structure_recognized(rear))
        {
            rdx_metadata_layout_detected[port_num] = TRUE;
        }
    }
    else
    {
        rdx_metadata_io_failed[port_num] = TRUE;
        if ((READ32(PxCI(port_num)) & 0x01U) != 0U)
        {
            return FALSE;
        }
    }

    if (rear_valid)
    {
        rdx_metadata_layout_detected[port_num] = TRUE;
    }

    /* Mount validation never writes cartridge metadata. A valid front copy is
     * authoritative; the rear copy is the read-only fallback. */
    if (rear_valid)
    {
        *selected = rear;
        return TRUE;
    }
    return FALSE;
}

/**
 * @brief Check whether a metadata relative LBA is already queued.
 *
 * @param queue Relative-LBA queue.
 * @param count Number of active queue entries.
 * @param relative_lba Candidate relative LBA.
 * @return TRUE when the candidate is already present.
 */
static BOOLEAN_T rdx_metadata_link_is_queued(const UINT64_T *queue,
                                              UINT32_T count,
                                              UINT64_T relative_lba)
{
    UINT32_T index;

    for (index = 0U; index < count; index++)
    {
        if (queue[index] == relative_lba)
        {
            return TRUE;
        }
    }
    return FALSE;
}

/**
 * @brief Load cartridge identity and stored usage counters.
 *
 * Resolve context links through record types 04h, 06h, and 0Bh, follow the
 * active type-03h link for identity and type-04h links for usage counters.
 * Continue after finding identity so later counter objects are still read.
 * The bounded queue prevents cyclic metadata from causing an unbounded walk
 * and avoids dynamic allocation.
 *
 * @param port_num SATA port number.
 * @param device Parsed ATA device information.
 * @return TRUE when all four identity properties were loaded.
 */
static BOOLEAN_T rdx_load_media_identity(UINT32_T port_num,
                                          const ATA_DEVICE_INFO_T *device)
{
    UINT64_T queue[RDX_METADATA_LINK_LIMIT];
    BOOLEAN_T statistics_context[RDX_METADATA_LINK_LIMIT];
    UINT32_T queue_count = 1U;
    UINT32_T queue_index = 0U;

    /* Use the platform memory routine; aggregate initialization can emit a
     * C runtime memset call that is not linked into this firmware. */
    ti_memset(statistics_context, 0U, sizeof(statistics_context));
    queue[0] = 0U;
    while (queue_index < queue_count)
    {
        const volatile UINT8_T *sector;
        UINT32_T type_index;

        if (!rdx_load_metadata_sector(
                port_num, device, queue[queue_index], &sector))
        {
            queue_index++;
            continue;
        }

        rdx_capture_media_identity(port_num, sector);
        if (statistics_context[queue_index])
        {
            rdx_capture_media_statistics(port_num, sector);
        }

        for (type_index = 0U;
             type_index < sizeof(rdx_metadata_identity_link_types);
             type_index++)
        {
            RDX_METADATA_RECORD_T link;

            if (rdx_find_metadata_record(
                    sector, rdx_metadata_identity_link_types[type_index],
                    &link) &&
                (link.first <= RDX_METADATA_REAR_DISTANCE) &&
                (queue_count < RDX_METADATA_LINK_LIMIT) &&
                !rdx_metadata_link_is_queued(
                    queue, queue_count, link.first))
            {
                statistics_context[queue_count] =
                    (rdx_metadata_identity_link_types[type_index] == 0x04U);
                queue[queue_count++] = link.first;
            }
        }
        queue_index++;
    }
    return rdx_get_media_identity(port_num) != NULL;
}

/**
 * @brief Load the host-visible data extent from RDX metadata.
 *
 * Follow typed 04h, 06h, and 0Bh pointer records from the mirrored root sector,
 * then obtain type 02h for 32-bit media or 1Bh for 64-bit media. The record's
 * two values are the first and last physical user LBAs; the first value is
 * added to each host request.
 *
 * @param port_num SATA port number.
 * @param device Parsed ATA device information; visible capacity is updated.
 * @return TRUE when a valid user-data extent was loaded.
 */
static BOOLEAN_T rdx_load_media_extent(UINT32_T port_num,
                                       ATA_DEVICE_INFO_T *device)
{
    const volatile UINT8_T *sector;
    RDX_METADATA_RECORD_T record;
    UINT64_T relative_lba = 0U;
    UINT8_T extent_type;
    UINT32_T level;

    extent_type = ((device->ddTrueMaxLBA >> 32) == 0U) ?
                  RDX_METADATA_TYPE_LBA32 : RDX_METADATA_TYPE_LBA64;

    for (level = 0U; level <= 3U; level++)
    {
        if (!rdx_load_metadata_sector(
                port_num, device, relative_lba, &sector))
        {
            return FALSE;
        }

        /* The final context-3 sector carries both the user extent and the four
         * byte-string properties consumed by VPD page C0h. */
        rdx_capture_media_identity(port_num, sector);

        if (rdx_find_metadata_record(sector, extent_type, &record))
        {
            if ((record.first > record.second) ||
                (record.second >= device->ddTrueMaxLBA))
            {
                return FALSE;
            }

            rdx_media_lba_offset[port_num] = record.first;
            device->ddMaxLBA = record.second - record.first + 1U;
            return TRUE;
        }

        if ((level == 3U) ||
            !rdx_find_metadata_record(
                sector, rdx_metadata_pointer_types[level], &record) ||
            (record.first >= device->ddTrueMaxLBA))
        {
            return FALSE;
        }
        relative_lba = record.first;
    }

    return FALSE;
}

/** Clear cached RDX state at a SATA discovery boundary. */
void rdx_reset_media_context(UINT32_T port_num)
{
    if (port_num >= NUM_AHCI_PORTS)
    {
        return;
    }

    rdx_media_lba_offset[port_num] = 0U;
    rdx_reset_media_metadata(port_num);
    rdx_metadata_root_readable[port_num] = FALSE;
    rdx_metadata_layout_detected[port_num] = FALSE;
    rdx_metadata_io_failed[port_num] = FALSE;
}

/**
 * @brief Finish an RDX mount after access to metadata is available.
 *
 * @param port_num SATA port number.
 * @param device Parsed ATA IDENTIFY information.
 * @return TRUE when the checksummed user-data extent is valid.
 */
static BOOLEAN_T rdx_finish_media_mount(
    UINT32_T port_num,
    ATA_DEVICE_INFO_T *device)
{
    if (!rdx_load_media_extent(port_num, device))
    {
        return FALSE;
    }

    /* Identity and counters are advisory for presentation and must not make an
     * otherwise authenticated, valid cartridge unreadable. */
    (void)rdx_load_media_identity(port_num, device);
    return TRUE;
}

/**
 * @brief Authenticate and validate one non-packet RDX cartridge.
 *
 * @param port_num SATA port number.
 * @param device Parsed ATA IDENTIFY information.
 * @return Structured access and metadata-validation result.
 */
RDX_ACCESS_RESULT_T rdx_unlock_media(
    UINT32_T port_num,
    ATA_DEVICE_INFO_T *device)
{
    UINT32_T password[RDX_PASSWORD_SIZE / sizeof(UINT32_T)];
    RDX_INITIALIZATION_COMMAND_RESULT_T command_result;
    RDX_ACCESS_RESULT_T access_result;

    if (port_num >= NUM_AHCI_PORTS)
    {
        return RDX_ACCESS_RESULT_RETRYABLE_FAILURE;
    }

    /* Never cache an earlier result. The dispatcher invokes this operation
     * for a newly identified locked candidate, and successful access must be
     * followed by a fresh metadata walk for the current link. */
    rdx_reset_media_context(port_num);

    if ((device == NULL) || device->bPacketDevice ||
        (device->ddTrueMaxLBA == 0U) ||
        (device->dTrueSectorSize == 0U))
    {
        return RDX_ACCESS_RESULT_RETRYABLE_FAILURE;
    }

    /* Issue F2 after cartridge context construction without consulting
     * IDENTIFY word 128. RDX controllers need not advertise the desktop-disk
    * security-state combination. Packet devices remain excluded because F2
    * is an ATA disk command. */
    rdx_generate_password(device, password);
    command_result = rdx_issue_security_unlock(port_num, password);
    ti_memset(password, 0U, sizeof(password));
    if (command_result == RDX_INITIALIZATION_COMMAND_REJECTED)
    {
        access_result = RDX_ACCESS_RESULT_REJECTED;
    }
    else if (command_result != RDX_INITIALIZATION_COMMAND_SUCCESS)
    {
        access_result = RDX_ACCESS_RESULT_RETRYABLE_FAILURE;
    }
    else if (rdx_finish_media_mount(port_num, device))
    {
        /* Do not expose the medium unless the mirrored metadata walk succeeds.
         * Besides authenticating the RDX layout, the walk supplies the
         * physical start/end of the user-data extent required by every host
         * LBA. */
        access_result = RDX_ACCESS_RESULT_READY;
    }
    else
    {
        access_result = RDX_ACCESS_RESULT_RETRYABLE_FAILURE;
    }
    if (access_result != RDX_ACCESS_RESULT_READY)
    {
        rdx_reset_media_context(port_num);
    }
    return access_result;
}

/**
 * @brief Inspect and commit an already accessible RDX media layout.
 *
 * @param port_num SATA port number.
 * @param device Parsed ATA IDENTIFY information.
 * @return Classification result for the directly readable media.
 */
RDX_MEDIA_INSPECTION_T rdx_inspect_accessible_media(
    UINT32_T port_num,
    ATA_DEVICE_INFO_T *device)
{
    if ((port_num >= NUM_AHCI_PORTS) || (device == NULL) ||
        device->bPacketDevice)
    {
        return RDX_MEDIA_INSPECTION_UNREADABLE;
    }

    rdx_reset_media_context(port_num);
    if ((device->dTrueSectorSize != RDX_METADATA_BYTES) ||
        (device->ddTrueMaxLBA <= RDX_METADATA_REAR_DISTANCE))
    {
        return RDX_MEDIA_INSPECTION_NOT_RECOGNIZED;
    }
    if (rdx_finish_media_mount(port_num, device))
    {
        return RDX_MEDIA_INSPECTION_READY;
    }

    if (rdx_metadata_io_failed[port_num])
    {
        rdx_reset_media_context(port_num);
        return RDX_MEDIA_INSPECTION_UNREADABLE;
    }

    /* A checksummed metadata sector proves this is an RDX layout even when
     * its pointer chain or user extent is invalid. Never expose that physical
     * layout through the generic direct-LBA path. */
    if (rdx_metadata_layout_detected[port_num])
    {
        rdx_reset_media_context(port_num);
        return RDX_MEDIA_INSPECTION_INVALID;
    }

    if (rdx_metadata_root_readable[port_num])
    {
        rdx_reset_media_context(port_num);
        return RDX_MEDIA_INSPECTION_NOT_RECOGNIZED;
    }

    rdx_reset_media_context(port_num);
    return RDX_MEDIA_INSPECTION_UNREADABLE;
}

/**
 * @brief Translate a host-visible LBA through the authenticated data extent.
 *
 * @param port_num SATA port number.
 * @param logical_lba Host-visible logical block address.
 * @return Physical SATA logical block address.
 */
UINT64_T rdx_translate_media_lba(UINT32_T port_num, UINT64_T logical_lba)
{
    if (port_num >= NUM_AHCI_PORTS)
    {
        return logical_lba;
    }
    return logical_lba + rdx_media_lba_offset[port_num];
}
