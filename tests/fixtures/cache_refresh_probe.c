/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t UINT8_T;
typedef uint16_t UINT16_T;
typedef uint32_t UINT32_T;
typedef int BOOLEAN_T;
#define TRUE 1
#define FALSE 0
#define ATA_CMD_IDENTIFY_DEVICE 0xECU
#define H2D_REGISTER_FIS_TYPE 0x27U
#define RDX_RUNTIME_DATA_BYTES 512U
#define RDX_PIO_COMPLETION_STATUS 7U
#define PxIS(port) (port)
#define WRITE32(address, value) observe_status(address, value)
#define ti_memset memset

typedef struct {
    struct { UINT8_T FIS_type, CRRR_PMP, command; } fis;
    UINT32_T dDataByteCnt;
} ATA_COMMAND_T;
typedef struct { UINT32_T epoch; } RDX_RUNTIME_SESSION_T;
typedef struct {
    uint64_t ddMaxLBA, ddTrueMaxLBA;
    UINT32_T dSectorSize, dTrueSectorSize;
    UINT16_T wIdentifyDeviceInfo[256];
    UINT8_T identity[128];
} DEVICE_T;
static DEVICE_T ata_dev[1];
static struct { UINT8_T normal_data_buffer[512]; } ram;
static const struct { UINT8_T *normal_data_buffer; } datapath_value = { ram.normal_data_buffer };
#define datapath_ram (&datapath_value)
static int busy, failed, replaced, dispatches, unlocks, acknowledgements;
static UINT16_T reply_bits;

/** Fail a host invariant without issuing operating-system or device calls. */
static void require(int condition, const char *message)
{
    if (!condition) { fprintf(stderr, "%s\n", message); exit(1); }
}

/** Model idle-port reservation. */
static BOOLEAN_T rdx_runtime_begin_sync_commands(UINT32_T port, RDX_RUNTIME_SESSION_T *session)
{
    require(port == 0U, "wrong port");
    session->epoch = 17U;
    return !busy;
}

/** Supply native IDENTIFY capacity and mutable cache bits in the shared buffer. */
static BOOLEAN_T rdx_runtime_execute_sync_command(UINT32_T port, ATA_COMMAND_T *command,
                                                  const RDX_RUNTIME_SESSION_T *session)
{
    uint64_t native_count = UINT64_C(5860533168);
    require(port == 0U && session->epoch == 17U, "wrong media session");
    require(command->fis.command == ATA_CMD_IDENTIFY_DEVICE && command->dDataByteCnt == 512U,
            "unexpected ATA command");
    dispatches++;
    memset(ram.normal_data_buffer, 0xA5, sizeof(ram.normal_data_buffer));
    memcpy(ram.normal_data_buffer + 200, &native_count, sizeof(native_count));
    ram.normal_data_buffer[170] = (UINT8_T)reply_bits;
    ram.normal_data_buffer[171] = (UINT8_T)(reply_bits >> 8U);
    return !failed;
}

/** Refuse commit ownership after a simulated cartridge change. */
static BOOLEAN_T rdx_runtime_lock_current_session(UINT32_T port, const RDX_RUNTIME_SESSION_T *session)
{
    require(port == 0U && session->epoch == 17U, "wrong commit session");
    return !replaced;
}

/** Count interrupt restoration only while media still matches. */
static void rdx_runtime_unlock_current_session(UINT32_T port, const RDX_RUNTIME_SESSION_T *session)
{
    require(port == 0U && session->epoch == 17U && !replaced, "restored stale interrupt ownership");
    unlocks++;
}

/** Check command-status acknowledgement framing. */
static void observe_status(UINT32_T port, UINT32_T status)
{
    require(port == 0U && status == RDX_PIO_COMPLETION_STATUS, "wrong completion mask");
    acknowledgements++;
}

/* PRODUCTION_FUNCTION */

/** Check all admitted bytes, including capacity and identity, after a refresh. */
static void check_case(UINT16_T bits)
{
    DEVICE_T expected;
    BOOLEAN_T successful;
    memset(ata_dev, 0x5A, sizeof(ata_dev));
    ata_dev[0].ddMaxLBA = UINT64_C(5860524976);
    ata_dev[0].ddTrueMaxLBA = UINT64_C(5860533168);
    ata_dev[0].dSectorSize = ata_dev[0].dTrueSectorSize = 512U;
    memcpy(&expected, &ata_dev[0], sizeof(expected));
    dispatches = unlocks = acknowledgements = 0;
    reply_bits = bits;
    if (!busy && !failed && !replaced)
        expected.wIdentifyDeviceInfo[85] =
            (UINT16_T)((expected.wIdentifyDeviceInfo[85] & 0xFF9FU) | (bits & 0x0060U));
    successful = rdx_refresh_cache_info(0U);
    require(successful == (!busy && !failed && !replaced), "wrong refresh outcome");
    require(memcmp(&expected, &ata_dev[0], sizeof(expected)) == 0, "admitted media state overwritten");
    require(dispatches == (busy ? 0 : 1), "wrong command count");
    require(acknowledgements == (busy ? 0 : 2), "unbalanced completion acknowledgements");
    require(unlocks == (busy || replaced ? 0 : 1), "wrong interrupt restoration count");
}

/** Exercise one requested isolated scenario without hardware. */
int main(int argc, char **argv)
{
    UINT16_T bits;
    require(argc == 2, "scenario missing");
    busy = strcmp(argv[1], "busy") == 0;
    failed = strcmp(argv[1], "failed") == 0;
    replaced = strcmp(argv[1], "replaced") == 0;
    require(busy || failed || replaced || strcmp(argv[1], "success") == 0, "unknown scenario");
    for (bits = 0U; bits <= 0x60U; bits = (UINT16_T)(bits + 0x20U))
        check_case((UINT16_T)(bits | 0xA500U));
    return 0;
}
