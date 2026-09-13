/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
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
#define DEBUG(...) ((void)0)
#define ti_memcpy memcpy
#define ti_memset memset
#define ONE_TOUCH_STATE_SIZE 1
#define RDX_MECHANISM_DIAGNOSTIC_LENGTH 64

/* PRODUCTION_STATUS_AND_DEFINES */

static UINT8_T flash[0x40000], initial_flash[0x40000];
static unsigned int spi_calls, sector_erases;
static int write_enabled, read_failure;
static UINT8_T scsi_resp_buff[256];
static struct { UINT8_T dev_speed; } usb_dev;
typedef struct {
    UINT8_T pCommandBlock[16];
    void *pData;
    UINT32_T dDataByteCnt;
} PROBE_INPUT_T;
static PROBE_INPUT_T input;

/* The command handler only consumes these fields from the full firmware state. */
static struct { PROBE_INPUT_T *pCmdInput; } scsi_cmd = { &input };

/** Fail the probe with an actionable assertion rather than touching hardware. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}

/** Model write-enable, sector/chip erases, and one-way flash programming. */
static STATUS_T SpiOps(UINT8_T opcode, UINT32_T address, UINT8_T *buffer,
                       UINT32_T length, UINT32_T chip_select)
{
    UINT32_T index;
    (void)chip_select;
    spi_calls++;
    require(address <= sizeof(flash) && length <= sizeof(flash) - address,
            "SPI operation exceeds simulated flash");
    if (opcode == OpcodeWriteEnable)
    {
        write_enabled = 1;
        return STATUS_OK;
    }
    if (opcode == OpcodeReadData)
    {
        if (read_failure) return STATUS_ERROR;
        memcpy(buffer, flash + address, length);
        return STATUS_OK;
    }
    require(write_enabled, "flash mutation without write-enable");
    write_enabled = 0;
    if (opcode == OpcodeChipErase)
        memset(flash, 0xFF, sizeof(flash));
    else if (opcode == OpcodeSectorErase)
    {
        require(address % 4096 == 0 && address <= sizeof(flash) - 4096,
                "invalid flash sector erase");
        memset(flash + address, 0xFF, 4096);
        sector_erases++;
    }
    else if (opcode == OpcodePageProgram)
    {
        require(length <= 256 - (address & 255), "program crosses flash page");
        for (index = 0; index < length; index++)
            flash[address + index] &= buffer[index];
    }
    else
        require(0, "unexpected SPI operation");
    return STATUS_OK;
}

/** Supply an inert button response for the unexercised query command. */
static UINT8_T *one_touch_button_state_query(void) { return scsi_resp_buff; }
/** Reject an unexpected USB disconnect in these command-handler tests. */
static void usb_hal_disconnect(void) { require(0, "unexpected USB disconnect"); }
/** Reject an unexpected system reset in these command-handler tests. */
static void system_reset(void) { require(0, "unexpected reset"); }
/** Reject an unrelated diagnostic query in the flash probe. */
static BOOLEAN_T rdx_mechanism_read_diagnostics(UINT8_T *buffer, UINT32_T size)
{
    (void)buffer;
    (void)size;
    require(0, "unexpected mechanism query");
    return FALSE;
}

/** Firmware download commands must never invoke the separate serial writer. */
static STATUS_T rdx_manager_serial_write(const UINT8_T *payload, UINT32_T length,
                                         BOOLEAN_T *mutation_started)
{
    (void)payload;
    (void)length;
    (void)mutation_started;
    require(0, "firmware update called the serial writer");
    return STATUS_SCSI_INVALID_CMD;
}

BOOLEAN_T rdx_manufacturing_mutation_started(void);
/* PRODUCTION_COMMAND_HANDLERS */

