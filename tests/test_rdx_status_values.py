# SPDX-FileCopyrightText: 2026 André Fiedler
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Execute status builders with controlled inputs; TI still builds the firmware."""

import ctypes
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
CC = shutil.which("cc")


def function(source, name):
    """Extract an unchanged production function for a host logic probe."""
    match = re.search(r"(?:static )?(?:UINT16_T|UINT32_T|UINT64_T|BOOLEAN_T|void) " + name + r"\(", source)
    start = source.index("{", match.start())
    depth = 1
    end = start + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


@unittest.skipUnless(CC, "Host C logic probe requires cc; production uses TI CGT")
class StatusValuesTests(unittest.TestCase):
    """Exercise actual response bytes without accessing USB, GPIO, or storage."""

    @classmethod
    def setUpClass(cls):
        """Compile production status builders and identity loader with input shims."""
        cls.temporary = tempfile.TemporaryDirectory(prefix="openrdx-status-")
        cls.addClassCleanup(cls.temporary.cleanup)
        temporary = Path(cls.temporary.name)
        protocol = (ROOT / "src/rdx_mount/rdx_manager_protocol.c").read_text()
        status_path = ROOT / "src/rdx_mount/rdx_manager_status.c"
        status = status_path.read_text() if status_path.exists() else protocol
        identity = (ROOT / "src/rdx_mount/rdx_manager_identity.c").read_text()
        identity_header = (ROOT / "include/rdx_mount/rdx_manager_identity.h").read_text()
        remove_includes = lambda text: re.sub(r'^#include .*$', '', text, flags=re.M)
        metadata_path = ROOT / "src/rdx_mount/rdx_media_metadata.c"
        metadata = ""
        if metadata_path.exists():
            metadata = remove_includes((ROOT / "include/rdx_mount/rdx_unlock.h").read_text())
            metadata += remove_includes(metadata_path.read_text())
        prelude = r"""
#include <stdint.h>
#include <string.h>
typedef uint8_t UINT8_T;
typedef uint16_t UINT16_T;
typedef uint32_t UINT32_T;
typedef uint64_t UINT64_T;
typedef int BOOLEAN_T;
typedef int STATUS_T;
#define TRUE 1
#define FALSE 0
#define NULL ((void*)0)
#define STATUS_OK 0
#define OpcodeReadData 3
#define NUM_AHCI_PORTS 1
#define ti_memcpy memcpy
#define ti_memset memset
#define RDX_LOG_SENSE_DRIVE_PAGE_CODE 0x20
#define RDX_LOG_SENSE_CARTRIDGE_PAGE_CODE 0x21
#define RDX_LOG_SENSE_TEMPERATURE_PAGE_CODE 0x0d
#define SATA_MEDIA_KIND_GENERIC 1
typedef struct { UINT64_T ddTrueMaxLBA; UINT32_T dTrueSectorSize;
 UINT8_T bSATA_Gen; } ATA_DEVICE_INFO_T;
typedef struct { int unused; } RDX_RUNTIME_ATA_T;
static ATA_DEVICE_INFO_T ata_dev[1] = {{625142448ULL, 512, 2}};
static UINT8_T flash_record[256];
static int flash_ok, cam, self_powered, write_protected;
STATUS_T SpiOps(int opcode, UINT32_T address, void *buffer, UINT32_T size, int flags) {
 (void)opcode; (void)address; (void)flags;
 memcpy(buffer, flash_record, size); return flash_ok ? 0 : 1;
}
void seed_identity(const UINT8_T *bytes, int valid) {
 memcpy(flash_record, bytes, 256); flash_ok = valid;
}
void seed_gpio(int c, int p, int wp) { cam=c; self_powered=p; write_protected=wp; }
BOOLEAN_T gio_rdx_mechanism_input_asserted(void) { return cam; }
BOOLEAN_T gio_is_usb_device_self_powered(void) { return self_powered; }
BOOLEAN_T rdx_hardware_is_write_protected(void) { return write_protected; }
UINT8_T rdx_hardware_get_temperature_celsius(void) { return 25; }
UINT32_T rdx_manager_get_drive_load_count(void) { return 23; }
"""
        length = re.search(r'#define RDX_DRIVE_ID_VPD_LENGTH\s+\d+U', protocol).group()
        helpers = "\n".join(function(protocol, name) for name in ["rdx_store_be16", "rdx_store_be32"])
        builder_name = "rdx_manager_build_status_log" if status_path.exists() else "rdx_manager_build_log_sense"
        source = prelude + remove_includes(identity_header) + remove_includes(identity)
        source += metadata + "\n" + length + "\n" + helpers
        if metadata_path.exists():
            source += "\nvoid seed_statistics(const UINT8_T *data) { rdx_capture_media_statistics(0, data); }\n"
            unlock = (ROOT / "src/rdx_mount/rdx_unlock.c").read_text()
            source += "\n" + "\n".join(re.findall(r'^#define RDX_METADATA_.*$', unlock, re.M))
            source += "\n" + re.search(r'typedef struct _RDX_METADATA_RECORD_T.*?} RDX_METADATA_RECORD_T;', unlock, re.S).group()
            source += "\n" + re.search(r'static const UINT8_T rdx_metadata_identity_link_types.*?};', unlock, re.S).group()
            source += "\n" + "\n".join(function(unlock, name) for name in (
                "rdx_read_le16", "rdx_read_le32", "rdx_read_le64",
                "rdx_find_metadata_record", "rdx_metadata_link_is_queued",
            ))
            source += r"""
