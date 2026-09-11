# SPDX-FileCopyrightText: 2026 André Fiedler
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Exercise the production serial writer with a NOR-flash model, without USB."""

import ctypes
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
import zlib

ROOT = Path(__file__).resolve().parents[1]
CC = shutil.which("cc")


def release_probe(test_class):
    """Unload the completed host probe before removing its temporary directory."""
    library = test_class.lib
    test_class.lib = None
    if sys.platform == "win32":
        from _ctypes import FreeLibrary
        FreeLibrary(library._handle)
    else:
        from _ctypes import dlclose
        dlclose(library._handle)


def checksum(record):
    """Calculate the independently specified four-lane manufacturing checksum."""
    return bytes((seed + sum(record[4 + lane::4])) & 255
                 for lane, seed in enumerate((0x78, 0x56, 0x34, 0x12)))


def manufacturing_record(serial=b"1234567890"):
    """Build the documented fallback profile, with a caller-supplied unit serial."""
    record = bytearray(256)
    record[4] = 6
    record[8:18] = serial
    record[0x12:0x1a] = b"TANDBERG"
    record[0x1a:0x2a] = b"RDX             "
    record[0x30] = 1
    record[0x76:0x7a] = bytes.fromhex("5a 1a 06 00")
    record[0x7a:0x82] = b"TANDBERG"
    record[0x82:0x92] = b"RDX             "
    record[0x9c:0xa4] = b"10102010"
    record[0xa4] = 0x38
    record[:4] = checksum(record)
    return record


@unittest.skipUnless(CC, "Host logic probe requires cc; production uses TI CGT")
class SerialRepairTests(unittest.TestCase):
    """Ensure a serial command cannot silently damage other manufacturing bytes."""

    @classmethod
    def setUpClass(cls):
        """Compile the unchanged writer with bounded flash/GPIO test adapters."""
        cls.temporary = tempfile.TemporaryDirectory(prefix="openrdx-serial-")
        cls.addClassCleanup(cls.temporary.cleanup)
        directory = Path(cls.temporary.name)
        source = ROOT / "src/rdx_mount/rdx_manager_serial.c"
        if not source.exists():
            raise AssertionError("Production serial writer is not implemented")
        prelude = r"""
