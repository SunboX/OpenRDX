# SPDX-FileCopyrightText: 2026 Andre Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Source contracts for SATA media classification and retry boundaries."""

from pathlib import Path
import unittest


PROJECT_ROOT = Path(__file__).resolve().parents[1]
SOURCE_ROOT = PROJECT_ROOT / "src" / "rdx_mount"
INCLUDE_ROOT = PROJECT_ROOT / "include" / "rdx_mount"
MEDIA = (SOURCE_ROOT / "sata_media.c").read_text(encoding="utf-8")
RDX_ACCESS = (SOURCE_ROOT / "rdx_unlock.c").read_text(encoding="utf-8")
RDX_ACCESS_HEADER = (INCLUDE_ROOT / "rdx_unlock.h").read_text(encoding="utf-8")
AHCI = (SOURCE_ROOT / "ahci.c").read_text(encoding="utf-8")


def function_body(source: str, signature: str) -> str:
    """Return one C function body using balanced braces."""

    signature_start = source.index(signature)
    body_start = source.index("{", signature_start)
    depth = 0
    for index in range(body_start, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[body_start : index + 1]
    raise AssertionError("unterminated function: {}".format(signature))


def assert_statements_in_order(
    test_case: unittest.TestCase,
    body: str,
    statements: tuple[str, ...],
) -> None:
    """Require each supplied statement to follow the preceding statement."""

    cursor = 0
    for statement in statements:
        position = body.find(statement, cursor)
        test_case.assertNotEqual(
            -1,
            position,
            "missing or out-of-order statement: {}".format(statement),
        )
        cursor = position + len(statement)


class SataMediaClassificationTests(unittest.TestCase):
    """Verify generic and RDX paths remain distinct and fail closed."""

    def test_identify_decodes_4kn_logical_sector_words_low_half_first(self):
        """Decode ATA words 117-118 without reversing their 16-bit halves."""

        save_info = function_body(AHCI, "inline void ahci_save_device_info(")
        assert_statements_in_order(
            self,
            save_info,
            (
                "logical_sector_words =",
                "((UINT32_T)id_info[118] << 16)",
                "(UINT32_T)id_info[117]",
                "logical_sector_words == 0U",
                "logical_sector_words > 0x7FFFFFFFU",
                "dSectorSize = 0U;",
                "logical_sector_words * 2U;",
            ),
        )
        self.assertNotIn("id_info[117] << 16", save_info)

    def test_direct_generic_requires_security_disabled_and_unlocked(self):
        """Reject ambiguous word 128 and either active security-state bit."""

        security = function_body(
            MEDIA,
            "static BOOLEAN_T sata_media_generic_security_is_accessible(",
        )
        self.assertIn("SATA_IDENTIFY_SECURITY_STATUS_WORD", security)
        self.assertIn("SATA_IDENTIFY_WORD_UNAVAILABLE", security)
        self.assertIn("return FALSE;", security)
        self.assertIn("SATA_SECURITY_ENABLED_BIT", security)
        self.assertIn("SATA_SECURITY_LOCKED_BIT", security)
        self.assertRegex(
            security,
            r"\(SATA_SECURITY_ENABLED_BIT\s*\|\s*"
            r"SATA_SECURITY_LOCKED_BIT\)\)\s*==\s*0U",
        )

        generic = function_body(
            MEDIA, "static BOOLEAN_T sata_media_prepare_generic("
        )
        self.assertIn(
            "sata_media_generic_security_is_accessible(device)", generic
        )
        prepare = function_body(MEDIA, "BOOLEAN_T sata_media_prepare(")
        assert_statements_in_order(
            self,
            prepare,
            (
                "inspection != RDX_MEDIA_INSPECTION_NOT_RECOGNIZED",
                "sata_media_generic_security_is_accessible(device)",
                "sata_media_prepare_generic(port_num, device)",
                "SATA_MEDIA_KIND_GENERIC",
            ),
        )

    def test_unreadable_media_gets_one_bounded_rdx_access_fallback(self):
        """Try access once after an unreadable inspection and require success."""

        prepare = function_body(MEDIA, "BOOLEAN_T sata_media_prepare(")
        unreadable = prepare[prepare.index(
            "if (inspection == RDX_MEDIA_INSPECTION_UNREADABLE)"
        ) : prepare.index(
            "if (inspection != RDX_MEDIA_INSPECTION_NOT_RECOGNIZED)"
        )]
        assert_statements_in_order(
            self,
            unreadable,
            (
                "sata_media_try_rdx_access(port_num, device)",
                "return FALSE;",
                "return sata_media_select_rdx(port_num, device);",
            ),
        )
        self.assertEqual(
            1,
            unreadable.count("sata_media_try_rdx_access(port_num, device)"),
        )
        self.assertNotIn("sata_media_generic_security_is_accessible", unreadable)
        access = function_body(
            MEDIA, "static BOOLEAN_T sata_media_try_rdx_access("
        )
        self.assertIn("rdx_unlock_media(port_num, device)", access)

    def test_failed_rdx_access_is_throttled_per_disk_until_disconnect(self):
        """Throttle only completed rejection and clear it on physical link loss."""

        access_enum = RDX_ACCESS_HEADER[
            RDX_ACCESS_HEADER.index("typedef enum _RDX_ACCESS_RESULT_T") :
            RDX_ACCESS_HEADER.index("} RDX_ACCESS_RESULT_T;")
            + len("} RDX_ACCESS_RESULT_T;")
        ]
        assert_statements_in_order(
            self,
            access_enum,
            (
                "RDX_ACCESS_RESULT_READY",
                "RDX_ACCESS_RESULT_REJECTED",
                "RDX_ACCESS_RESULT_RETRYABLE_FAILURE",
            ),
        )

        unlock = function_body(RDX_ACCESS, "RDX_ACCESS_RESULT_T rdx_unlock_media(")
        assert_statements_in_order(
            self,
            unlock,
            (
                "RDX_INITIALIZATION_COMMAND_REJECTED",
                "access_result = RDX_ACCESS_RESULT_REJECTED;",
                "command_result != RDX_INITIALIZATION_COMMAND_SUCCESS",
                "access_result = RDX_ACCESS_RESULT_RETRYABLE_FAILURE;",
                "rdx_finish_media_mount(port_num, device)",
                "access_result = RDX_ACCESS_RESULT_READY;",
            ),
        )
        metadata_failure = unlock[unlock.index(
            "else if (rdx_finish_media_mount(port_num, device))"
        ) :]
        self.assertIn(
            "access_result = RDX_ACCESS_RESULT_RETRYABLE_FAILURE;",
            metadata_failure,
        )

        for state in (
            "sata_media_failed_rdx_fingerprint[NUM_AHCI_PORTS]",
            "sata_media_failed_rdx_attempt[NUM_AHCI_PORTS]",
        ):
            self.assertIn(state, MEDIA)

        fingerprint = function_body(
            MEDIA, "static UINT32_T sata_media_device_fingerprint("
        )
        for identity_field in (
            "device->wModelNum",
            "device->wSerialNum",
            "device->ddTrueMaxLBA",
        ):
            self.assertIn(identity_field, fingerprint)
        self.assertIn("2166136261U", fingerprint)
        self.assertIn("16777619U", fingerprint)

        access = function_body(
            MEDIA, "static BOOLEAN_T sata_media_try_rdx_access("
        )
        assert_statements_in_order(
            self,
            access,
            (
                "sata_media_device_fingerprint(device)",
                "sata_media_failed_rdx_attempt[port_num]",
                "sata_media_failed_rdx_fingerprint[port_num] == fingerprint",
                "return FALSE;",
                "rdx_unlock_media(port_num, device)",
                "access_result == RDX_ACCESS_RESULT_READY",
                "sata_media_failed_rdx_attempt[port_num] = FALSE;",
                "access_result == RDX_ACCESS_RESULT_REJECTED",
                "sata_media_failed_rdx_fingerprint[port_num] = fingerprint;",
                "sata_media_failed_rdx_attempt[port_num] = TRUE;",
            ),
        )
        rejection = access.index(
            "if (access_result == RDX_ACCESS_RESULT_REJECTED)"
        )
        self.assertNotIn(
            "sata_media_failed_rdx_fingerprint[port_num] = fingerprint;",
            access[:rejection],
        )
        self.assertEqual(
            1,
            access.count(
                "sata_media_failed_rdx_fingerprint[port_num] = fingerprint;"
            ),
        )

        policy_reset = function_body(MEDIA, "void sata_media_reset(")
        self.assertNotIn("sata_media_failed_rdx_", policy_reset)
        link_disconnected = function_body(
            MEDIA, "void sata_media_link_disconnected("
        )
        self.assertIn(
            "sata_media_failed_rdx_fingerprint[port_num] = 0U;",
            link_disconnected,
        )
        self.assertIn(
            "sata_media_failed_rdx_attempt[port_num] = FALSE;",
            link_disconnected,
        )
        link_change = function_body(
            AHCI, "static void ahci_handle_media_link_change("
        )
        assert_statements_in_order(
            self,
            link_change,
            (
                "PSSTS_DET_PHY_READY",
                "sata_media_link_disconnected(port_num);",
                "ahci_schedule_media_discovery(port_num, media_was_ready);",
            ),
        )

    def test_recognizable_bad_checksum_is_invalid_not_generic(self):
        """Treat known record structure as RDX evidence even with bad checksum."""

        recognize = function_body(
            RDX_ACCESS,
            "static BOOLEAN_T rdx_metadata_structure_recognized(",
        )
        for marker in (
            "rdx_metadata_pointer_types",
            "RDX_METADATA_TYPE_CONTEXT3",
            "RDX_METADATA_TYPE_LBA32",
            "RDX_METADATA_TYPE_LBA64",
        ):
            self.assertIn(marker, recognize)
        self.assertGreaterEqual(recognize.count("rdx_find_metadata_record("), 4)

        load = function_body(
            RDX_ACCESS, "static BOOLEAN_T rdx_load_metadata_sector("
        )
        self.assertGreaterEqual(
            load.count("rdx_metadata_structure_recognized("), 2
        )
        self.assertGreaterEqual(
            load.count("rdx_metadata_layout_detected[port_num] = TRUE;"), 3
        )

        inspect = function_body(
            RDX_ACCESS, "RDX_MEDIA_INSPECTION_T rdx_inspect_accessible_media("
        )
        assert_statements_in_order(
            self,
            inspect,
            (
                "rdx_metadata_io_failed[port_num]",
                "RDX_MEDIA_INSPECTION_UNREADABLE",
                "rdx_metadata_layout_detected[port_num]",
                "RDX_MEDIA_INSPECTION_INVALID",
                "rdx_metadata_root_readable[port_num]",
                "RDX_MEDIA_INSPECTION_NOT_RECOGNIZED",
            ),
        )


if __name__ == "__main__":
    unittest.main()
