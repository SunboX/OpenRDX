/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/* Host-only register/storage boundary. The receiver itself is included by cc. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef uint8_t UINT8_T;
typedef uint16_t UINT16_T;
typedef uint32_t UINT32_T;
typedef int BOOLEAN_T;
typedef int STATUS_T;
#define TRUE 1
#define FALSE 0
#define STATUS_OK 0
#define STATUS_ERROR 1
#define STATUS_SCSI_INVALID_CMD_FIELD 2
#define STATUS_SCSI_INTERNAL_TARGET_FAILURE 3
#define STATUS_SCSI_LOGICAL_UNIT_NOT_READY 4
#define RDX_MANUFACTURING_TRANSFER_LENGTH 528U
#define OpcodeReadData 3U
#define OpcodeWriteEnable 6U
#define OpcodeWriteDisable 4U
#define OpcodeSectorErase 0x20U
#define OpcodePageProgram 2U
#define VIM_REQMASKSET0 1U
#define VIM_REQMASKCLR0 2U
#define PxSSTS(port) 3U
#define PxCI(port) 4U
#define PxSACT(port) 5U
#define PSSTS_DET_MASK 15U
#define PSSTS_DET_PHY_READY 3U
#define ATA_CALLBACK_QUEUE_DEPTH 8U

static struct {
    UINT8_T scsi_response_buffer[4116];
    UINT8_T normal_data_buffer[4096];
} simulated_datapath;
#define datapath_ram (&simulated_datapath)
static struct {
    BOOLEAN_T bDeviceInitComplete;
    BOOLEAN_T callback_pending[ATA_CALLBACK_QUEUE_DEPTH];
} ata_dev[1];
static UINT32_T registers[6];
static UINT8_T flash[4096];
static UINT8_T before_restore[4096];
static UINT8_T request[528];
static UINT8_T command[10] = {0x3b, 2, 0x4d, 0, 0, 0, 0, 2, 0x10, 0};
static BOOLEAN_T occupied, active, ejecting, spi_owned, insert_after_read, insert_after_enable;
static unsigned reads, erases, programs, enables;
static int fail_opcode = -1;
static BOOLEAN_T spoil_readback, write_enabled;
static BOOLEAN_T update_active;
#define RDX_SERIAL_BUFFER_ID 0x53U
#define RDX_MANUFACTURING_BUFFER_ID 0x4DU
UINT8_T *rdx_manager_write_buffer_data(const UINT8_T *cdb);
BOOLEAN_T rdx_manufacturing_mutation_started(void);
typedef struct {
    UINT8_T *pCommandBlock;
    UINT8_T bCmdBlkLength;
    UINT32_T dDataXferLength;
} TEST_COMMAND_T;
static TEST_COMMAND_T command_input;
static struct { TEST_COMMAND_T *pCmdInput; } scsi_cmd;

/** Stub update lifecycle at the SCSI dispatcher boundary. */
BOOLEAN_T rdx_manager_manufacturing_restore_blocked(void) { return update_active; }
/** A manufacturing request must never be misrouted to the boot-image updater. */
static STATUS_T rdx_manager_handle_write_buffer(const UINT8_T *cdb,
                                               UINT32_T length,
                                               const UINT8_T *payload)
{
    (void)cdb; (void)length; (void)payload;
    return STATUS_SCSI_INVALID_CMD_FIELD;
}

/** Abort one simulated run with a useful failing invariant. */
static void require(int condition, const char *message)
{
    if (!condition) { fprintf(stderr, "%s\n", message); exit(1); }
}

/** Copy bytes without depending on the fake production string header. */
static void *ti_memcpy(void *destination, const void *source, UINT32_T length)
{
    UINT8_T *to = destination;
    const UINT8_T *from = source;
    UINT32_T index;
    for (index = 0; index < length; index++) to[index] = from[index];
    return destination;
}

/** Compare complete flash images and exact protocol markers. */
static BOOLEAN_T equal(const UINT8_T *left, const UINT8_T *right, UINT32_T length)
{
    UINT32_T index;
    for (index = 0; index < length; index++) if (left[index] != right[index]) return FALSE;
    return TRUE;
}

/** Match the selected host test scenario. */
static BOOLEAN_T scenario_is(const char *left, const char *right)
{
    while (*left && *left == *right) { left++; right++; }
    return *left == *right;
}