#include <stdint.h>
#include <string.h>
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
#define STATUS_SCSI_INVALID_CMD 4
#define OpcodeReadData 3
#define OpcodeWriteEnable 6
#define OpcodePageProgram 2
#define OpcodeSectorErase 0x20
#define NUM_AHCI_PORTS 1
#define PxCI(port) ((port) * 2)
#define PxSACT(port) ((port) * 2 + 1)
#define READ32(address) dma_registers[(address)]
#define ti_memcpy memcpy
#define ti_memset memset
typedef struct { UINT8_T normal_data_buffer[4096]; UINT8_T scsi_response_buffer[4116]; } DATAPATH_RAM_T;
static DATAPATH_RAM_T ram;
static DATAPATH_RAM_T *datapath_ram = &ram;
static UINT32_T dma_registers[2];
static int dma_during_snapshot;
static UINT8_T flash[0x40000];
static int writes, erases, enabled, media, fail_at, calls, corrupt_after_write;
BOOLEAN_T gio_rdx_cartridge_present(void) { return media; }
STATUS_T SpiOps(int op, UINT32_T address, UINT8_T *data, UINT32_T size, int cs) {
    (void)cs;
    if (++calls == fail_at) return STATUS_ERROR;
    if (op == OpcodeReadData) {
        if (address + size > sizeof(flash)) return STATUS_ERROR;
        memcpy(data, flash + address, size);
        if (dma_during_snapshot) dma_registers[0] = 1;
        if (corrupt_after_write && writes && address == 0x3ef00) data[255] ^= 1;
        return STATUS_OK;
    }
    if (op == OpcodeWriteEnable) { enabled = 1; return STATUS_OK; }
    if (!enabled || address < 0x3e000 || address >= 0x3f000) return STATUS_ERROR;
    enabled = 0;
    if (op == OpcodeSectorErase && address == 0x3e000) {
        erases++; memset(flash + address, 255, 4096); return STATUS_OK;
    }
    if (op == OpcodePageProgram && size <= 256 && (address & 255) + size <= 256) {
        writes++;
        for (UINT32_T i = 0; i < size; i++) flash[address + i] &= data[i];
        return STATUS_OK;
    }
    return STATUS_ERROR;
}
void seed(const UINT8_T *sector) {
    memset(flash, 0xa5, sizeof(flash)); memcpy(flash + 0x3e000, sector, 4096);
    writes = erases = enabled = media = fail_at = calls = corrupt_after_write = 0;
    dma_registers[0] = dma_registers[1] = 0; dma_during_snapshot = 0;
}
void inspect(UINT8_T *sector) { memcpy(sector, flash + 0x3e000, 4096); }
void fault(int at, int present, int corrupt) {
    fail_at = at; media = present; corrupt_after_write = corrupt;
}
void fault_dma(UINT32_T ci, UINT32_T sact, int later) {
    dma_registers[0] = ci; dma_registers[1] = sact; dma_during_snapshot = later;
}
void scribble_ata_buffer(void) { memset(ram.normal_data_buffer, 0x33, 4096); }
int write_count(void) { return writes; }
int erase_count(void) { return erases; }
int outside_unchanged(void) {
    for (UINT32_T i = 0; i < sizeof(flash); i++)
        if ((i < 0x3e000 || i >= 0x3f000) && flash[i] != 0xa5) return 0;
    return 1;
}
"""
        source_text = re.sub(r'^#include .*$', '', source.read_text(), flags=re.M)
        header = re.sub(r'^#include .*$', '', (
            ROOT / "include/rdx_mount/rdx_manager_serial.h").read_text(), flags=re.M)
        harness = directory / "probe.c"
        harness.write_text(prelude + header + source_text)
        library = directory / "probe.so"
        result = subprocess.run([CC, "-shared", "-fPIC", "-std=c99", "-Wall",
                                 "-Wextra", "-Werror", str(harness), "-o", str(library)],
                                capture_output=True, text=True, check=False)
        if result.returncode:
            raise AssertionError(result.stdout + result.stderr)
        cls.lib = ctypes.CDLL(str(library))
        # Class cleanups run last-in-first-out; Windows locks loaded modules.
        cls.addClassCleanup(release_probe, cls)
        cls.lib.rdx_manager_serial_write.argtypes = [ctypes.c_void_p, ctypes.c_uint32,
                                                     ctypes.POINTER(ctypes.c_int)]
        cls.lib.rdx_manager_serial_write.restype = ctypes.c_int

    def seed(self, record=None):
        """Use sentinel neighbor pages so page and sector preservation is observable."""
        self.sector = bytearray((index * 71 + 13) & 255 for index in range(4096))
        self.sector[:256] = manufacturing_record() if record is None else record
        self.lib.seed(bytes(self.sector))

    def request(self, serial=b"7820746632", initialize_erased=False):
        """Prepare the fixed request from an independent complete-sector snapshot."""
        return bytearray(b"RDXSER01" + zlib.crc32(self.sector).to_bytes(4, "little")
                         + bytes((int(initialize_erased), 0)) + serial + self.sector[:256])

    def write(self, request):
        """Call the production writer and return both status and mutation state."""
        changed = ctypes.c_int(-1)
        result = self.lib.rdx_manager_serial_write(bytes(request), len(request),
                                                   ctypes.byref(changed))
        return result, changed.value

    def read(self):
        """Return the entire sector observed after a command."""
        output = ctypes.create_string_buffer(4096)
        self.lib.inspect(output)
        return output.raw

    def test_changes_only_serial_checksum_and_preserves_neighbor_pages(self):
        """A change requiring erase retains every opaque field and adjacent page."""
        self.seed()
        self.sector[0xa6:0xd6] = bytes(range(48))
        self.sector[:4] = checksum(self.sector[:256])
        self.lib.seed(bytes(self.sector))
        self.assertEqual((0, 1), self.write(self.request()))
        expected = self.sector.copy()
        expected[8:18] = b"7820746632"
        expected[:4] = checksum(expected[:256])
        self.assertEqual(expected, self.read())
        self.assertEqual(1, self.lib.erase_count())
        self.assertEqual(1, self.lib.outside_unchanged())

    def test_erased_fallback_initialization_is_explicit_and_does_not_erase(self):
        """An all-FF record becomes the declared fallback profile, never guessed data."""
        self.seed(bytes([255]) * 256)
        self.assertEqual((2, 0), self.write(self.request()))
        self.assertEqual(0, self.lib.write_count())
        self.assertEqual((0, 1), self.write(self.request(initialize_erased=True)))
        expected = self.sector.copy()
        expected[:256] = manufacturing_record(b"7820746632")
        self.assertEqual(expected, self.read())
        self.assertEqual(0, self.lib.erase_count())
        self.assertEqual(1, self.lib.write_count())

    def test_same_serial_is_a_noop(self):
        """An idempotent serial request never consumes an erase/program cycle."""
        self.seed()
        self.assertEqual((0, 0), self.write(self.request(b"1234567890")))
        self.assertEqual(0, self.lib.write_count())
        self.assertEqual(0, self.lib.erase_count())

    def test_bad_frames_stale_sector_and_stale_record_never_write(self):
        """Reject malformed framing and snapshots changed outside the serial field."""
        for offset in (0, 8, 13, 14, 24):
            with self.subTest(offset=offset):
                self.seed()
                request = self.request()
                request[offset] ^= 128
                self.assertEqual((2, 0), self.write(request))
                self.assertEqual(self.sector, self.read())
                self.assertEqual(0, self.lib.erase_count())
        self.seed()
        request = self.request()
        for invalid in (request[:-1], request + b"\0"):
            self.assertEqual((2, 0), self.write(invalid))
        request[12] = 2
        self.assertEqual((2, 0), self.write(request))

    def test_partial_corruption_is_not_treated_as_blank(self):
        """Fallback initialization cannot discard a nonblank corrupt manufacturing object."""
        record = manufacturing_record()
        record[0] ^= 1
        self.seed(record)
        self.assertEqual((2, 0), self.write(self.request(initialize_erased=True)))
        self.assertEqual(0, self.lib.write_count())

    def test_occupied_bay_and_failed_snapshot_prevent_mutation(self):
        """Hardware occupancy and an unsuccessful flash read both stop the write."""
        for at, media in ((0, 1), (1, 0)):
            with self.subTest(at=at, media=media):
                self.seed()
                self.lib.fault(at, media, 0)
                status, changed = self.write(self.request())
                self.assertNotEqual(0, status)
                self.assertEqual(0, changed)
                self.assertEqual(self.sector, self.read())

    def test_write_failure_and_neighbor_readback_mismatch_are_not_success(self):
        """Report uncertain mutation on program failure or any sector readback mismatch."""
        for at, corrupt in ((5, 0), (0, 1)):
            with self.subTest(at=at, corrupt=corrupt):
                self.seed()
                self.lib.fault(at, 0, corrupt)
                status, changed = self.write(self.request())
                self.assertEqual(3, status)
                self.assertEqual(1, changed)
                self.assertEqual(1, self.lib.outside_unchanged())

    def test_pending_dma_cannot_share_serial_scratch_memory(self):
        """Reject any active ATA slot before scratch use and again before erase."""
        for ci, sact, later in ((1, 0, 0), (0x80000000, 0, 0),
                                (0, 0x100, 0), (0, 0, 1)):
            with self.subTest(ci=ci, sact=sact, later=later):
                self.seed()
                self.lib.fault_dma(ci, sact, later)
                self.assertEqual((4, 0), self.write(self.request()))
                self.assertEqual(self.sector, self.read())
                self.assertEqual(0, self.lib.write_count())
                self.assertEqual(0, self.lib.erase_count())

    def test_serial_receive_buffer_survives_ata_scratch_writes(self):
        """Receive into non-ATA storage even if an ATA DMA completes before dispatch."""
        self.assertTrue(hasattr(self.lib, "rdx_manager_write_buffer_data"),
                        "Serial receive storage must be separate from ATA scratch")
        select = self.lib.rdx_manager_write_buffer_data
        select.argtypes = [ctypes.c_void_p]
        select.restype = ctypes.c_void_p
        self.seed()
        serial = select(bytes.fromhex("3b025300000000011800"))
        firmware = select(bytes.fromhex("3b040000000000100000"))
        self.assertNotEqual(serial, firmware)
        request = self.request()
        ctypes.memmove(serial, bytes(request), len(request))
        self.lib.scribble_ata_buffer()
        changed = ctypes.c_int(0)
        self.assertEqual(0, self.lib.rdx_manager_serial_write(serial, len(request),
                                                            ctypes.byref(changed)))
        self.assertEqual(b"7820746632", self.read()[8:18])
        self.assertEqual(self.sector[256:], self.read()[256:])

    def test_receive_and_dispatch_use_the_same_buffer_selector(self):
        """Keep BOT reception and SCSI payload handoff wired to the isolated buffer."""
        bot = (ROOT / "src/rdx_mount/ums_bot.c").read_text()
        scsi = (ROOT / "src/rdx_mount/scsi.c").read_text()
        self.assertIn("rdx_manager_write_buffer_data(&gCBW->CB[0])", bot)
        self.assertIn("rdx_manager_write_buffer_data(\n"
                      "            scsi_cmd.pCmdInput->pCommandBlock)", scsi)

    def test_oversized_cbw_cannot_masquerade_as_280_byte_serial_request(self):
        """A final discarded 280-byte fragment cannot hide a 4376-byte host transfer."""
        self.assertTrue(hasattr(self.lib, "rdx_manager_write_buffer_host_length"),
                        "Serial dispatch must retain the complete BOT transfer length")
        length = self.lib.rdx_manager_write_buffer_host_length
        length.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32]
        length.restype = ctypes.c_uint32
        serial = bytes.fromhex("3b025300000000011800")
        self.assertEqual(4376, length(serial, 4376, 280))
        self.assertEqual(280, length(serial, 280, 280))
        self.assertEqual(4096, length(bytes.fromhex("3b040000000000100000"),
                                      8192, 4096))
        bot = (ROOT / "src/rdx_mount/ums_bot.c").read_text()
        self.assertIn("rdx_manager_write_buffer_host_length(\n"
                      "                        &gCBW->CB[0], gCBW->dDataTransferLength,", bot)


@unittest.skipUnless(CC, "Host logic probe requires cc; production uses TI CGT")
class SerialDispatchTests(unittest.TestCase):
    """Exercise the actual SCSI dispatcher, including update/serial exclusion."""

    @classmethod
    def setUpClass(cls):
        """Compile the command dispatcher with observable semantic write stubs."""
        from test_flash_preservation import production_function
        cls.temporary = tempfile.TemporaryDirectory(prefix="openrdx-serial-dispatch-")
        cls.addClassCleanup(cls.temporary.cleanup)
        directory = Path(cls.temporary.name)
        source = (ROOT / "src/rdx_mount/rdx_manager_protocol.c").read_text()
        header = (ROOT / "include/rdx_mount/rdx_manager_serial.h").read_text()
        constants = "\n".join(re.findall(r"^#define RDX_.*$", source, re.M))
        constants += "\n" + re.sub(r'^#include .*$', '', header, flags=re.M)
        prelude = r"""
