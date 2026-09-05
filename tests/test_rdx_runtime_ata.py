# SPDX-FileCopyrightText: 2026 André Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Source contracts for runtime SMART polling and safe media eject."""

from pathlib import Path
import unittest


PROJECT_DIR = Path(__file__).resolve().parents[1]
UNLOCK_HEADER = (
    PROJECT_DIR / "include" / "rdx_mount" / "rdx_unlock.h"
).read_text(encoding="utf-8")
RUNTIME_HEADER = (
    PROJECT_DIR / "include" / "rdx_mount" / "rdx_runtime_ata.h"
).read_text(encoding="utf-8")
RUNTIME_SOURCE = (
    PROJECT_DIR / "src" / "rdx_mount" / "rdx_runtime_ata.c"
).read_text(encoding="utf-8")


def function_body(source, signature):
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


def assert_statements_in_order(test_case, body, statements):
    """Require each supplied statement to appear after the preceding one."""
    cursor = 0
    for statement in statements:
        position = body.find(statement, cursor)
        test_case.assertNotEqual(
            -1,
            position,
            "missing or out-of-order statement: {}".format(statement),
        )
        cursor = position + len(statement)


class RdxRuntimeAtaTests(unittest.TestCase):
    """Preserve runtime AHCI ownership, temperature, and eject behavior."""

    def test_public_runtime_helpers_are_declared(self):
        """Expose the runtime API without changing the existing unlock API."""
        self.assertIn('#include "rdx_runtime_ata.h"', UNLOCK_HEADER)
        self.assertIn(
            "BOOLEAN_T rdx_read_smart_temperature(UINT32_T port_num,",
            RUNTIME_HEADER,
        )
        self.assertIn(
            "BOOLEAN_T rdx_prepare_media_stop(UINT32_T port_num);",
            RUNTIME_HEADER,
        )
        self.assertIn(
            "BOOLEAN_T rdx_prepare_mechanism_eject(UINT32_T port_num);",
            RUNTIME_HEADER,
        )

    def test_runtime_session_saves_masks_and_restores_only_current_epoch(self):
        """Preserve live PxIE without reviving an invalid media session."""

        session = RUNTIME_SOURCE[
            RUNTIME_SOURCE.index("typedef struct _RDX_RUNTIME_SESSION_T") :
            RUNTIME_SOURCE.index("} RDX_RUNTIME_SESSION_T;")
            + len("} RDX_RUNTIME_SESSION_T;")
        ]
        for field in ("saved_pxie", "media_epoch", "media_kind"):
            self.assertIn(field, session)

        current = function_body(
            RUNTIME_SOURCE,
            "static BOOLEAN_T rdx_runtime_session_is_current(",
        )
        for condition in (
            "ata_dev[port_num].bDeviceInitComplete",
            "session->media_kind != SATA_MEDIA_KIND_NONE",
            "sata_media_get_kind(port_num) == session->media_kind",
            "sata_media_get_epoch(port_num) == session->media_epoch",
        ):
            self.assertIn(condition, current)

        begin = function_body(
            RUNTIME_SOURCE,
            "static BOOLEAN_T rdx_runtime_begin_sync_commands(",
        )
        self.assertIn("!ata_dev[port_num].bDeviceInitComplete", begin)
        self.assertGreaterEqual(begin.count("READ32(PxCI(port_num))"), 2)
        self.assertGreaterEqual(begin.count("READ32(PxSACT(port_num))"), 2)
        assert_statements_in_order(
            self,
            begin,
            (
                "session->media_kind = sata_media_get_kind(port_num);",
                "session->media_epoch = sata_media_get_epoch(port_num);",
                "rdx_runtime_lock_current_session(port_num, session)",
                "session->saved_pxie = READ32(PxIE(port_num));",
                "session->saved_pxie & ~RDX_RUNTIME_OWNED_INTERRUPT_STATUS",
                "rdx_runtime_session_is_current(port_num, session)",
            ),
        )
        stale_lock = function_body(
            RUNTIME_SOURCE,
            "static BOOLEAN_T rdx_runtime_lock_current_session(",
        )
        assert_statements_in_order(
            self,
            stale_lock,
            (
                "rdx_runtime_enter_critical_section();",
                "!rdx_runtime_session_is_current(port_num, session)",
                "WRITE32(PxIE(port_num), 0U);",
                "rdx_runtime_leave_critical_section(FALSE);",
                "return FALSE;",
            ),
        )
        finish = function_body(
            RUNTIME_SOURCE,
            "static BOOLEAN_T rdx_runtime_finish_sync_commands(",
        )
        assert_statements_in_order(
            self,
            finish,
            (
                "rdx_runtime_lock_current_session(port_num, session)",
                "return FALSE;",
                "rdx_runtime_unlock_current_session(port_num, session);",
            ),
        )
        restore = function_body(
            RUNTIME_SOURCE,
            "static void rdx_runtime_unlock_current_session(",
        )
        self.assertIn(
            "WRITE32(PxIE(port_num), session->saved_pxie);", restore
        )

    def test_smart_read_uses_required_fis_and_attribute_layout(self):
        """Read one PIO sector and prefer C2h over the BEh fallback."""
        body = function_body(
            RUNTIME_SOURCE,
            "BOOLEAN_T rdx_read_smart_temperature(UINT32_T port_num,",
        )
        for statement in (
            "ata_cmd.fis.command = RDX_ATA_SMART;",
            "ata_cmd.fis.features = RDX_ATA_SMART_READ_DATA;",
            "ata_cmd.fis.LBA_mid = RDX_ATA_SMART_LBA_MID;",
            "ata_cmd.fis.LBA_high = RDX_ATA_SMART_LBA_HIGH;",
            "ata_cmd.dDataByteCnt = RDX_RUNTIME_DATA_BYTES;",
            "attribute_index < RDX_SMART_ATTRIBUTE_COUNT",
            "attribute += RDX_SMART_ATTRIBUTE_SIZE",
            "attribute[RDX_SMART_RAW_VALUE_OFFSET]",
        ):
            self.assertIn(statement, body)
        self.assertLess(
            body.index("attribute[0] == RDX_SMART_TEMPERATURE_ID"),
            body.index("attribute[0] == RDX_SMART_AIRFLOW_TEMP_ID"),
        )
        self.assertLess(
            body.index("*temperature_celsius = 0xFFU;"),
            body.index("rdx_runtime_begin_sync_commands"),
        )
        for definition in (
            "#define RDX_ATA_SMART_READ_DATA             0xD0U",
            "#define RDX_SMART_ATTRIBUTE_OFFSET             2U",
            "#define RDX_SMART_ATTRIBUTE_SIZE              12U",
            "#define RDX_SMART_ATTRIBUTE_COUNT             30U",
            "#define RDX_SMART_RAW_VALUE_OFFSET             5U",
        ):
            self.assertIn(definition, RUNTIME_SOURCE)

    def test_stop_flushes_then_enters_standby_with_one_pxie_session(self):
        """Select the LBA48 flush opcode and stop after either command fails."""
        body = function_body(
            RUNTIME_SOURCE,
            "BOOLEAN_T rdx_prepare_media_stop(UINT32_T port_num)",
        )
        assert_statements_in_order(
            self,
            body,
            (
                "rdx_runtime_begin_sync_commands(port_num, &session)",
                "ata_dev[port_num].bLBA48 ?",
                "ATA_CMD_FLUSH_CACHE_EXT : ATA_CMD_FLUSH_CACHE;",
                "if (successful)",
                "ata_cmd.fis.command = ATA_CMD_STANDBY_IMMEDIATE;",
                "rdx_runtime_finish_sync_commands(port_num, &session)",
            ),
        )
        self.assertEqual(1, body.count("rdx_runtime_begin_sync_commands"))
        self.assertEqual(2, body.count("rdx_runtime_execute_sync_command"))
        self.assertEqual(1, body.count("rdx_runtime_finish_sync_commands"))

    def test_mechanism_eject_issues_standby_without_flush(self):
        """Keep accepted mechanism eject separate from the STOP sequence."""
        body = function_body(
            RUNTIME_SOURCE,
            "BOOLEAN_T rdx_prepare_mechanism_eject(UINT32_T port_num)",
        )
        self.assertIn("ATA_CMD_STANDBY_IMMEDIATE", body)
        self.assertNotIn("ATA_CMD_FLUSH_CACHE", body)
        self.assertEqual(1, body.count("rdx_runtime_execute_sync_command"))
        self.assertEqual(2, body.count(
            "WRITE32(PxIS(port_num), RDX_NONDATA_COMPLETION_STATUS);"
        ))
        self.assertEqual(1, body.count("rdx_runtime_finish_sync_commands"))

    def test_runtime_commands_acknowledge_completion_before_and_after(self):
        """Keep polled completion/TFES ownership away from the fatal ISR."""
        # The foreground port runs with the TI ISR live, where TFES is a fatal
        # bit, so the completion masks below are part of the safety contract.
        self.assertIn(
            "#define RDX_RUNTIME_COMMAND_ERROR_STATUS TASK_FILE_ERROR_STATUS",
            RUNTIME_SOURCE,
        )
        self.assertRegex(
            RUNTIME_SOURCE,
            r"#define RDX_PIO_COMPLETION_STATUS\s+\\\n"
            r"\s*\(PIO_SETUP_FIS_INTR \| D2H_REGISTER_FIS_INTR \|\s*\\\n"
            r"\s*RDX_RUNTIME_COMMAND_ERROR_STATUS\)",
        )
        self.assertRegex(
            RUNTIME_SOURCE,
            r"#define RDX_NONDATA_COMPLETION_STATUS\s+\\\n"
            r"\s*\(D2H_REGISTER_FIS_INTR \| "
            r"RDX_RUNTIME_COMMAND_ERROR_STATUS\)",
        )
        smart_body = function_body(
            RUNTIME_SOURCE,
            "BOOLEAN_T rdx_read_smart_temperature(UINT32_T port_num,",
        )
        self.assertEqual(
            2,
            smart_body.count(
                "WRITE32(PxIS(port_num), RDX_PIO_COMPLETION_STATUS);"
            ),
        )
        eject_body = function_body(
            RUNTIME_SOURCE,
            "BOOLEAN_T rdx_prepare_media_stop(UINT32_T port_num)",
        )
        self.assertEqual(
            4,
            eject_body.count(
                "WRITE32(PxIS(port_num), RDX_NONDATA_COMPLETION_STATUS);"
            ),
        )
        mechanism_body = function_body(
            RUNTIME_SOURCE,
            "BOOLEAN_T rdx_prepare_mechanism_eject(UINT32_T port_num)",
        )
        self.assertEqual(
            2,
            mechanism_body.count(
                "WRITE32(PxIS(port_num), RDX_NONDATA_COMPLETION_STATUS);"
            ),
        )
        self.assertGreaterEqual(
            RUNTIME_SOURCE.count("rdx_runtime_execute_sync_command("), 5
        )

    def test_runtime_command_wait_is_bracketed_by_epoch_checks(self):
        """Check ownership before command issue and again after the wait."""

        execute = function_body(
            RUNTIME_SOURCE,
            "static BOOLEAN_T rdx_runtime_execute_sync_command(",
        )
        assert_statements_in_order(
            self,
            execute,
            (
                "rdx_runtime_lock_current_session(port_num, session)",
                "ahci_build_cmd(port_num, ata_cmd, 0U)",
                "ahci_issue_cmd(port_num, 0U, FALSE)",
                "rdx_runtime_leave_critical_section(TRUE);",
                "ahci_wait_complete(PxCI(port_num)",
                "rdx_runtime_lock_current_session(port_num, session)",
                "PTFD_STS_FAILURE_MASK",
                "rdx_runtime_leave_critical_section(TRUE);",
            ),
        )

    def test_runtime_commands_reject_error_and_device_fault_status(self):
        """Treat either ATA failure bit as a failed synchronous command."""
        execute = function_body(
            RUNTIME_SOURCE,
            "static BOOLEAN_T rdx_runtime_execute_sync_command(",
        )

        self.assertIn("PTFD_STS_FAILURE_MASK", execute)
        self.assertNotIn("ATA_ERROR_STATUS_BIT", execute)


if __name__ == "__main__":
    unittest.main()