static UINT8_T metadata_sectors[4][512];
static UINT32_T metadata_reads;
void seed_sector(UINT32_T lba, const UINT8_T *data) {
 if (lba < 4) memcpy(metadata_sectors[lba], data, 512);
}
static BOOLEAN_T rdx_load_metadata_sector(UINT32_T port,
 const ATA_DEVICE_INFO_T *device, UINT64_T lba, const volatile UINT8_T **data) {
 (void)port; (void)device; metadata_reads++;
 if (lba >= 4) return FALSE;
 *data = metadata_sectors[lba]; return TRUE;
}
"""
            source += "\n" + function(unlock, "rdx_load_media_identity")
            source += "\nUINT32_T probe_mount_metadata(void) { metadata_reads=0; rdx_load_media_identity(0, &ata_dev[0]); return metadata_reads; }\n"
        else:
            source += "\nvoid seed_statistics(const UINT8_T *data) { (void)data; }\n"
        source += "\n" + function(status, "rdx_append_log_parameter")
        source += "\n" + function(protocol, "rdx_manager_build_drive_id_vpd")
        source += "\n" + function(status, builder_name)
        if builder_name != "rdx_manager_build_log_sense":
            source += "\n" + function(protocol, "rdx_manager_build_log_sense")
        (temporary / "probe.c").write_text(source)
        output = temporary / "probe.so"
        completed = subprocess.run([CC, "-shared", "-fPIC", "-std=c99", "-Wno-macro-redefined", str(temporary / "probe.c"), "-o", str(output)], capture_output=True, text=True)
        if completed.returncode:
            raise AssertionError(completed.stdout + completed.stderr)
        cls.lib = ctypes.CDLL(str(output))

    def setUp(self):
        """Reset every input before each independent response assertion."""
        self.lib.seed_identity(bytes(256), 0)
        self.lib.rdx_manager_identity_init()
        self.lib.seed_gpio(1, 0, 0)
        if hasattr(self.lib, "rdx_reset_media_metadata"):
            self.lib.rdx_reset_media_metadata(0)

    def log(self, page):
        """Decode integer parameters from a production LOG SENSE response."""
        output = (ctypes.c_ubyte * 256)()
        length = self.lib.rdx_manager_build_log_sense(output, 256, 0, page)
        raw = bytes(output[:length])
        self.assertEqual(length, int.from_bytes(raw[2:4], "big") + 4)
        result, offset = {}, 4
        while offset + 4 <= length:
            code, size = int.from_bytes(raw[offset:offset+2], "big"), raw[offset+3]
            self.assertNotIn(code, result)
            self.assertLessEqual(offset + 4 + size, length)
            result[code] = int.from_bytes(raw[offset+4:offset+4+size], "big")
            offset += 4 + size
        self.assertEqual(bytes(length-offset), raw[offset:])
        return result

    def identity(self):
        """Return the exact T10 designator emitted by production code."""
        output = (ctypes.c_ubyte * 80)()
        length = self.lib.rdx_manager_build_drive_id_vpd(output, 80)
        return bytes(output[:length])

    def test_date_uses_documented_fallback_and_checked_record(self):
        """Missing record uses the default date; valid manufacturing date wins."""
        self.assertEqual(b"10102010", self.identity()[42:50])
        record = bytearray(256)
        record[4:8] = (6).to_bytes(4, "little")
        record[8:18] = b"0123456789"
        record[18:26] = b"TANDBERG"
        record[26:42] = b"RDX             "
        record[156:164] = b"20122021"
        for lane, seed in enumerate([0x78,0x56,0x34,0x12]):
            record[lane] = (seed + sum(record[4+lane::4])) & 255
        self.lib.seed_identity(bytes(record), 1)
        self.lib.rdx_manager_identity_init()
        self.assertEqual(b"20122021", self.identity()[42:50])
        self.assertEqual(b"0123456789", self.identity()[32:42])
        record[156] ^= 1
        self.lib.seed_identity(bytes(record), 1)
        self.lib.rdx_manager_identity_init()
        self.assertEqual(b"10102010", self.identity()[42:50])

    def test_cam_and_power_follow_live_inputs(self):
        """Neither a hardcoded cam state nor a hardcoded power source is valid."""
        self.assertEqual(1, self.log(0x20)[3])
        self.assertEqual(1, self.log(0x20)[5])
        self.lib.seed_gpio(0, 1, 1)
        self.assertEqual(0, self.log(0x20)[3])
        self.assertEqual(0, self.log(0x20)[5])
        self.assertEqual(1, self.log(0x21)[8])

    def test_missing_counters_are_not_reported_as_zero(self):
        """Unknown cartridge metadata must not become measured zero counters."""
        page = self.log(0x21)
        for code in (1, 5, 6):
            self.assertNotIn(code, page)

    def statistics_sector(self, values):
        """Build illustrative context-4 records using the verified wire schema."""
        sector = bytearray(512)
        offset = 4
        for code, value in values:
            sector[offset:offset+4] = bytes([code, 0, 8, 0])
            sector[offset+4:offset+8] = value.to_bytes(4, "little")
            offset += 8
        return bytes(sector)

    def test_stored_counters_keep_value_width_and_direction(self):
        """LE metadata counters emerge as the corresponding BE LOG parameters."""
        self.lib.seed_statistics(self.statistics_sector([
            (0x0A, 25), (0x0B, 2540), (0x0C, 720),
        ]))
        page = self.log(0x21)
        self.assertEqual(25, page[1])
        self.assertEqual(2540, page[6])
        self.assertEqual(720, page[5])
        self.lib.seed_statistics(self.statistics_sector([
            (0x0A, 0), (0x0B, 0xFFFFFFFF), (0x0C, 0x12345678),
        ]))
        page = self.log(0x21)
        self.assertEqual(0, page[1])
        self.assertEqual(0xFFFFFFFF, page[6])
        self.assertEqual(0x12345678, page[5])

    def test_malformed_duplicate_and_replaced_metadata_stays_unavailable(self):
        """Invalid scalar framing cannot publish invented or previous counters."""
        duplicate = self.statistics_sector([(0x0A, 10), (0x0A, 11)])
        malformed = bytearray(self.statistics_sector([(0x0A, 10)]))
        malformed[6:8] = (509).to_bytes(2, "little")
        for sector in (duplicate, bytes(malformed)):
            with self.subTest(sector=sector[:20].hex()):
                self.lib.seed_statistics(sector)
                self.assertNotIn(1, self.log(0x21))
        self.lib.seed_statistics(self.statistics_sector([(0x0A, 10)]))
        self.assertEqual(10, self.log(0x21)[1])
        self.lib.rdx_reset_media_metadata(0)
        self.assertNotIn(1, self.log(0x21))

    def test_mount_continues_from_identity_to_counter_context(self):
        """Finding identity first must not stop the bounded metadata traversal."""
        root = bytearray(512)
        for offset, code, lba in ((4, 3, 1), (16, 4, 2)):
            root[offset:offset+12] = bytes([code, 1, 12, 0]) + lba.to_bytes(4, "little") + bytes(4)
        identity = bytearray(512)
        offset = 4
        for code, value in ((6, b"IMATION"), (0x31, b"RDX-320"), (5, b"CARTRIDGE01"), (0x35, b"BARCODE")):
            length = 4 + len(value)
            identity[offset:offset+length] = bytes([code, 0]) + length.to_bytes(2, "little") + value
            offset += length
        counters = bytearray(self.statistics_sector([(0x0A, 25), (0x0B, 2540), (0x0C, 720)]))
        # A back-link to the root cannot restart traversal indefinitely.
        counters[28:40] = bytes([6, 1, 12, 0]) + bytes(8)
        for lba, sector in enumerate((root, identity, counters)):
            self.lib.seed_sector(lba, bytes(sector))
        self.assertEqual(3, self.lib.probe_mount_metadata())
        page = self.log(0x21)
        self.assertEqual((25, 2540, 720), (page[1], page[6], page[5]))

    def test_small_destinations_are_not_written(self):
        """Builders reject short output buffers before any byte is changed."""
        for page, minimum in ((0x20, 56), (0x21, 116), (0x0D, 16)):
            output = (ctypes.c_ubyte * 256)(*([0xA5] * 256))
            self.assertEqual(0, self.lib.rdx_manager_build_log_sense(output, minimum-1, 0, page))
            self.assertEqual(bytes([0xA5] * 256), bytes(output))


if __name__ == "__main__":
    unittest.main()