#include <stdint.h>
#include <string.h>
#include <stddef.h>
typedef uint8_t UINT8_T; typedef uint32_t UINT32_T;
typedef int BOOLEAN_T; typedef int STATUS_T;
#define FALSE 0
#define TRUE 1
#define STATUS_OK 0
#define STATUS_SCSI_INVALID_CMD_FIELD 2
#define STATUS_SCSI_INTERNAL_TARGET_FAILURE 3
#define STATUS_SCSI_AUTHENTICATION_FAILURE 4
static struct { int hash, payload_hash, started, failed, image_valid;
 UINT32_T next_container_offset; UINT8_T first_vector_word[4]; } rdx_update;
static UINT8_T rdx_update_reset_ticks;
static BOOLEAN_T rdx_serial_mutation_started;
static UINT8_T rdx_compatibility_image_sha256[32];
static int serial_calls, mutate, serial_status, firmware_calls;
STATUS_T rdx_manager_serial_write(const UINT8_T *p, UINT32_T n, BOOLEAN_T *changed) {
 (void)p; (void)n; serial_calls++; *changed = mutate; return serial_status;
}
static int rdx_begin_update(void) { firmware_calls++; rdx_update.started = 1; return 0; }
static void rdx_sha256_update(int *c, const UINT8_T *p, UINT32_T n) { (void)c;(void)p;(void)n; }
static void rdx_sha256_final(int *c, UINT8_T *d) { (void)c;memset(d,0,32); }
static void rdx_capture_custom_auth(UINT32_T o, const UINT8_T *p, UINT32_T n) { (void)o;(void)p;(void)n; }
static void rdx_hash_container_payload(UINT32_T o, const UINT8_T *p, UINT32_T n) { (void)o;(void)p;(void)n; }
static int rdx_program_container_payload(UINT32_T o, const UINT8_T *p, UINT32_T n) { (void)o;(void)p;(void)n;return 0; }
static int rdx_program_flash(UINT32_T o, const UINT8_T *p, UINT32_T n) { (void)o;(void)p;(void)n;return 0; }
static int rdx_bytes_equal(const UINT8_T *a, const UINT8_T *b, UINT32_T n) { return memcmp(a,b,n)==0; }
static int rdx_custom_update_is_valid(const UINT8_T *p) { (void)p;return 1; }
void reset(int update, int m, int status) { memset(&rdx_update,0,sizeof(rdx_update));
 rdx_update.started=update; rdx_serial_mutation_started=0; serial_calls=firmware_calls=0;
 mutate=m;serial_status=status; }
