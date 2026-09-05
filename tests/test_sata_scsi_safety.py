# SPDX-FileCopyrightText: 2026 André Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Source contracts for fail-closed SATA SCSI command construction."""

from pathlib import Path
import re
import unittest


PROJECT_ROOT = Path(__file__).resolve().parents[1]
SOURCE_ROOT = PROJECT_ROOT / "src" / "rdx_mount"
INCLUDE_ROOT = PROJECT_ROOT / "include" / "rdx_mount"
SCSI = (SOURCE_ROOT / "scsi.c").read_text(encoding="utf-8")
MEDIA = (SOURCE_ROOT / "sata_media.c").read_text(encoding="utf-8")
MEDIA_HEADER = (INCLUDE_ROOT / "sata_media.h").read_text(encoding="utf-8")


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


class SataScsiSafetyTests(unittest.TestCase):
    """Verify mapping revocation and destructive-command validation."""

    def test_lba_translation_api_and_builders_fail_closed(self):
        """Require an admitted layout and an explicit translated-LBA output."""

        self.assertRegex(
            MEDIA_HEADER,
            r"BOOLEAN_T\s+sata_media_translate_lba\(\s*"
            r"UINT32_T\s+port_num,\s*UINT64_T\s+logical_lba,\s*"
            r"UINT64_T\s*\*physical_lba\s*\);",
        )
        translate = function_body(
            MEDIA, "BOOLEAN_T sata_media_translate_lba("
        )
        assert_statements_in_order(
            self,
            translate,
            (
                "physical_lba == NULL",
                "return FALSE;",
                "SATA_MEDIA_KIND_RDX",
                "*physical_lba = rdx_translate_media_lba",
                "SATA_MEDIA_KIND_GENERIC",
                "*physical_lba = logical_lba;",
                "*physical_lba = 0U;",
                "return FALSE;",
            ),
        )

        for signature in (
            "inline BOOLEAN_T scsi_build_ata_rw_cmd(",
            "BOOLEAN_T scsi_build_ata_read_verify_sectors_cmd(",
        ):
            builder = function_body(SCSI, signature)
            translation = builder.index("sata_media_translate_lba(")
            failure = builder.index("return FALSE;", translation)
            fis_lba = builder.index("ata_cmd->fis.LBA_low", failure)
            self.assertIn("&physical_lba", builder[translation:failure])
            self.assertLess(translation, failure)
            self.assertLess(failure, fis_lba)
            self.assertIn("return TRUE;", builder[fis_lba:])

    def test_every_lba_builder_call_site_checks_failure(self):
        """Do not submit READ, WRITE, or VERIFY after mapping revocation."""

        calls = (
            "scsi_build_ata_rw_cmd(&ata_cmd)",
            "scsi_build_ata_read_verify_sectors_cmd(&ata_cmd)",
        )
        for call in calls:
            checked_calls = re.findall(
                r"if\s*\([^{{}};]*{}".format(re.escape(call)),
                SCSI,
            )
            self.assertGreater(SCSI.count(call), 0)
            self.assertEqual(SCSI.count(call), len(checked_calls))

        for signature, call, expected_calls in (
            ("inline STATUS_T scsi_handle_rw_cmd(", calls[0], 1),
            ("inline STATUS_T scsi_handle_verify_cmd(", calls[1], 1),
            (
                "inline STATUS_T scsi_handle_start_stop_unit_cmd(",
                calls[1],
                2,
            ),
            ("static STATUS_T scsi_run_diagnostic_verify(", calls[1], 1),
        ):
            handler = function_body(SCSI, signature)
            self.assertEqual(expected_calls, handler.count(call))
            self.assertIn("STATUS_SCSI_LOGICAL_UNIT_NOT_READY", handler)

        diagnostic = function_body(
            SCSI, "inline STATUS_T scsi_handle_send_diagnostic_cmd("
        )
        self.assertEqual(3, diagnostic.count("scsi_run_diagnostic_verify("))
        self.assertNotIn("MODIFY32(PxIE", diagnostic)

    def test_unmap_lengths_are_validated_before_buffer_use(self):
        """Reject truncated, oversized, and misaligned UNMAP parameter data."""

        unmap = function_body(SCSI, "inline STATUS_T scsi_handle_unmap_cmd(")
        for name, value in (
            ("UNMAP_HEADER_SIZE", "8U"),
            ("UNMAP_DESCRIPTOR_SIZE", "16U"),
        ):
            self.assertRegex(
                SCSI,
                r"#define\s+{}\s+{}\b".format(name, value),
            )

        list_zero = unmap.index("if (param_list_len == 0U)")
        list_bounds = unmap.index("param_list_len < UNMAP_HEADER_SIZE")
        copy = unmap.index("ti_memcpy(unmap_param_list")
        self.assertLess(list_zero, list_bounds)
        self.assertLess(list_bounds, copy)
        self.assertIn("param_list_len > MAX_UNMAP_DESC_SIZE", unmap)
        self.assertIn(
            "scsi_cmd.pCmdInput->dDataXferLength < param_list_len",
            unmap,
        )

        descriptor_decode = unmap.index("descriptor_data_len =")
        descriptor_alignment = unmap.index(
            "descriptor_data_len % UNMAP_DESCRIPTOR_SIZE"
        )
        descriptor_count = unmap.index(
            "num_unmap_block_desc = descriptor_data_len"
        )
        self.assertLess(copy, descriptor_decode)
        self.assertLess(descriptor_decode, descriptor_alignment)
        self.assertLess(descriptor_alignment, descriptor_count)
        self.assertIn(
            "descriptor_data_len > (param_list_len - UNMAP_HEADER_SIZE)",
            unmap,
        )
        self.assertIn(
            "scsi_cmd.pCmdInput->dDataXferLength - UNMAP_HEADER_SIZE",
            unmap,
        )

    def test_unmap_rejects_each_invalid_range_before_encoding(self):
        """Use subtraction-based overflow and capacity checks per descriptor."""

        unmap = function_body(SCSI, "inline STATUS_T scsi_handle_unmap_cmd(")
        descriptor_loop = unmap[unmap.index("while (num_unmap_block_desc--") :]
        lba_decode = descriptor_loop.index("lba =")
        count_decode = descriptor_loop.index("num_blocks =", lba_decode)
        capacity = descriptor_loop.index("lba >= ata_dev[lun].ddMaxLBA")
        remaining = descriptor_loop.index(
            "ata_dev[lun].ddMaxLBA - lba", capacity
        )
        rejection = descriptor_loop.index(
            "return STATUS_SCSI_INVALID_ADDRESS_RANGE;", remaining
        )
        encode = descriptor_loop.index("while (num_blocks != 0U)", rejection)
        self.assertLess(lba_decode, count_decode)
        self.assertLess(count_decode, capacity)
        self.assertLess(capacity, remaining)
        self.assertLess(remaining, rejection)
        self.assertLess(rejection, encode)
        self.assertNotIn("lba + num_blocks", descriptor_loop[:encode])

    def test_unmap_uses_disjoint_input_and_dsm_scratch_regions(self):
        """Preserve descriptors while ATA DSM entries are generated below them."""

        self.assertRegex(
            SCSI,
            r"#define\s+MAX_UNMAP_DESC_SIZE\s+\(2\s*\*\s*1024\)",
        )
        unmap = function_body(SCSI, "inline STATUS_T scsi_handle_unmap_cmd(")
        input_region = unmap.index(
            "sizeof(datapath_ram->normal_data_buffer) - MAX_UNMAP_DESC_SIZE"
        )
        preserve = unmap.index("ti_memcpy(unmap_param_list")
        dsm_region = unmap.index(
            "dsm_data = (UINT8_T*)datapath_ram->normal_data_buffer;"
        )
        lower_bound = unmap.index(
            "sizeof(datapath_ram->normal_data_buffer) -",
            dsm_region,
        )
        clear = unmap.index("ti_memset(dsm_data", lower_bound)
        self.assertLess(input_region, preserve)
        self.assertLess(preserve, dsm_region)
        self.assertLess(dsm_region, lower_bound)
        self.assertLess(lower_bound, clear)
        self.assertIn("MAX_UNMAP_DESC_SIZE) / DSM_BLOCK_SIZE", unmap)

    def test_unmap_skips_empty_dsm_and_rejects_failed_completion(self):
        """Never issue a zero-block DSM command and detect ATA DF or ERR."""

        unmap = function_body(SCSI, "inline STATUS_T scsi_handle_unmap_cmd(")
        no_descriptors = unmap.index("if (num_unmap_block_desc == 0U)")
        dsm_setup = unmap.index("dsm_data =", no_descriptors)
        self.assertIn("return STATUS_OK;", unmap[no_descriptors:dsm_setup])
        self.assertNotIn(
            "scsi_send_trim_data", unmap[no_descriptors:dsm_setup]
        )

        no_entries = unmap.index("if (dsm_entry_count == 0U)")
        count = unmap.index("dsm_data_block_cnt =", no_entries)
        final_send = unmap.index(
            "scsi_send_trim_data(dsm_data_block_cnt, FALSE)", count
        )
        self.assertIn("return STATUS_OK;", unmap[no_entries:count])
        self.assertLess(no_entries, count)
        self.assertLess(count, final_send)

        send = function_body(SCSI, "static STATUS_T scsi_send_trim_data(")
        zero = send.index("if (dsm_data_block_cnt == 0U)")
        command = send.index("ata_cmd.fis.command = ATA_CMD_DATA_SET_MGMT")
        begin = send.index("scsi_begin_intermediate_ata_command", command)
        submit = send.index("status = scsi_send_ata_cmd(&ata_cmd);", begin)
        finish = send.index("scsi_finish_intermediate_ata_command", submit)
        self.assertIn("return STATUS_OK;", send[zero:command])
        self.assertLess(zero, command)
        self.assertLess(command, begin)
        self.assertLess(begin, submit)
        self.assertLess(submit, finish)
        self.assertIn(
            "ata_cmd.dDataByteCnt = dsm_data_block_cnt * DSM_BLOCK_SIZE;",
            send,
        )

    def test_intermediate_commands_gate_masks_and_df_err_by_media_epoch(self):
        """Keep polled command completion from reviving stale interrupts."""

        session = SCSI[
            SCSI.index("typedef struct _SCSI_INTERMEDIATE_SESSION_T") :
            SCSI.index("} SCSI_INTERMEDIATE_SESSION_T;")
            + len("} SCSI_INTERMEDIATE_SESSION_T;")
        ]
        self.assertIn("UINT32_T media_epoch;", session)
        self.assertIn("SATA_MEDIA_KIND_T media_kind;", session)

        current = function_body(
            SCSI, "static BOOLEAN_T scsi_intermediate_session_is_current("
        )
        for condition in (
            "ata_dev[lun].bDeviceInitComplete",
            "session->media_kind != SATA_MEDIA_KIND_NONE",
            "sata_media_get_kind(lun) == session->media_kind",
            "sata_media_get_epoch(lun) == session->media_epoch",
        ):
            self.assertIn(condition, current)

        begin = function_body(
            SCSI, "static BOOLEAN_T scsi_begin_intermediate_ata_command("
        )
        assert_statements_in_order(
            self,
            begin,
            (
                "session->media_kind = sata_media_get_kind(lun);",
                "session->media_epoch = sata_media_get_epoch(lun);",
                "WRITE_REG32(VIM_REQMASKCLR0, SCSI_CONTROLLER_INTERRUPT_MASK);",
                "!scsi_intermediate_session_is_current(lun, session)",
                "WRITE32(PxIE(lun), 0U);",
                "return FALSE;",
                "WRITE32(PxIS(lun), SCSI_INTERMEDIATE_COMPLETION_STATUS);",
                "MODIFY32(PxIE(lun), SCSI_INTERMEDIATE_COMPLETION_STATUS, 0U);",
            ),
        )

        finish = function_body(
            SCSI, "static STATUS_T scsi_finish_intermediate_ata_command("
        )
        assert_statements_in_order(
            self,
            finish,
            (
                "ahci_wait_complete(",
                "WRITE_REG32(VIM_REQMASKCLR0, SCSI_CONTROLLER_INTERRUPT_MASK);",
                "media_is_current = scsi_intermediate_session_is_current",
                "PTFD_STS_FAILURE_MASK",
                "status = STATUS_SCSI_INTERNAL_TARGET_FAILURE;",
                "WRITE32(PxIS(lun), SCSI_INTERMEDIATE_COMPLETION_STATUS);",
                "if (media_is_current)",
                "MODIFY32(PxIE(lun), SCSI_INTERMEDIATE_COMPLETION_STATUS",
                "WRITE32(PxIE(lun), 0U);",
                "status = STATUS_SCSI_LOGICAL_UNIT_NOT_READY;",
            ),
        )
        self.assertIn(
            "D2H_REGISTER_FIS_INTR | TASK_FILE_ERROR_STATUS",
            SCSI,
        )
        self.assertIn("SCSI_SATA_RX_ERROR_INTERRUPT_MASK", finish)

        for signature in (
            "inline STATUS_T scsi_handle_start_stop_unit_cmd(",
            "inline STATUS_T scsi_handle_mode_select_cmd(",
            "static STATUS_T scsi_send_trim_data(",
            "static STATUS_T scsi_run_diagnostic_verify(",
        ):
            body = function_body(SCSI, signature)
            self.assertIn("scsi_begin_intermediate_ata_command", body)
            self.assertIn("scsi_finish_intermediate_ata_command", body)


if __name__ == "__main__":
    unittest.main()