/** Read emulated live registers. */
static UINT32_T read_register(UINT32_T address) { return registers[address]; }
#define READ_REG32(address) read_register(address)
#define READ32(address) read_register(address)

/** Preserve the VIM set/clear semantics rather than overwriting saved masks. */
static void write_register(UINT32_T address, UINT32_T value)
{
    if (address == VIM_REQMASKCLR0) registers[VIM_REQMASKSET0] &= ~value;
    else if (address == VIM_REQMASKSET0) registers[address] |= value;
    else registers[address] = value;
}
#define WRITE_REG32(address, value) write_register(address, value)

/** Expose the physical GPIO5 signal independent of cached ATA readiness. */
static BOOLEAN_T gio_rdx_cartridge_present(void) { return occupied; }
/** Expose mechanism activity independently from the cartridge sensor. */
static BOOLEAN_T rdx_mechanism_is_active(void) { return active; }
/** Expose queued eject work before a mechanism starts. */
static BOOLEAN_T rdx_hardware_eject_in_progress(void) { return ejecting; }
/** Model contention with a preempted foreground ADC/flash operation. */
static BOOLEAN_T rdx_spi_acquire(void)
{
    if (spi_owned) return FALSE;
    spi_owned = TRUE;
    return TRUE;
}
/** Release ownership only after a successful acquisition. */
static void rdx_spi_release(void) { spi_owned = FALSE; }

/** Emulate NOR semantics and verify every receiver operation is sector-bounded. */
static STATUS_T SpiOpsBounded(UINT8_T opcode, UINT32_T address,
                             UINT8_T *buffer, UINT32_T length, UINT32_T chip)
{
    UINT32_T offset, index;
    require(spi_owned, "SPI transaction without exclusive ownership");
    require(chip == 0, "Wrong chip select");
    if (opcode == OpcodeWriteEnable) {
        enables++;
        if (fail_opcode == (int)opcode) return STATUS_ERROR;
        write_enabled = TRUE;
        if (insert_after_enable) occupied = TRUE;
        return STATUS_OK;
    }
    if (opcode == OpcodeWriteDisable) { write_enabled = FALSE; return STATUS_OK; }
    require(address >= 0x3e000 && address + length <= 0x3f000,
            "Operation escaped manufacturing sector");
    offset = address - 0x3e000;
    if (opcode == OpcodeReadData) {
        reads++;
        if (fail_opcode == (int)opcode) return STATUS_ERROR;
        ti_memcpy(buffer, flash + offset, length);
        if (spoil_readback && programs && offset <= 4000 && offset + length > 4000)
            buffer[4000 - offset] ^= 1;
        if (insert_after_read) occupied = TRUE;
        return STATUS_OK;
    }
    require(write_enabled, "Mutation without WREN");
    write_enabled = FALSE;
    if (opcode == OpcodeSectorErase) {
        require(address == 0x3e000 && length == 0, "Wrong erased sector");
        erases++;
        if (fail_opcode == (int)opcode) return STATUS_ERROR;
        for (index = 0; index < sizeof(flash); index++) flash[index] = 0xff;
        return STATUS_OK;
    }
    require(opcode == OpcodePageProgram, "Unsupported mutation opcode");
    require(length == 256 && (offset & 255) == 0, "Unbounded page write");
    programs++;
    if (fail_opcode == (int)opcode) return STATUS_ERROR;
    for (index = 0; index < length; index++) flash[offset + index] &= buffer[index];
    return STATUS_OK;
}

/** Fixture checksum independently calculates each lane by strided summation. */
static void checksum(UINT8_T *record)
{
    UINT32_T lane, index;
    const UINT8_T seed[4] = {0x78, 0x56, 0x34, 0x12};
    for (lane = 0; lane < 4; lane++) {
        record[lane] = seed[lane];
        for (index = lane + 4; index < 256; index += 4) record[lane] += record[index];
    }
}