int serial_count(void) { return serial_calls; }
int firmware_count(void) { return firmware_calls; }
"""
        production = production_function(source, "rdx_load_be24") + "\n"
        production += production_function(source, "rdx_manager_handle_write_buffer")
        harness = directory / "probe.c"
        harness.write_text(prelude + constants + "\n" + production)
        library = directory / "probe.so"
        result = subprocess.run([CC, "-shared", "-fPIC", "-std=c99", "-Wall", "-Werror",
                                 str(harness), "-o", str(library)], capture_output=True, text=True)
        if result.returncode:
            raise AssertionError(result.stdout + result.stderr)
        cls.lib = ctypes.CDLL(str(library))
        # Class cleanups run last-in-first-out; Windows locks loaded modules.
        cls.addClassCleanup(release_probe, cls)
        cls.lib.rdx_manager_handle_write_buffer.argtypes = [ctypes.c_void_p,
                                                           ctypes.c_uint32, ctypes.c_void_p]

    def command(self, cdb, length=None):
        """Submit a fixed command and a correctly sized synthetic transport buffer."""
        cdb = bytes.fromhex(cdb) if isinstance(cdb, str) else cdb
        length = int.from_bytes(cdb[6:9], "big") if length is None else length
        payload = bytes(length) if length else None
        return self.lib.rdx_manager_handle_write_buffer(cdb, length, payload)

    def test_capability_is_side_effect_free(self):
        """Capability negotiation must succeed without invoking a write or erase."""
        self.lib.reset(0, 1, 0)
        self.assertEqual(0, self.command("3b025300000100000000"))
        self.assertEqual(0, self.lib.serial_count())
        self.assertEqual(0, self.lib.firmware_count())

    def test_serial_dispatch_and_update_exclusion(self):
        """Only the exact new frame reaches the serial writer on an idle receiver."""
        self.lib.reset(0, 1, 0)
        self.assertEqual(0, self.command("3b025300000000011800"))
        self.assertEqual(1, self.lib.serial_count())
        self.assertEqual(2, self.command("3b040000000000000100"))
        self.assertEqual(0, self.lib.firmware_count())
        self.lib.reset(1, 1, 0)
        self.assertEqual(2, self.command("3b025300000000011800"))
        self.assertEqual(0, self.lib.serial_count())

    def test_invalid_serial_frames_do_not_reach_the_writer(self):
        """Reject wrong mode, id, offset, length, reserved bytes or host length."""
        for frame in ("3b045300000000011800", "3b025400000000011800",
                      "3b025300000200011800", "3b025300000000011700",
                      "3b025300000000011801", "3b225300000000011800",
                      "3b025300000100000100"):
            with self.subTest(frame=frame):
                self.lib.reset(0, 1, 0)
                self.assertEqual(2, self.command(frame))
                self.assertEqual(0, self.lib.serial_count())
                self.assertEqual(0, self.lib.firmware_count())
        self.lib.reset(0, 1, 0)
        self.assertEqual(2, self.command("3b025300000000011800", 279))
        self.assertEqual(0, self.lib.serial_count())
        self.assertEqual(2, self.command("3b025300000000011800", 4376))
        self.assertEqual(0, self.lib.serial_count())

    def test_uncertain_serial_write_blocks_firmware_until_reset(self):
        """A failed write that may have mutated flash cannot start a firmware update."""
        self.lib.reset(0, 1, 3)
        self.assertEqual(3, self.command("3b025300000000011800"))
        self.assertEqual(2, self.command("3b040000000000000100"))
        self.assertEqual(0, self.lib.firmware_count())
        self.lib.reset(0, 0, 2)
        self.assertEqual(2, self.command("3b025300000000011800"))
        self.assertEqual(0, self.command("3b040000000000000100"))
        self.assertEqual(1, self.lib.firmware_count())


if __name__ == "__main__":
    unittest.main()