/** Exercise a real SCSI handler and compare the complete resulting flash. */
int main(int argc, char **argv)
{
    UINT32_T index, offset, length;
    UINT8_T container[62110], cdb[10] = {0x3B, 4, 0};
    STATUS_T status;
    FILE *stream;
    require(argc >= 2, "missing probe scenario");
    for (index = 0; index < sizeof(flash); index++)
        flash[index] = (UINT8_T)(index * 23U + 11U);
    memcpy(initial_flash, flash, sizeof(flash));
    if (strcmp(argv[1], "erase") == 0 || strcmp(argv[1], "unlock") == 0 ||
        strcmp(argv[1], "unlock-erase") == 0)
    {
        if (strcmp(argv[1], "erase") != 0)
        {
            input.pCommandBlock[0] = 0xE1;
            status = scsi_handle_ti_defined_cmd();
            if (strcmp(argv[1], "unlock") == 0)
            {
                require(status == STATUS_SCSI_INVALID_CMD, "legacy unlock returned success");
                require(spi_calls == 0, "legacy unlock issued an SPI operation");
                return 0;
            }
        }
        input.pCommandBlock[0] = 0xE2;
        status = scsi_handle_ti_defined_cmd();
        require(memcmp(flash, initial_flash, sizeof(flash)) == 0,
                "legacy E2 modified flash, including manufacturing/state records");
        require(status == STATUS_SCSI_INVALID_CMD, "legacy E2 did not return INVALID CMD");
        require(spi_calls == 0, "legacy E2 issued an SPI operation");
        return 0;
    }
    if (strcmp(argv[1], "read-failure") == 0 || strcmp(argv[1], "read") == 0)
    {
        input.pCommandBlock[0] = 0xE7;
        input.pCommandBlock[2] = 3;
        input.pCommandBlock[3] = 0xE0;
        input.pCommandBlock[5] = 128;
        memset(scsi_resp_buff, 0xA5, sizeof(scsi_resp_buff));
        read_failure = strcmp(argv[1], "read-failure") == 0;
        status = scsi_handle_ti_defined_cmd();
        if (read_failure) {
            require(status == STATUS_SCSI_INTERNAL_TARGET_FAILURE, "Failed flash read returned success");
            require(input.pData == NULL && input.dDataByteCnt == 0, "Failed read exposed stale response bytes");
        } else {
            require(status == STATUS_SCSI_RESPONSE_READY, "Valid flash read failed");
            require(input.dDataByteCnt == 128, "Incorrect flash response length");
            require(memcmp(input.pData, flash + 0x3E000, 128) == 0, "Wrong flash response bytes");
        }
        require(spi_calls == 1, "Flash read retried unexpectedly");
        require(memcmp(flash, initial_flash, sizeof(flash)) == 0, "Read changed flash");
        return 0;
    }
    if (strcmp(argv[1], "restore-interlocks") == 0)
    {
        require(!rdx_manager_manufacturing_restore_blocked(), "Idle receiver blocked");
        rdx_update.failed = TRUE;
        require(rdx_manager_manufacturing_restore_blocked(), "Failed update admitted restore");
        rdx_update.failed = FALSE; rdx_update.started = TRUE;
        require(rdx_manager_manufacturing_restore_blocked(), "Active update admitted restore");
        rdx_update.started = FALSE; rdx_update_reset_ticks = 1;
        require(rdx_manager_manufacturing_restore_blocked(), "Pending reset admitted restore");
        rdx_update_reset_ticks = 0; rdx_serial_mutation_started = TRUE;
        require(rdx_manager_manufacturing_restore_blocked(), "Prior serial write admitted restore");
        return 0;
    }
    require(argc == 3, "missing container fixture");
    stream = fopen(argv[2], "rb");
    require(stream != NULL, "cannot open container fixture");
    require(fread(container, 1, sizeof(container), stream) == sizeof(container),
            "short container fixture");
    fclose(stream);
    for (offset = 0; offset < sizeof(container); offset += length)
    {
        length = sizeof(container) - offset;
        if (length > 4096) length = 4096;
        cdb[3] = (UINT8_T)(offset >> 16);
        cdb[4] = (UINT8_T)(offset >> 8);
        cdb[5] = (UINT8_T)offset;
        cdb[6] = (UINT8_T)(length >> 16);
        cdb[7] = (UINT8_T)(length >> 8);
        cdb[8] = (UINT8_T)length;
        require(rdx_manager_handle_write_buffer(cdb, length, container + offset) == STATUS_OK,
                "valid mode04 chunk was rejected");
        require(memcmp(flash + 0x10000, initial_flash + 0x10000, 0x30000) == 0,
                "mode04 modified flash above the boot image");
    }
    require(sector_erases == 16, "mode04 must erase exactly the 16 boot sectors");
    require(memcmp(flash, "\xFF\xFF\xFF\xFF", 4) == 0, "boot vector committed early");
    require(memcmp(flash + 4, container + 0x190, 0xF10A) == 0, "incorrect firmware payload");
    cdb[1] = 5;
    cdb[3] = 0;
    cdb[4] = 0xF2;
    cdb[5] = 0x9E;
    cdb[6] = cdb[7] = cdb[8] = 0;
    require(rdx_manager_handle_write_buffer(cdb, 0, NULL) == STATUS_OK,
            "valid activation was rejected");
    require(memcmp(flash, container + 0x18C, 0xF10E) == 0, "incorrect activated image");
    require(memcmp(flash + 0x10000, initial_flash + 0x10000, 0x30000) == 0,
            "activation modified high flash");
    return 0;
}

/** This probe isolates boot erasure; the manufacturing latch is tested separately. */
BOOLEAN_T rdx_manufacturing_mutation_started(void) { return FALSE; }
