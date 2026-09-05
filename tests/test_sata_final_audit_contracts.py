# SPDX-FileCopyrightText: 2026 André Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Structural contracts for final SATA command and recovery safety."""

from pathlib import Path
import re
import unittest


PROJECT_ROOT = Path(__file__).resolve().parents[1]
SOURCE_ROOT = PROJECT_ROOT / "src" / "rdx_mount"
INCLUDE_ROOT = PROJECT_ROOT / "include" / "rdx_mount"
AHCI = (SOURCE_ROOT / "ahci.c").read_text(encoding="utf-8")
RUNTIME = (SOURCE_ROOT / "rdx_runtime_ata.c").read_text(encoding="utf-8")
SCSI = (SOURCE_ROOT / "scsi.c").read_text(encoding="utf-8")
SCSI_HEADER = (INCLUDE_ROOT / "scsi.h").read_text(encoding="utf-8")
ALL_FIRMWARE_C = "\n".join(
    path.read_text(encoding="utf-8")
    for path in sorted(SOURCE_ROOT.glob("*.c"))
)


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


class SataFinalAuditContractTests(unittest.TestCase):
    """Verify command ownership across faults, timeouts, and media changes."""

    def test_device_fault_latch_clears_only_in_quiesced_discovery(self):
        """Discard a prior DF only after the port is idle and before IDENTIFY."""

        clears = re.findall(r"bDeviceFault\s*=\s*FALSE\s*;", ALL_FIRMWARE_C)
        self.assertEqual(1, len(clears))

        init_port = function_body(AHCI, "STATUS_T ahci_init_port(")
        assert_statements_in_order(
            self,
            init_port,
            (
                "ata_dev[port_num].bDeviceInitComplete = FALSE;",
                "sata_media_reset(port_num);",
                "WRITE32(PxIE(port_num), 0);",
                "status = ahci_stop(port_num);",
                "READ32(PxCI(port_num)) | READ32(PxSACT(port_num))",
                "status = ahci_port_reset(port_num);",
                "if (status == STATUS_OK)",
                "ata_dev[port_num].bDeviceFault = FALSE;",
                "ahci_start(port_num);",
                "status = ahci_classify_device(port_num);",
                "status = ahci_identify_device(port_num);",
            ),
        )
        clear_position = init_port.index(
            "ata_dev[port_num].bDeviceFault = FALSE;"
        )
        first_build = init_port.find("ahci_build_cmd(")
        self.assertTrue(first_build == -1 or clear_position < first_build)

    def test_local_recovery_masks_completion_and_defers_readmission(self):
        """Recover a locally owned slot without creating a second completion."""

        recover = function_body(AHCI, "void ahci_recover_local_command(")
        assert_statements_in_order(
            self,
            recover,
            (
                "WRITE_REG32(VIM_REQMASKCLR0, AHCI_SATA_INTERRUPT_MASK);",
                "WRITE32(PxIE(port_num), 0U);",
                "if (reset_port)",
                "mww_force_sata_interface_ready();",
                "(void)ahci_stop(port_num);",
                "(void)ahci_port_reset(port_num);",
                "ahci_schedule_media_discovery(port_num, FALSE);",
                "WRITE_REG32(VIM_REQMASKSET0, AHCI_CONTROLLER_INTERRUPT_MASK);",
            ),
        )
        self.assertNotIn("ahci_ata_cbk_queue_add", recover)
        self.assertNotIn("AHCI_SATA_RX_ERROR_INTERRUPT_MASK", recover)

    def test_polled_timeouts_recover_before_any_interrupt_restoration(self):
        """Keep completion masked while active timed-out slots are retired."""

        runtime = function_body(
            RUNTIME, "static BOOLEAN_T rdx_runtime_execute_sync_command("
        )
        assert_statements_in_order(
            self,
            runtime,
            (
                "status = ahci_wait_complete(",
                "if ((status != STATUS_OK)",
                "READ32(PxCI(port_num)) & 0x01U",
                "ahci_recover_local_command(port_num, TRUE);",
                "return FALSE;",
                "rdx_runtime_lock_current_session(port_num, session)",
            ),
        )
        timeout_start = runtime.index("if ((status != STATUS_OK)")
        timeout_end = runtime.index(
            "if (!rdx_runtime_lock_current_session", timeout_start
        )
        timeout_path = runtime[timeout_start:timeout_end]
        self.assertNotIn("WRITE32(PxIE", timeout_path)
        self.assertNotIn("VIM_REQMASKSET0", timeout_path)

        intermediate = function_body(
            SCSI, "static STATUS_T scsi_finish_intermediate_ata_command("
        )
        assert_statements_in_order(
            self,
            intermediate,
            (
                "status = ahci_wait_complete(",
                "if ((status != STATUS_OK)",
                "READ32(PxCI(lun))",
                "ahci_recover_local_command(lun, TRUE);",
                "return STATUS_SCSI_LOGICAL_UNIT_NOT_READY;",
                "WRITE_REG32(VIM_REQMASKCLR0, SCSI_CONTROLLER_INTERRUPT_MASK);",
                "media_is_current = scsi_intermediate_session_is_current(",
                "MODIFY32(PxIE(lun), SCSI_INTERMEDIATE_COMPLETION_STATUS",
            ),
        )
        timeout_start = intermediate.index("if ((status != STATUS_OK)")
        timeout_end = intermediate.index(
            "WRITE_REG32(VIM_REQMASKCLR0", timeout_start
        )
        timeout_path = intermediate[timeout_start:timeout_end]
        self.assertNotIn("MODIFY32(PxIE", timeout_path)
        self.assertNotIn("VIM_REQMASKSET0", timeout_path)

    def test_scsi_submission_is_atomic_for_one_media_epoch(self):
        """Mask both SATA sources across final validation, build, and issue."""

        self.assertRegex(
            SCSI,
            r"#define\s+SCSI_CONTROLLER_INTERRUPT_MASK\s*\\\s*\n"
            r"\s*\(SCSI_SATA_RX_ERROR_INTERRUPT_MASK\s*\|\s*"
            r"SCSI_AHCI_INTERRUPT_MASK\)",
        )
        self.assertRegex(
            SCSI_HEADER,
            r"UINT32_T\s+dMediaEpoch\s*;",
        )

        current = function_body(
            SCSI, "static BOOLEAN_T scsi_submission_media_is_current("
        )
        for condition in (
            "ata_dev[lun].bDeviceInitComplete",
            "sata_media_get_kind(lun) != SATA_MEDIA_KIND_NONE",
            "sata_media_get_epoch(lun) == scsi_cmd.dMediaEpoch",
            "READ32(PxSSTS(lun)) & PSSTS_DET_MASK",
            "PSSTS_DET_PHY_READY",
            "READ32(PxIS(lun)) & PORT_CONNECT_CHANGE_STATUS",
        ):
            self.assertIn(condition, current)

        handler = function_body(SCSI, "STATUS_T scsi_command_handler(")
        assert_statements_in_order(
            self,
            handler,
            (
                "scsi_cmd.pCmdInput = cmd_input;",
                "scsi_cmd.dMediaEpoch = sata_media_get_epoch(cmd_input->bLUN);",
                "cmd_input->pCommandBlock[0]",
            ),
        )

        submit = function_body(SCSI, "STATUS_T scsi_send_ata_cmd(")
        mask = submit.index(
            "WRITE_REG32(VIM_REQMASKCLR0, SCSI_CONTROLLER_INTERRUPT_MASK);"
        )
        build = submit.index("status = ahci_build_cmd(", mask)
        first_check = submit.index(
            "if (!scsi_submission_media_is_current(lun))", mask
        )
        second_check = submit.index(
            "if (!scsi_submission_media_is_current(lun))", first_check + 1
        )
        issue = submit.index("status = ahci_issue_cmd(", second_check)
        final_check = submit.index(
            "if (!recovery_started && !scsi_submission_media_is_current(lun))",
            issue,
        )
        restore_guard = submit.index("if (!recovery_started)", final_check + 1)
        restore = submit.index(
            "WRITE_REG32(VIM_REQMASKSET0, SCSI_CONTROLLER_INTERRUPT_MASK);",
            restore_guard,
        )
        self.assertLess(mask, first_check)
        self.assertLess(first_check, build)
        self.assertLess(build, second_check)
        self.assertLess(second_check, issue)
        self.assertLess(issue, final_check)
        self.assertLess(final_check, restore_guard)
        self.assertLess(restore_guard, restore)
        self.assertIn(
            "sata_media_get_epoch(lun) == scsi_cmd.dMediaEpoch", current
        )
        self.assertIn(
            "READ32(PxCI(lun))", submit[final_check:restore_guard]
        )
        self.assertEqual(
            1,
            submit.count(
                "WRITE_REG32(VIM_REQMASKSET0, SCSI_CONTROLLER_INTERRUPT_MASK);"
            ),
        )

    def test_pass_through_policy_is_media_specific_and_fail_closed(self):
        """Expose no raw cartridge LBAs and restrict locked generic media."""

        allowed = function_body(
            SCSI, "static BOOLEAN_T scsi_ata_pass_through_is_allowed("
        )
        rdx_start = allowed.index("if (media_kind == SATA_MEDIA_KIND_RDX)")
        generic_start = allowed.index(
            "if (media_kind != SATA_MEDIA_KIND_GENERIC)", rdx_start
        )
        rdx_policy = allowed[rdx_start:generic_start]
        for helper in (
            "scsi_ata_pass_through_is_identify_command",
            "scsi_ata_pass_through_is_smart_read_command",
            "scsi_ata_pass_through_has_no_data",
        ):
            self.assertIn(helper, rdx_policy)
        self.assertIn("ATA_CMD_CHECK_POWER_MODE", rdx_policy)
        self.assertNotIn("scsi_ata_pass_through_is_read_only_command", rdx_policy)
        for raw_command in (
            "SCSI_ATA_READ_SECTORS",
            "SCSI_ATA_READ_MULTIPLE",
            "ATA_CMD_READ_DMA",
            "ATA_CMD_READ_VERIFY_SECTORS",
        ):
            self.assertNotIn(raw_command, rdx_policy)

        assert_statements_in_order(
            self,
            allowed,
            (
                "if (media_kind == SATA_MEDIA_KIND_RDX)",
                "if (media_kind != SATA_MEDIA_KIND_GENERIC)",
                "return FALSE;",
                "if (!rdx_hardware_is_write_protected())",
                "return TRUE;",
                "return scsi_ata_pass_through_is_read_only_command(",
            ),
        )

        read_only = function_body(
            SCSI,
            "static BOOLEAN_T scsi_ata_pass_through_is_read_only_command(",
        )
        for read_command in (
            "ATA_CMD_CHECK_POWER_MODE",
            "ATA_CMD_READ_VERIFY_SECTORS",
            "ATA_CMD_READ_VERIFY_SECTORS_EXT",
            "SCSI_ATA_READ_NATIVE_MAX",
            "SCSI_ATA_READ_NATIVE_MAX_EXT",
            "SCSI_ATA_READ_SECTORS",
            "SCSI_ATA_READ_SECTORS_EXT",
            "SCSI_ATA_READ_MULTIPLE",
            "SCSI_ATA_READ_MULTIPLE_EXT",
            "ATA_CMD_READ_DMA",
            "ATA_CMD_READ_DMA_EXT",
        ):
            self.assertIn(read_command, read_only)
        for disallowed_command in (
            "ATA_CMD_WRITE_DMA",
            "ATA_CMD_WRITE_SECTORS",
            "ATA_CMD_DATA_SET_MGMT",
            "ATA_CMD_SET_FEATURES",
            "ATA_CMD_READ_FPDMA_QUEUED",
            "ATA_CMD_READ_LOG_EXT",
        ):
            self.assertNotIn(disallowed_command, read_only)
        self.assertIn("default:", read_only)
        self.assertIn("return FALSE;", read_only)

    def test_pass_through_read_shapes_and_completion_owner_are_bounded(self):
        """Validate read lengths, direction, lock rejection, and local resets."""

        data_count = function_body(
            SCSI,
            "static BOOLEAN_T "
            "scsi_ata_pass_through_data_count_matches_sector_count(",
        )
        for constraint in (
            "T_LENGTH_SECTOR_CNT",
            "ATA_PASS_THROUGH_BYTE_BLOCK_BIT",
            "sector_count == 0U",
            "sector_size == 0U",
            "sector_count > (0xFFFFFFFFU / sector_size)",
            "ata_cmd->dDataByteCnt == (sector_count * sector_size)",
        ):
            self.assertIn(constraint, data_count)

        smart = function_body(
            SCSI, "static BOOLEAN_T scsi_ata_pass_through_is_smart_read_command("
        )
        for constraint in (
            "SCSI_ATA_SMART_LBA_MID",
            "SCSI_ATA_SMART_LBA_HIGH",
            "ata_cmd->fis.sector_cnt == 1U",
            "ata_cmd->dDataByteCnt == 0x200U",
        ):
            self.assertIn(constraint, smart)

        builder = function_body(
            SCSI, "inline void scsi_build_ata_pass_through_cmd("
        )
        assert_statements_in_order(
            self,
            builder,
            (
                "ata_cmd->dDataByteCnt",
                "ata_cmd->bIsWriteCmd =",
                "ata_cmd->dDataByteCnt != 0U",
                "ATA_PASS_THROUGH_T_DIR_BIT) == 0U",
            ),
        )

        handler = function_body(
            SCSI, "STATUS_T scsi_handle_ata_pass_through_cmd("
        )
        reset_start = handler.index("if ((protocol == ATA_HW_RESET_CODE)")
        send_start = handler.index("else if ((protocol >=", reset_start)
        reset_path = handler[reset_start:send_start]
        self.assertEqual(1, reset_path.count("ahci_recover_local_command("))
        self.assertIn("scsi_cmd.pCmdInput->bLUN, TRUE", reset_path)
        self.assertIn("status = STATUS_OK;", reset_path)
        self.assertNotIn("scsi_send_ata_cmd", reset_path)
        self.assertNotIn("ahci_ata_cbk_queue_add", reset_path)

        allowed = handler.index("scsi_ata_pass_through_is_allowed(", send_start)
        send = handler.index("status = scsi_send_ata_cmd(&ata_cmd);", allowed)
        lock_rejection = handler.index(
            "rdx_hardware_is_write_protected()", send
        )
        write_protected = handler.index(
            "status = STATUS_SCSI_WRITE_PROTECTED;", lock_rejection
        )
        invalid = handler.index("status = STATUS_SCSI_INVALID_CMD;", write_protected)
        self.assertLess(allowed, send)
        self.assertLess(send, lock_rejection)
        self.assertLess(lock_rejection, write_protected)
        self.assertLess(write_protected, invalid)


if __name__ == "__main__":
    unittest.main()