/** Compute a fixture CRC using a table independent of the receiver bit loop. */
static UINT32_T fixture_crc(const UINT8_T *data, UINT32_T length)
{
    UINT32_T table[256], index, bit, value = 0xffffffffU;
    for (index = 0; index < 256; index++) {
        table[index] = index;
        for (bit = 0; bit < 8; bit++)
            table[index] = (table[index] >> 1) ^ ((table[index] & 1) ? 0xedb88320U : 0);
    }
    for (index = 0; index < length; index++) value = (value >> 8) ^ table[(value ^ data[index]) & 255];
    return ~value;
}

/** Capture expected current record and full-sector fingerprint. */
static void capture_expected(void)
{
    UINT32_T crc = fixture_crc(flash, sizeof(flash));
    unsigned index;
    for (index = 0; index < 4; index++) request[index + 8] = (UINT8_T)(crc >> (index * 8));
    ti_memcpy(request + 16, flash, 256);
    ti_memcpy(before_restore, flash, sizeof(flash));
}

/** Initialize nonuniform neighboring bytes so incomplete preservation is visible. */
static void initialize(void)
{
    unsigned index;
    for (index = 0; index < sizeof(flash); index++) flash[index] = (UINT8_T)(index * 37 + index / 256);
    checksum(flash);
    ti_memcpy(request, "RDXMFG01", 8);
    for (index = 0; index < 256; index++) request[index + 272] = (UINT8_T)(index * 11 + 7);
    checksum(request + 272);
    capture_expected();
    registers[VIM_REQMASKSET0] = 0x12345678;
}

STATUS_T rdx_manager_handle_manufacturing(const UINT8_T *cdb, UINT32_T host_length,
                                         const UINT8_T *payload);
STATUS_T scsi_handle_write_buffer_cmd(void);

