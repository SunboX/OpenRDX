# SPDX-FileCopyrightText: 2026 Andre Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Structural contracts for SATA epochs, DMA limits, and hot-plug safety."""

from pathlib import Path
import re
import unittest


PROJECT_ROOT = Path(__file__).resolve().parents[1]
SOURCE_ROOT = PROJECT_ROOT / "src" / "rdx_mount"
INCLUDE_ROOT = PROJECT_ROOT / "include" / "rdx_mount"
AHCI = (SOURCE_ROOT / "ahci.c").read_text(encoding="utf-8")
SCSI = (SOURCE_ROOT / "scsi.c").read_text(encoding="utf-8")
MEDIA = (SOURCE_ROOT / "sata_media.c").read_text(encoding="utf-8")
MEDIA_HEADER = (INCLUDE_ROOT / "sata_media.h").read_text(encoding="utf-8")
MWW_HEADER = (INCLUDE_ROOT / "mww.h").read_text(encoding="utf-8")
TUSB_HEADER = (INCLUDE_ROOT / "tusb9260.h").read_text(encoding="utf-8")


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


class SataEpochAndTransferSafetyTests(unittest.TestCase):
    """Verify invalidation tokens, transfer bounds, and callback convergence."""

    def test_media_epoch_api_advances_at_every_policy_reset(self):
        """Expose a volatile per-port token and advance it with invalidation."""

        self.assertRegex(
            MEDIA_HEADER,
            r"UINT32_T\s+sata_media_get_epoch\(UINT32_T\s+port_num\);",
        )
        self.assertRegex(
            MEDIA,
            r"static\s+volatile\s+UINT32_T\s+"
            r"sata_media_epoch\[NUM_AHCI_PORTS\];",
        )
        reset = function_body(MEDIA, "void sata_media_reset(")
        assert_statements_in_order(
            self,
            reset,
            (
                "if (port_num >= NUM_AHCI_PORTS)",
                "sata_media_kind[port_num] = SATA_MEDIA_KIND_NONE;",
                "sata_media_epoch[port_num]++;",
                "sata_media_smart_available[port_num] = FALSE;",
                "rdx_reset_media_context(port_num);",
            ),
        )
        getter = function_body(MEDIA, "UINT32_T sata_media_get_epoch(")
        assert_statements_in_order(
            self,
            getter,
            (
                "if (port_num >= NUM_AHCI_PORTS)",
                "return 0U;",
                "return sata_media_epoch[port_num];",
            ),
        )
        prepare = function_body(MEDIA, "BOOLEAN_T sata_media_prepare(")
        self.assertLess(
            prepare.index("sata_media_reset(port_num);"),
            prepare.index("rdx_inspect_accessible_media(port_num, device)"),
        )
        schedule = function_body(
            AHCI, "static void ahci_schedule_media_discovery("
        )
        assert_statements_in_order(
            self,
            schedule,
            (
                "bDeviceInitComplete = FALSE;",
                "sata_media_reset(port_num);",
                "WRITE32(PxIE(port_num), 0U);",
                "ahci_hotplug_pending[port_num] = TRUE;",
            ),
        )

    def test_block_limits_scale_for_512_and_4kn_and_gate_rw(self):
        """Advertise and enforce the same PRDT byte ceiling per sector size."""

        self.assertRegex(
            TUSB_HEADER, r"#define\s+AHCI_MAX_SCAT_GATH\s+8\b"
        )
        self.assertRegex(
            MWW_HEADER,
            r"#define\s+MWW_VIRTUAL_WINDOW_SIZE\s+0x0003E000\b",
        )
        maximum_bytes = 8 * 0x0003E000
        self.assertEqual(3968, maximum_bytes // 512)
        self.assertEqual(496, maximum_bytes // 4096)

        limit = function_body(SCSI, "static UINT32_T scsi_get_max_rw_blocks(")
        self.assertIn("SCSI_AHCI_MAX_TRANSFER_BYTES / sector_size", limit)
        self.assertIn("sector_size == 0U", limit)
        self.assertIn("SCSI_LBA28_MAX_TRANSFER_BLOCKS", limit)

        vpd = function_body(SCSI, "inline void scsi_build_block_limits_vpd_page(")
        assert_statements_in_order(
            self,
            vpd,
            (
                "max_transfer_blocks = scsi_get_max_rw_blocks(",
                "ata_dev[scsi_cmd.pCmdInput->bLUN].dSectorSize",
                "scsi_resp_buff[8]",
                "scsi_resp_buff[15]",
            ),
        )

        rw = function_body(SCSI, "inline STATUS_T scsi_handle_rw_cmd(")
        assert_statements_in_order(
            self,
            rw,
            (
                "status = scsi_get_rw_params();",
                "status = scsi_validate_LBA_range();",
                "scsi_cmd.dXferLength > scsi_get_max_rw_blocks(",
                "ata_dev[scsi_cmd.pCmdInput->bLUN].dTrueSectorSize",
                "status = STATUS_SCSI_INVALID_CMD_FIELD;",
                "scsi_cmd.dXferLength *",
                "scsi_build_ata_rw_cmd(&ata_cmd)",
            ),
        )

    def test_prdt_oversize_fails_before_descriptors_or_header(self):
        """Reject an unrepresentable byte count instead of truncating DMA."""

        setup = function_body(AHCI, "inline STATUS_T ahci_setup_PRDT(")
        assert_statements_in_order(
            self,
            setup,
            (
                "*prdt_entry_count = 0U;",
                "maximum_byte_cnt = ata_cmd->bUseMemoryWrapWindow ?",
                "AHCI_MAX_SCAT_GATH",
                "sizeof(datapath_ram->normal_data_buffer)",
                "if (ata_cmd->dDataByteCnt > maximum_byte_cnt)",
                "return STATUS_ERROR;",
                "remaining_byte_cnt = ata_cmd->dDataByteCnt;",
                "while (remaining_byte_cnt > 0U)",
            ),
        )
        build = function_body(AHCI, "STATUS_T ahci_build_cmd(")
        assert_statements_in_order(
            self,
            build,
            (
                "status = ahci_setup_PRDT(",
                "if (status == STATUS_OK)",
                "ahci_create_cmd_header(",
            ),
        )

    def test_single_port_isr_rereads_pxie_each_loop_iteration(self):
        """Apply the live interrupt mask to every newly latched status read."""

        isr = function_body(AHCI, "void ahci_isr(")
        loop_start = isr.index("do\n")
        loop_end = isr.index("} while (port_intr);", loop_start)
        loop = isr[loop_start:loop_end]
        assert_statements_in_order(
            self,
            loop,
            (
                "IE_mask = READ32(PxIE(0));",
                "WRITE32(PxIS(0), port_intr);",
                "ahci_port_intr_handler(0, (port_intr & IE_mask));",
                "port_intr = READ32(PxIS(0));",
            ),
        )
        self.assertEqual(1, loop.count("IE_mask = READ32(PxIE(0));"))

    def test_final_admission_rechecks_epoch_link_and_connect_change(self):
        """Validate the admitted target again before publishing readiness."""

        init_port = function_body(AHCI, "STATUS_T ahci_init_port(")
        admission = init_port.index(
            "sata_media_prepare(port_num, &ata_dev[port_num])"
        )
        publish = init_port.index(
            "ata_dev[port_num].bDeviceInitComplete = TRUE;", admission
        )
        final_path = init_port[admission:publish]
        assert_statements_in_order(
            self,
            final_path,
            (
                "discovery_epoch = sata_media_get_epoch(port_num);",
                "0xFFFFFFFFU & ~PORT_CONNECT_CHANGE_STATUS",
                "rdx_hardware_prepare_media_ready(port_num);",
                "sata_media_get_epoch(port_num) != discovery_epoch",
                "READ32(PxSSTS(port_num)) & PSSTS_DET_MASK",
                "PSSTS_DET_PHY_READY",
                "READ32(PxIS(port_num))",
                "PORT_CONNECT_CHANGE_STATUS",
                "ahci_schedule_media_discovery(port_num, FALSE);",
                "status = STATUS_ERROR;",
            ),
        )
        self.assertNotIn("bDeviceInitComplete = TRUE;", final_path)
        ready_path = init_port[publish:]
        assert_statements_in_order(
            self,
            ready_path,
            (
                "bDeviceInitComplete = TRUE;",
                "WRITE32(PxIE(port_num), PORT_DEFAULT_INTR_ENABLE);",
                "if (status == STATUS_OK)",
                "WRITE_REG32(VIM_REQMASKSET0, AHCI_SATA_INTERRUPT_MASK);",
                "WRITE_REG32(VIM_REQMASKSET0, AHCI_USB_INTERRUPT_MASK);",
            ),
        )

    def test_terminal_callbacks_are_coalesced_without_skipping_readmission(self):
        """Schedule every invalidation while queueing at most one error callback."""

        pending = function_body(AHCI, "static BOOLEAN_T ahci_callbacks_are_pending(")
        self.assertIn("callback_index < ATA_CALLBACK_QUEUE_DEPTH", pending)
        self.assertIn("callback_pending[callback_index]", pending)

        link_change = function_body(
            AHCI, "static void ahci_handle_media_link_change("
        )
        assert_statements_in_order(
            self,
            link_change,
            (
                "ahci_schedule_media_discovery(port_num, media_was_ready);",
                "media_was_ready &&",
                "!ahci_callbacks_are_pending(port_num)",
                "ahci_ata_cbk_queue_add(",
            ),
        )

        fatal = function_body(AHCI, "void ahci_fatal_error_recovery(")
        assert_statements_in_order(
            self,
            fatal,
            (
                "if (port_was_reset && media_was_ready)",
                "ahci_schedule_media_discovery(port_num, TRUE);",
                "media_was_ready &&",
                "!ahci_callbacks_are_pending(port_num)",
                "ahci_ata_cbk_queue_add(port_num",
            ),
        )

        rx_error = function_body(AHCI, "void ahci_rx_error_isr(")
        schedule = re.search(
            r"if\s*\(media_was_ready\)\s*\{\s*"
            r"ahci_schedule_media_discovery\(0U, TRUE\);",
            rx_error,
            re.DOTALL,
        )
        self.assertIsNotNone(schedule)
        queue = re.search(
            r"if\s*\(media_was_ready\s*&&\s*"
            r"!ahci_callbacks_are_pending\(0U\)\)\s*\{.*?"
            r"ahci_ata_cbk_queue_add\(",
            rx_error,
            re.DOTALL,
        )
        self.assertIsNotNone(queue)
        self.assertLess(schedule.start(), queue.start())

        service = function_body(AHCI, "void ahci_service(")
        assert_statements_in_order(
            self,
            service,
            (
                "if (ahci_reinit_wait_for_callbacks[port_num])",
                "if (ahci_callbacks_are_pending(port_num))",
                "continue;",
                "if (!ahci_hotplug_quiesced[port_num])",
                "ahci_stop(port_num);",
                "status = ahci_init_port(port_num);",
            ),
        )


if __name__ == "__main__":
    unittest.main()