/** Exercise production receiver paths; all failing admissions must leave flash intact. */
int main(int argc, char **argv)
{
    STATUS_T status;
    UINT32_T length = 528, index;
    const UINT8_T *payload = request, *cdb = command;
    BOOLEAN_T success = FALSE, mutated_failure = FALSE, busy = FALSE;
    require(argc == 2, "Missing scenario");
    initialize();
    if (scenario_is(argv[1], "restore")) success = TRUE;
    else if (scenario_is(argv[1], "stale-record")) request[24] ^= 1;
    else if (scenario_is(argv[1], "stale-sector")) { flash[3000] ^= 1; before_restore[3000] ^= 1; }
    else if (scenario_is(argv[1], "corrupt-desired")) request[300] ^= 1;
    else if (scenario_is(argv[1], "erased-desired")) for (index = 272; index < 528; index++) request[index] = 255;
    else if (scenario_is(argv[1], "erased-current")) {
        for (index = 0; index < 256; index++) flash[index] = 255;
        capture_expected(); success = TRUE;
    } else if (scenario_is(argv[1], "corrupt-current")) {
        flash[30] ^= 1; capture_expected(); success = TRUE;
    } else if (scenario_is(argv[1], "occupied")) occupied = TRUE;
    else if (scenario_is(argv[1], "insertion-race")) insert_after_read = TRUE;
    else if (scenario_is(argv[1], "wren-insertion-race")) insert_after_enable = TRUE;
    else if (scenario_is(argv[1], "mechanism")) active = TRUE;
    else if (scenario_is(argv[1], "eject")) ejecting = TRUE;
    else if (scenario_is(argv[1], "sata")) registers[PxSSTS(0)] = PSSTS_DET_PHY_READY;
    else if (scenario_is(argv[1], "callbacks")) ata_dev[0].callback_pending[7] = TRUE;
    else if (scenario_is(argv[1], "malformed-cdb")) {
        for (index = 0; index < 10; index++) {
            command[index] ^= 0x80;
            require(rdx_manager_handle_manufacturing(command, length, payload) != STATUS_OK,
                    "Malformed CDB admitted");
            command[index] ^= 0x80;
        }
    } else if (scenario_is(argv[1], "length")) length = 527;
    else if (scenario_is(argv[1], "long-payload")) length = 529;
    else if (scenario_is(argv[1], "null")) payload = NULL;
    else if (scenario_is(argv[1], "null-cdb")) cdb = NULL;
    else if (scenario_is(argv[1], "magic")) request[0] ^= 1;
    else if (scenario_is(argv[1], "reserved")) request[15] = 1;
    else if (scenario_is(argv[1], "capability")) {
        command[5] = 1; command[7] = 0; command[8] = 0;
        occupied = TRUE; active = TRUE; length = 0; payload = NULL; success = TRUE;
    } else if (scenario_is(argv[1], "noop")) {
        ti_memcpy(request + 272, flash, 256); success = TRUE;
    } else if (scenario_is(argv[1], "read-failure")) fail_opcode = OpcodeReadData;
    else if (scenario_is(argv[1], "wren-failure")) fail_opcode = OpcodeWriteEnable;
    else if (scenario_is(argv[1], "erase-failure")) { fail_opcode = OpcodeSectorErase; mutated_failure = TRUE; }
    else if (scenario_is(argv[1], "program-failure")) { fail_opcode = OpcodePageProgram; mutated_failure = TRUE; }
    else if (scenario_is(argv[1], "verify-failure")) { spoil_readback = TRUE; mutated_failure = TRUE; }
    else if (scenario_is(argv[1], "spi-busy")) { spi_owned = TRUE; busy = TRUE; }
    else if (scenario_is(argv[1], "dispatcher")) success = TRUE;
    else if (scenario_is(argv[1], "short-cdb") || scenario_is(argv[1], "long-cdb")) { }
    else if (scenario_is(argv[1], "update-active")) update_active = TRUE;
    else require(FALSE, "Unknown scenario");

    if (scenario_is(argv[1], "malformed-cdb")) command[1] = 0xff;
    if (scenario_is(argv[1], "dispatcher") || scenario_is(argv[1], "short-cdb") ||
        scenario_is(argv[1], "long-cdb") || scenario_is(argv[1], "update-active")) {
        ti_memcpy(rdx_manager_write_buffer_data(command), request, sizeof(request));
        /* An ATA DMA completion before command dispatch must not touch payload. */
        for (index = 0; index < 4096; index++) datapath_ram->normal_data_buffer[index] = 0x33;
        command_input.pCommandBlock = command;
        command_input.bCmdBlkLength = scenario_is(argv[1], "short-cdb") ? 9 :
            (scenario_is(argv[1], "long-cdb") ? 11 : 10);
        command_input.dDataXferLength = length;
        scsi_cmd.pCmdInput = &command_input;
        status = scsi_handle_write_buffer_cmd();
    } else status = rdx_manager_handle_manufacturing(cdb, length, payload);
    require((status == STATUS_OK) == success, "Unexpected receiver status");
    require(registers[VIM_REQMASKSET0] == 0x12345678, "Interrupt enables were not preserved");
    require(spi_owned == busy, "SPI ownership was leaked or released without acquisition");
    if (success && !scenario_is(argv[1], "capability")) {
        require(equal(flash, request + 272, 256), "Saved record was not restored exactly");
        require(equal(flash + 256, before_restore + 256, 3840), "Neighboring bytes changed");
    }
    if (scenario_is(argv[1], "capability")) require(reads == 0, "Capability performed flash I/O");
    if (!mutated_failure && (!success || scenario_is(argv[1], "noop") || scenario_is(argv[1], "capability"))) {
        require(erases == 0 && programs == 0, "Rejected/noop command mutated flash");
        if (!insert_after_enable && fail_opcode != OpcodeWriteEnable)
            require(enables == 0, "Rejected/noop command enabled writes");
        require(equal(flash, before_restore, sizeof(flash)), "Rejected command changed flash bytes");
    }
    if (scenario_is(argv[1], "wren-insertion-race"))
        require(!write_enabled, "Rejected insertion left flash write-enabled");
    if (scenario_is(argv[1], "erase-failure")) require(programs == 0, "Programming continued after erase failure");
    if (scenario_is(argv[1], "program-failure")) require(programs == 1, "Programming retried after failure");
    if (scenario_is(argv[1], "restore")) require(erases == 1 && programs == 16, "Restore did not rewrite exactly one sector");
    if (enables || fail_opcode == OpcodeWriteEnable) {
        unsigned prior_reads = reads;
        require(rdx_manufacturing_mutation_started(), "Mutation interlock missing");
        require(rdx_manager_handle_manufacturing(command, length, request) != STATUS_OK,
                "Second restore admitted before restart");
        require(prior_reads == reads, "Blocked restore performed flash I/O");
    } else require(!rdx_manufacturing_mutation_started(), "Rejected/probe request latched mutation");
    return 0;
}
