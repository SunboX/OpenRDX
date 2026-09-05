# SPDX-FileCopyrightText: 2026 André Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Source contracts for dual RDX and generic SATA media admission."""

from pathlib import Path
import re
import unittest


PROJECT_ROOT = Path(__file__).resolve().parents[1]
SOURCE_ROOT = PROJECT_ROOT / "src" / "rdx_mount"
INCLUDE_ROOT = PROJECT_ROOT / "include" / "rdx_mount"
AHCI = (SOURCE_ROOT / "ahci.c").read_text(encoding="utf-8")
AHCI_HEADER = (INCLUDE_ROOT / "ahci.h").read_text(encoding="utf-8")
HARDWARE = (SOURCE_ROOT / "rdx_hardware.c").read_text(encoding="utf-8")
MAIN = (SOURCE_ROOT / "main.c").read_text(encoding="utf-8")
MEDIA = (SOURCE_ROOT / "sata_media.c").read_text(encoding="utf-8")
MEDIA_HEADER = (INCLUDE_ROOT / "sata_media.h").read_text(encoding="utf-8")
RDX_ACCESS = (SOURCE_ROOT / "rdx_unlock.c").read_text(encoding="utf-8")
SCSI = (SOURCE_ROOT / "scsi.c").read_text(encoding="utf-8")
USB_CHAPTER9 = (SOURCE_ROOT / "usb_chap9.c").read_text(encoding="utf-8")


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


class SataMediaPolicyTests(unittest.TestCase):
    """Lock the safety and compatibility boundaries of dual-media support."""

    def test_puis_wakeup_handles_both_identify_signatures(self):
        """Issue SET FEATURES 07h and refresh incomplete IDENTIFY data."""

        for name, value in (
            ("SATA_IDENTIFY_SPECIFIC_CONFIGURATION_WORD", "2U"),
            ("SATA_PUIS_IDENTIFY_INCOMPLETE", "0x37C8U"),
            ("SATA_PUIS_IDENTIFY_COMPLETE", "0x738CU"),
            ("SATA_SET_FEATURES_PUIS_SPIN_UP", "0x07U"),
            ("SATA_MEDIA_COMMAND_TIMEOUT_MS", "30000"),
        ):
            self.assertRegex(
                MEDIA,
                r"#define\s+{}\s+{}\b".format(name, value),
            )
        self.assertRegex(
            AHCI_HEADER,
            r"#define\s+ATA_CMD_SET_FEATURES\s+0xEF\b",
        )

        wake = function_body(
            MEDIA, "BOOLEAN_T sata_media_start_after_identify("
        )
        self.assertIn("configuration != SATA_PUIS_IDENTIFY_INCOMPLETE", wake)
        self.assertIn("configuration != SATA_PUIS_IDENTIFY_COMPLETE", wake)
        assert_statements_in_order(
            self,
            wake,
            (
                "ti_memset(&ata_cmd",
                "ata_cmd.fis.command = ATA_CMD_SET_FEATURES;",
                "ata_cmd.fis.features = SATA_SET_FEATURES_PUIS_SPIN_UP;",
                "sata_media_execute_sync_command(port_num, &ata_cmd)",
            ),
        )
        incomplete = wake.index(
            "if (configuration == SATA_PUIS_IDENTIFY_INCOMPLETE)"
        )
        self.assertIn("ahci_identify_device(port_num)", wake[incomplete:])

    def test_puis_wakeup_precedes_transfer_mode_selection(self):
        """Wake after IDENTIFY and before either transfer-mode command."""

        init_port = function_body(AHCI, "STATUS_T ahci_init_port(")
        assert_statements_in_order(
            self,
            init_port,
            (
                "status = ahci_identify_device(port_num);",
                "sata_media_start_after_identify(",
                "status = ahci_set_features_xfer_mode(port_num, FALSE);",
                "status = ahci_set_features_xfer_mode(port_num, TRUE);",
                "sata_media_prepare(port_num, &ata_dev[port_num])",
                "ata_dev[port_num].bDeviceInitComplete = TRUE;",
            ),
        )

    def test_identify_validates_active_port_completion_before_parsing(self):
        """Clear stale data and accept IDENTIFY only after port-local success."""

        identify = function_body(AHCI, "STATUS_T ahci_identify_device(")
        buffer_clear = identify.index(
            "ti_memset((void *)datapath_ram->normal_data_buffer, 0U, 0x200U);"
        )
        build = identify.index("ahci_build_cmd(port_num, &ata_cmd, 0)")
        wait = identify.index(
            "ahci_wait_complete(PxCI(port_num), 0x1, 0x0, 7000)"
        )
        task_file = identify.index(
            "READ32(PxTFD(port_num)) & PTFD_STS_FAILURE_MASK"
        )
        parse = identify.index("ahci_save_device_info(port_num);")
        self.assertLess(buffer_clear, build)
        self.assertLess(build, wait)
        self.assertLess(wait, task_file)
        self.assertLess(task_file, parse)
        self.assertIn("status = STATUS_ERROR;", identify[task_file:parse])
        self.assertNotIn("PxCI(0)", identify)
        self.assertIn(
            "if (ata_dev[port_num].bDeviceInitComplete)",
            identify,
        )
        self.assertGreaterEqual(
            identify.count("MODIFY32(PxIE(port_num), PIO_SETUP_FIS_INTR"),
            2,
        )

    def test_set_features_helpers_validate_port_local_task_status(self):
        """Reject DF or ERR after each port-scoped SET FEATURES command."""

        for signature in (
            "\nSTATUS_T ahci_set_features_xfer_mode("
            "UINT32_T port_num, BOOLEAN_T dma)\n{",
            "\nSTATUS_T ahci_set_features_dma_auto_activate("
            "UINT32_T port_num, BOOLEAN_T enable)\n{",
        ):
            helper = function_body(AHCI, signature)
            wait = helper.index(
                "ahci_wait_complete(PxCI(port_num), 0x1, 0x0, 2000)"
            )
            task_file = helper.index(
                "READ32(PxTFD(port_num)) & PTFD_STS_FAILURE_MASK"
            )
            rejected = helper.index("status = STATUS_ERROR;", task_file)
            self.assertLess(wait, task_file)
            self.assertLess(task_file, rejected)
            self.assertNotIn("PxCI(0)", helper)

    def test_staggered_spinup_is_requested_before_link_negotiation(self):
        """Set PxCMD.SUD before speed selection and the PHY-ready wait."""

        self.assertRegex(
            AHCI_HEADER,
            r"#define\s+PCMD_SUD_BIT\s+0x00000002\b",
        )
        init_port = function_body(AHCI, "STATUS_T ahci_init_port(")
        assert_statements_in_order(
            self,
            init_port,
            (
                "WRITE32(PxCMD(port_num), PCMD_HPCP_BIT | PCMD_SUD_BIT);",
                "ahci_set_port_speed(port_num",
                "PSSTS_DET_PHY_READY",
            ),
        )
        hba_retry = init_port[init_port.index("ahci_hba_reset();") :]
        assert_statements_in_order(
            self,
            hba_retry,
            (
                "ahci_hba_reset();",
                "WRITE32(PxCMD(port_num), PCMD_HPCP_BIT | PCMD_SUD_BIT);",
                "ahci_set_port_speed(port_num, 2);",
            ),
        )

    def test_each_link_starts_with_an_explicit_media_kind_reset(self):
        """Clear both dispatcher state and cached RDX layout on discovery."""

        enum = re.search(
            r"typedef\s+enum(?:\s+_[A-Za-z0-9_]+)?\s*"
            r"\{(?P<body>.*?)\}\s*SATA_MEDIA_KIND_T;",
            MEDIA_HEADER,
            re.DOTALL,
        )
        self.assertIsNotNone(enum)
        assert_statements_in_order(
            self,
            enum.group("body"),
            (
                "SATA_MEDIA_KIND_NONE",
                "SATA_MEDIA_KIND_RDX",
                "SATA_MEDIA_KIND_GENERIC",
            ),
        )
        self.assertRegex(
            MEDIA,
            r"static\s+volatile\s+SATA_MEDIA_KIND_T\s+"
            r"sata_media_kind\[NUM_AHCI_PORTS\];",
        )

        reset = function_body(MEDIA, "void sata_media_reset(")
        assert_statements_in_order(
            self,
            reset,
            (
                "sata_media_kind[port_num] = SATA_MEDIA_KIND_NONE;",
                "rdx_reset_media_context(port_num);",
            ),
        )
        init_port = function_body(AHCI, "STATUS_T ahci_init_port(")
        self.assertLess(
            init_port.index("sata_media_reset(port_num);"),
            init_port.index("status = ahci_classify_device(port_num);"),
        )

        prepare = function_body(MEDIA, "BOOLEAN_T sata_media_prepare(")
        select_rdx = function_body(
            MEDIA, "static BOOLEAN_T sata_media_select_rdx("
        )
        self.assertIn(
            "sata_media_kind[port_num] = SATA_MEDIA_KIND_RDX;",
            select_rdx,
        )
        self.assertGreaterEqual(
            prepare.count("sata_media_select_rdx(port_num, device);"),
            2,
        )
        self.assertIn(
            "sata_media_kind[port_num] = SATA_MEDIA_KIND_GENERIC;",
            prepare,
        )

    def test_security_state_separates_locked_rdx_and_generic_media(self):
        """Interpret locked media broadly while keeping direct access strict."""

        admission = function_body(
            MEDIA, "static BOOLEAN_T sata_media_prepare_generic("
        )
        for required in (
            "device->bPacketDevice",
            "sata_media_generic_security_is_accessible(device)",
            "device->ddTrueMaxLBA == 0U",
            "device->dTrueSectorSize == 0U",
            "sata_media_verify_generic_access(port_num, device)",
        ):
            self.assertIn(required, admission)
        locked = function_body(
            MEDIA, "static BOOLEAN_T sata_media_security_is_locked("
        )
        self.assertIn("SATA_IDENTIFY_SECURITY_STATUS_WORD", locked)
        self.assertIn("security_status == SATA_IDENTIFY_WORD_UNAVAILABLE", locked)
        self.assertIn("return FALSE;", locked)
        self.assertIn("security_status & SATA_SECURITY_LOCKED_BIT", locked)
        self.assertNotIn("SATA_SECURITY_SUPPORTED_BIT", MEDIA)

        generic_security = function_body(
            MEDIA, "static BOOLEAN_T sata_media_generic_security_is_accessible("
        )
        self.assertIn(
            "security_status == SATA_IDENTIFY_WORD_UNAVAILABLE",
            generic_security,
        )
        self.assertIn("return FALSE;", generic_security)
        self.assertIn("SATA_SECURITY_ENABLED_BIT", generic_security)
        self.assertIn("SATA_SECURITY_LOCKED_BIT", generic_security)
        self.assertRegex(
            generic_security,
            r"security_status\s*&\s*\(SATA_SECURITY_ENABLED_BIT\s*\|\s*"
            r"SATA_SECURITY_LOCKED_BIT\)\)\s*==\s*0U",
        )
        self.assertNotIn("bRemovableMediaDevice", admission)
        self.assertNotIn("bModelNumber", admission)
        self.assertLess(
            admission.index("sata_media_verify_generic_access(port_num, device)"),
            admission.index("device->ddMaxLBA = device->ddTrueMaxLBA;"),
        )

    def test_generic_admission_verifies_sector_zero_without_data_transfer(self):
        """Prove direct sector-zero access with a one-sector READ VERIFY."""

        verify = function_body(
            MEDIA, "static BOOLEAN_T sata_media_verify_generic_access("
        )
        assert_statements_in_order(
            self,
            verify,
            (
                "ti_memset(&ata_cmd, 0U, sizeof(ata_cmd));",
                "ATA_CMD_READ_VERIFY_SECTORS_EXT : ATA_CMD_READ_VERIFY_SECTORS",
                "ata_cmd.fis.device = SATA_LBA_MODE_BIT;",
                "ata_cmd.fis.sector_cnt = 1U;",
                "sata_media_execute_sync_command(port_num, &ata_cmd)",
            ),
        )
        for data_field in ("pData", "dDataByteCnt", "bDirection"):
            self.assertNotIn(data_field, verify)
        for lba_field in (
            "LBA_low =",
            "LBA_mid =",
            "LBA_high =",
            "LBA_low_exp =",
            "LBA_mid_exp =",
            "LBA_high_exp =",
        ):
            self.assertNotIn(lba_field, verify)

    def test_generic_smart_policy_uses_identify_state_and_per_port_tracking(self):
        """Enable SMART only when needed and publish its measured availability."""

        for name, value in (
            ("SATA_IDENTIFY_COMMAND_SET_SUPPORT_WORD", "82U"),
            ("SATA_IDENTIFY_COMMAND_SET_ACTIVE_WORD", "85U"),
            ("SATA_SMART_FEATURE_BIT", "0x0001U"),
        ):
            self.assertRegex(
                MEDIA,
                r"#define\s+{}\s+{}\b".format(name, value),
            )
        self.assertRegex(
            MEDIA,
            r"static\s+BOOLEAN_T\s+"
            r"sata_media_smart_available\[NUM_AHCI_PORTS\];",
        )
        self.assertIn(
            "BOOLEAN_T sata_media_smart_is_available(UINT32_T port_num);",
            MEDIA_HEADER,
        )

        smart = function_body(
            MEDIA, "static BOOLEAN_T sata_media_enable_smart("
        )
        assert_statements_in_order(
            self,
            smart,
            (
                "SATA_IDENTIFY_COMMAND_SET_SUPPORT_WORD",
                "SATA_IDENTIFY_COMMAND_SET_ACTIVE_WORD",
                "support == SATA_IDENTIFY_WORD_UNAVAILABLE",
                "support & SATA_SMART_FEATURE_BIT",
                "active != SATA_IDENTIFY_WORD_UNAVAILABLE",
                "active & SATA_SMART_FEATURE_BIT",
                "ata_cmd.fis.command = SATA_ATA_SMART;",
                "ata_cmd.fis.features = SATA_ATA_SMART_ENABLE;",
            ),
        )
        self.assertGreaterEqual(
            smart.count("sata_media_smart_available[port_num] = TRUE;"),
            2,
        )
        self.assertGreaterEqual(
            smart.count("sata_media_smart_available[port_num] = FALSE;"),
            2,
        )

        reset = function_body(MEDIA, "void sata_media_reset(")
        self.assertIn(
            "sata_media_smart_available[port_num] = FALSE;",
            reset,
        )
        query = function_body(
            MEDIA, "BOOLEAN_T sata_media_smart_is_available("
        )
        self.assertIn("if (port_num >= NUM_AHCI_PORTS)", query)
        self.assertIn("return FALSE;", query)
        self.assertIn("return sata_media_smart_available[port_num];", query)

    def test_generic_smart_transport_failure_blocks_media_readiness(self):
        """Keep the LUN unavailable when the SMART transaction still owns ATA."""

        execute = function_body(
            MEDIA, "static SATA_MEDIA_COMMAND_RESULT_T sata_media_execute_sync_command("
        )
        self.assertGreaterEqual(
            execute.count("return SATA_MEDIA_COMMAND_TRANSPORT_FAILURE;"),
            3,
        )
        self.assertIn("return SATA_MEDIA_COMMAND_REJECTED;", execute)
        self.assertIn("return SATA_MEDIA_COMMAND_SUCCESS;", execute)
        rdx_execute = function_body(
            RDX_ACCESS,
            "rdx_execute_initialization_sync_command(",
        )
        self.assertIn(
            "READ32(PxTFD(port_num)) & PTFD_STS_FAILURE_MASK",
            execute,
        )
        self.assertIn(
            "READ32(PxTFD(port_num)) & PTFD_STS_FAILURE_MASK",
            rdx_execute,
        )
        self.assertRegex(
            AHCI_HEADER,
            r"#define\s+PTFD_STS_FAILURE_MASK\s+"
            r"\(PTFD_STS_DF_BIT\s*\|\s*PTFD_STS_ERR_BIT\)",
        )

        smart = function_body(
            MEDIA, "static BOOLEAN_T sata_media_enable_smart("
        )
        default_path = smart[smart.index("default:") :]
        assert_statements_in_order(
            self,
            default_path,
            (
                "sata_media_smart_available[port_num] = FALSE;",
                "return FALSE;",
            ),
        )
        generic = function_body(
            MEDIA, "static BOOLEAN_T sata_media_prepare_generic("
        )
        self.assertIn(
            "return sata_media_enable_smart(port_num, device);",
            generic,
        )
        prepare = function_body(MEDIA, "BOOLEAN_T sata_media_prepare(")
        generic_call = prepare.index("sata_media_prepare_generic(port_num, device)")
        generic_kind = prepare.index(
            "sata_media_kind[port_num] = SATA_MEDIA_KIND_GENERIC;",
            generic_call,
        )
        self.assertIn("return FALSE;", prepare[generic_call:generic_kind])
        init_port = function_body(AHCI, "STATUS_T ahci_init_port(")
        self.assertLess(
            init_port.index("sata_media_prepare(port_num, &ata_dev[port_num])"),
            init_port.index("ata_dev[port_num].bDeviceInitComplete = TRUE;"),
        )

    def test_temperature_polling_skips_media_without_smart(self):
        """Avoid runtime ATA SMART commands when admission marked it unavailable."""

        temperature = function_body(
            HARDWARE, "static void rdx_hardware_service_temperature("
        )
        gate = temperature.index("sata_media_smart_is_available(0U)")
        read = temperature.index("rdx_read_smart_temperature(0U, &temperature)")
        self.assertLess(gate, read)
        unavailable_path = temperature[gate:read]
        self.assertIn(
            "rdx_hardware.temperature_celsius = RDX_TEMPERATURE_UNAVAILABLE;",
            unavailable_path,
        )
        self.assertIn("RDX_TEMPERATURE_SAMPLE_MS", unavailable_path)
        self.assertIn("return;", unavailable_path)

    def test_generic_branch_never_issues_the_rdx_access_command(self):
        """Keep direct admission free of the bounded RDX access operation."""

        generic = function_body(
            MEDIA, "static BOOLEAN_T sata_media_prepare_generic("
        )
        self.assertNotIn("rdx_unlock_media", generic)
        self.assertNotIn("rdx_issue_security_unlock", generic)
        self.assertNotIn("ATA_CMD_SECURITY_UNLOCK", generic)
        self.assertNotIn("0xF2", generic)

        prepare = function_body(MEDIA, "BOOLEAN_T sata_media_prepare(")
        locked = prepare.index("sata_media_security_is_locked(device)")
        unlock = prepare.index("sata_media_try_rdx_access(port_num, device)", locked)
        inspect = prepare.index("rdx_inspect_accessible_media", unlock)
        fallback = prepare.index(
            "inspection == RDX_MEDIA_INSPECTION_UNREADABLE", inspect
        )
        fallback_unlock = prepare.index(
            "sata_media_try_rdx_access(port_num, device)", fallback
        )
        fail_closed = prepare.index(
            "inspection != RDX_MEDIA_INSPECTION_NOT_RECOGNIZED",
            fallback_unlock,
        )
        security = prepare.index(
            "sata_media_generic_security_is_accessible(device)", fail_closed
        )
        generic_admission = prepare.index("sata_media_prepare_generic", security)
        self.assertLess(locked, unlock)
        self.assertLess(unlock, inspect)
        self.assertLess(inspect, fallback)
        self.assertLess(fallback, fallback_unlock)
        self.assertLess(fallback_unlock, fail_closed)
        self.assertLess(fail_closed, security)
        self.assertLess(security, generic_admission)
        self.assertLess(fallback_unlock, generic_admission)

    def test_invalid_rdx_metadata_cannot_fall_through_to_direct_lbas(self):
        """Fail closed once a recognizable RDX layout proves inconsistent."""

        prepare = function_body(MEDIA, "BOOLEAN_T sata_media_prepare(")
        inspection = prepare.index("rdx_inspect_accessible_media")
        ready = prepare.index("RDX_MEDIA_INSPECTION_READY", inspection)
        fail_closed = prepare.index(
            "inspection != RDX_MEDIA_INSPECTION_NOT_RECOGNIZED", ready
        )
        generic = prepare.index("sata_media_prepare_generic", fail_closed)
        self.assertLess(inspection, ready)
        self.assertLess(ready, fail_closed)
        self.assertLess(fail_closed, generic)
        self.assertIn("return FALSE;", prepare[fail_closed:generic])

        inspect = function_body(
            RDX_ACCESS, "RDX_MEDIA_INSPECTION_T rdx_inspect_accessible_media("
        )
        self.assertIn("rdx_metadata_layout_detected[port_num]", inspect)
        self.assertIn("return RDX_MEDIA_INSPECTION_INVALID;", inspect)
        self.assertIn("rdx_metadata_root_readable[port_num]", inspect)
        self.assertIn("return RDX_MEDIA_INSPECTION_NOT_RECOGNIZED;", inspect)

    def test_metadata_transport_failure_precedes_layout_fallback(self):
        """Classify failed or still-owned reads before any direct-LBA fallback."""

        load = function_body(
            RDX_ACCESS, "static BOOLEAN_T rdx_load_metadata_sector("
        )
        failure_markers = [
            match.start()
            for match in re.finditer(
                re.escape("rdx_metadata_io_failed[port_num] = TRUE;"),
                load,
            )
        ]
        self.assertEqual(2, len(failure_markers))
        for marker in failure_markers:
            guarded_failure = load[marker : marker + 220]
            self.assertIn("READ32(PxCI(port_num)) & 0x01U", guarded_failure)
            self.assertIn("return FALSE;", guarded_failure)

        inspect = function_body(
            RDX_ACCESS, "RDX_MEDIA_INSPECTION_T rdx_inspect_accessible_media("
        )
        assert_statements_in_order(
            self,
            inspect,
            (
                "rdx_finish_media_mount(port_num, device)",
                "rdx_metadata_io_failed[port_num]",
                "return RDX_MEDIA_INSPECTION_UNREADABLE;",
                "rdx_metadata_layout_detected[port_num]",
                "rdx_metadata_root_readable[port_num]",
            ),
        )

    def test_accessible_rdx_media_is_committed_without_a_second_f2(self):
        """Commit validated metadata through the common RDX selection gate."""

        inspect = function_body(
            RDX_ACCESS, "RDX_MEDIA_INSPECTION_T rdx_inspect_accessible_media("
        )
        self.assertIn("rdx_finish_media_mount(port_num, device)", inspect)
        finish = function_body(
            RDX_ACCESS, "static BOOLEAN_T rdx_finish_media_mount("
        )
        assert_statements_in_order(
            self,
            finish,
            (
                "rdx_load_media_extent(port_num, device)",
                "rdx_load_media_identity(port_num, device)",
            ),
        )

        select_rdx = function_body(
            MEDIA, "static BOOLEAN_T sata_media_select_rdx("
        )
        assert_statements_in_order(
            self,
            select_rdx,
            (
                "device->bTRIMSupport = FALSE;",
                "sata_media_enable_smart(port_num, device)",
                "return FALSE;",
                "sata_media_kind[port_num] = SATA_MEDIA_KIND_RDX;",
                "return TRUE;",
            ),
        )

        prepare = function_body(MEDIA, "BOOLEAN_T sata_media_prepare(")
        ready = prepare.index("RDX_MEDIA_INSPECTION_READY")
        fail_closed = prepare.index(
            "inspection != RDX_MEDIA_INSPECTION_NOT_RECOGNIZED", ready
        )
        ready_path = prepare[ready:fail_closed]
        self.assertIn(
            "return sata_media_select_rdx(port_num, device);",
            ready_path,
        )
        self.assertNotIn("rdx_unlock_media", ready_path)

    def test_ambiguous_security_cannot_bypass_missing_or_damaged_metadata(self):
        """Bound unreadable fallback and reject ambiguous direct admission."""

        prepare = function_body(MEDIA, "BOOLEAN_T sata_media_prepare(")
        inspection = prepare.index("rdx_inspect_accessible_media")
        fallback = prepare.index(
            "inspection == RDX_MEDIA_INSPECTION_UNREADABLE", inspection
        )
        fallback_access = prepare.index(
            "sata_media_try_rdx_access(port_num, device)", fallback
        )
        fail_closed = prepare.index(
            "inspection != RDX_MEDIA_INSPECTION_NOT_RECOGNIZED", inspection
        )
        security = prepare.index(
            "sata_media_generic_security_is_accessible(device)", fail_closed
        )
        generic = prepare.index("sata_media_prepare_generic", security)
        unreadable_path = prepare[fallback:fail_closed]
        self.assertEqual(
            1,
            unreadable_path.count(
                "sata_media_try_rdx_access(port_num, device)"
            ),
        )
        self.assertIn("return FALSE;", prepare[fallback_access:fail_closed])
        self.assertIn("return FALSE;", prepare[fail_closed:security])
        self.assertIn("return FALSE;", prepare[security:generic])
        self.assertLess(inspection, fallback)
        self.assertLess(fallback, fallback_access)
        self.assertLess(fallback_access, fail_closed)
        self.assertLess(fail_closed, security)
        self.assertLess(security, generic)

    def test_selected_layout_controls_capacity_and_lba_translation(self):
        """Map RDX LBAs through its extent and generic LBAs directly."""

        generic = function_body(
            MEDIA, "static BOOLEAN_T sata_media_prepare_generic("
        )
        self.assertIn("device->ddMaxLBA = device->ddTrueMaxLBA;", generic)

        translate = function_body(
            MEDIA, "BOOLEAN_T sata_media_translate_lba("
        )
        self.assertRegex(
            MEDIA_HEADER,
            r"BOOLEAN_T\s+sata_media_translate_lba\(\s*"
            r"UINT32_T\s+port_num,\s*UINT64_T\s+logical_lba,\s*"
            r"UINT64_T\s*\*physical_lba\s*\);",
        )
        self.assertIn("physical_lba == NULL", translate)
        assert_statements_in_order(
            self,
            translate,
            (
                "sata_media_kind[port_num] == SATA_MEDIA_KIND_RDX",
                "*physical_lba = rdx_translate_media_lba(port_num, logical_lba);",
                "sata_media_kind[port_num] == SATA_MEDIA_KIND_GENERIC",
                "*physical_lba = logical_lba;",
                "*physical_lba = 0U;",
                "return FALSE;",
            ),
        )
        self.assertGreaterEqual(translate.count("return TRUE;"), 2)
        self.assertIn(
            "logical_lba + rdx_media_lba_offset[port_num]",
            RDX_ACCESS,
        )
        self.assertGreaterEqual(SCSI.count("sata_media_translate_lba("), 2)
        self.assertIn(
            "ata_dev[scsi_cmd.pCmdInput->bLUN].ddMaxLBA",
            SCSI,
        )

    def test_rdx_selection_disables_untranslated_unmap_only(self):
        """Suppress RDX DSM passthrough while retaining generic IDENTIFY data."""

        select_rdx = function_body(
            MEDIA, "static BOOLEAN_T sata_media_select_rdx("
        )
        assert_statements_in_order(
            self,
            select_rdx,
            (
                "device->bTRIMSupport = FALSE;",
                "device->wDataSetMgmtMaxBlocks = 0U;",
                "sata_media_kind[port_num] = SATA_MEDIA_KIND_RDX;",
            ),
        )
        prepare = function_body(MEDIA, "BOOLEAN_T sata_media_prepare(")
        self.assertGreaterEqual(
            prepare.count("sata_media_select_rdx(port_num, device);"),
            2,
        )

        generic = function_body(
            MEDIA, "static BOOLEAN_T sata_media_prepare_generic("
        )
        self.assertNotIn("bTRIMSupport", generic)
        self.assertNotIn("wDataSetMgmtMaxBlocks", generic)
        self.assertIn(
            "ata_dev[port_num].bTRIMSupport = id_info[169] & 0x1;",
            AHCI,
        )
        self.assertIn(
            "ata_dev[port_num].wIdentifyDeviceInfo[105]",
            AHCI,
        )
        unmap = function_body(SCSI, "inline STATUS_T scsi_handle_unmap_cmd(")
        self.assertIn(
            "UINT32_T lun = scsi_cmd.pCmdInput->bLUN;",
            unmap,
        )
        self.assertIn(
            "if (!ata_dev[lun].bTRIMSupport)",
            unmap,
        )
        self.assertIn("return STATUS_SCSI_INVALID_CMD;", unmap)

    def test_bot_path_keeps_dma_setup_auto_activate_disabled(self):
        """Do not enable the optional NCQ shortcut on the published BOT path."""

        self.assertRegex(USB_CHAPTER9, r"#define\s+UAS_ENABLE\s+0\b")
        init_port = function_body(AHCI, "STATUS_T ahci_init_port(")
        port_reset = function_body(AHCI, "STATUS_T ahci_port_reset(")
        self.assertIn(
            "ata_dev[port_num].bDMA_SetupAutoActivateSupport = FALSE;",
            init_port,
        )
        for path in (init_port, port_reset):
            self.assertNotIn(
                "ahci_set_features_dma_auto_activate(port_num, TRUE)",
                path,
            )

    def test_large_disks_keep_64_bit_capacity_and_address_bits(self):
        """Carry media larger than LBA32 through IDENTIFY, SCSI, and ATA."""

        three_tb_sector_count = 3_000_000_000_000 // 512
        self.assertGreater(three_tb_sector_count - 1, 0xFFFFFFFF)
        self.assertIn("UINT64_T     ddMaxLBA;", AHCI_HEADER)
        self.assertIn("UINT64_T     ddTrueMaxLBA;", AHCI_HEADER)
        self.assertIn("id_info[83] & 0x0400", AHCI)
        self.assertIn("&id_info[100]", AHCI)
        self.assertIn("sizeof(ata_dev[port_num].ddMaxLBA)", AHCI)
        self.assertRegex(
            AHCI_HEADER,
            r"#define\s+ENABLE_LARGE_SECTOR_EMULATION\s+0\b",
        )

        capacity = function_body(
            SCSI, "inline STATUS_T scsi_handle_read_capacity_cmd("
        )
        self.assertIn("UINT64_T num_blocks;", capacity)
        self.assertIn("SCSI_READ_CAPACITY16", capacity)
        self.assertIn(
            "num_blocks = ata_dev[scsi_cmd.pCmdInput->bLUN].ddMaxLBA - 1;",
            capacity,
        )
        for shift in (56, 48, 40, 32):
            self.assertIn("num_blocks >> {}".format(shift), capacity)

        read_write = function_body(
            SCSI, "inline BOOLEAN_T scsi_build_ata_rw_cmd("
        )
        for shift in (40, 32, 24):
            self.assertIn("scsi_cmd.bLBA_exp", read_write)
            self.assertIn(">> {}".format(shift), read_write)
        self.assertIn("ata_cmd->fis.LBA_mid_exp", read_write)
        self.assertIn("ata_cmd->fis.LBA_high_exp", read_write)

    def test_read_format_capacities_saturates_large_block_counts(self):
        """Report FFFFFFFF instead of wrapping a capacity above 32 bits."""

        format_capacities = function_body(
            SCSI, "inline STATUS_T scsi_handle_read_format_capacities_cmd("
        )
        self.assertIn("UINT32_T reported_blocks;", format_capacities)
        assert_statements_in_order(
            self,
            format_capacities,
            (
                "ata_dev[scsi_cmd.pCmdInput->bLUN].ddMaxLBA >> 32",
                "0xFFFFFFFFU",
                "(UINT32_T)ata_dev[scsi_cmd.pCmdInput->bLUN].ddMaxLBA",
                "scsi_resp_buff[4] = (reported_blocks >> 24)",
                "scsi_resp_buff[7] = reported_blocks & 0xFF",
            ),
        )
        self.assertNotIn(
            "scsi_resp_buff[4] = "
            "(ata_dev[scsi_cmd.pCmdInput->bLUN].ddMaxLBA >> 24)",
            format_capacities,
        )

    def test_sixteen_byte_commands_reject_lbas_above_ata48(self):
        """Reject nonzero CDB bytes two or three before narrowing the LBA."""

        read_write_params = function_body(
            SCSI, "inline STATUS_T scsi_get_rw_params("
        )
        read_write_16 = read_write_params[
            read_write_params.index("case SCSI_READ16:") :
        ]
        verify_params = function_body(
            SCSI, "inline STATUS_T scsi_get_verify_params("
        )
        verify_16 = verify_params[verify_params.index("case SCSI_VERIFY16:") :]

        for command_path in (read_write_16, verify_16):
            self.assertRegex(
                command_path,
                r"pCommandBlock\[2\]\s*!=\s*0U\)\s*\|\|\s*"
                r"\(scsi_cmd\.pCmdInput->pCommandBlock\[3\]\s*!=\s*0U",
            )
            self.assertIn(
                "status = STATUS_SCSI_INVALID_ADDRESS_RANGE;",
                command_path,
            )
            self.assertLess(
                command_path.index("pCommandBlock[2] != 0U"),
                command_path.index(
                    "scsi_cmd.bLBA_exp[2] = "
                    "scsi_cmd.pCmdInput->pCommandBlock[4];"
                ),
            )

    def test_runtime_comreset_defers_readmission_until_error_callbacks_finish(self):
        """Invalidate link policy and preserve error delivery before discovery."""

        for flag in (
            "ahci_hotplug_pending",
            "ahci_hotplug_quiesced",
            "ahci_reinit_wait_for_callbacks",
        ):
            self.assertRegex(
                AHCI,
                r"static\s+volatile\s+BOOLEAN_T\s+{}"
                r"\[NUM_AHCI_PORTS\];".format(flag),
            )

        schedule = function_body(AHCI, "static void ahci_schedule_media_discovery(")
        assert_statements_in_order(
            self,
            schedule,
            (
                "bDeviceInitComplete = FALSE;",
                "sata_media_reset(port_num);",
                "ahci_reinit_wait_for_callbacks[port_num] = wait_for_callbacks;",
                "ahci_hotplug_pending[port_num] = TRUE;",
            ),
        )

        fatal = function_body(AHCI, "void ahci_fatal_error_recovery(")
        assert_statements_in_order(
            self,
            fatal,
            (
                "port_was_reset = TRUE;",
                "ahci_port_reset(port_num)",
                "if (port_was_reset && media_was_ready)",
                "ahci_schedule_media_discovery(port_num, TRUE);",
                "ahci_ata_cbk_queue_add(port_num, ata_dev[port_num].pAtaErrorCallback);",
            ),
        )

        receive_error = function_body(AHCI, "void ahci_rx_error_isr(")
        assert_statements_in_order(
            self,
            receive_error,
            (
                "media_was_ready = ata_dev[0].bDeviceInitComplete;",
                "ahci_port_reset(0)",
                "ahci_schedule_media_discovery(0U, TRUE);",
                "!ahci_callbacks_are_pending(0U)",
                "ahci_ata_cbk_queue_add(",
                "ata_dev[0].pAtaErrorCallback",
            ),
        )

        reset_lun = function_body(AHCI, "void ahci_reset_lun(")
        self.assertLess(
            reset_lun.index("ahci_clear_callback_queue(lun);"),
            reset_lun.index("ahci_fatal_error_recovery(lun, force_comreset);"),
        )
        assert_statements_in_order(
            self,
            reset_lun,
            (
                "if (ahci_hotplug_pending[lun])",
                "deferring media discovery",
                "else",
                "ahci_init();",
            ),
        )

        service = function_body(AHCI, "void ahci_service(")
        assert_statements_in_order(
            self,
            service,
            (
                "if (ahci_reinit_wait_for_callbacks[port_num])",
                "if (ahci_callbacks_are_pending(port_num))",
                "continue;",
                "if (ahci_callbacks_are_pending(port_num))",
                "ahci_reinit_wait_for_callbacks[port_num] = TRUE;",
                "continue;",
                "if (!ahci_hotplug_quiesced[port_num])",
                "ahci_hotplug_quiesced[port_num] = TRUE;",
                "ahci_stop(port_num);",
                "continue;",
                "PSSTS_DET_PHY_READY",
                "status = ahci_init_port(port_num);",
            ),
        )

    def test_hotplug_service_reenters_the_same_dual_media_pipeline(self):
        """Defer insertion work, then rerun reset, IDENTIFY, and admission."""

        interrupt = function_body(
            AHCI,
            "inline void ahci_port_intr_handler(",
        )
        connect_change = interrupt[interrupt.index("PORT_CONNECT_CHANGE_STATUS") :]
        self.assertIn("ahci_handle_media_link_change(port_num);", connect_change)
        self.assertLess(
            connect_change.index("ahci_handle_media_link_change(port_num);"),
            connect_change.index("return;"),
        )
        link_change = function_body(
            AHCI, "static void ahci_handle_media_link_change("
        )
        assert_statements_in_order(
            self,
            link_change,
            (
                "media_was_ready = ata_dev[port_num].bDeviceInitComplete;",
                "sata_media_link_disconnected(port_num);",
                "ahci_schedule_media_discovery(port_num, media_was_ready);",
                "if (media_was_ready &&",
                "USB_DEVICE_STATE_CONFIGURED",
                "ahci_ata_cbk_queue_add(",
                "pAtaErrorCallback",
            ),
        )
        self.assertNotIn("ahci_init_port(port_num)", link_change)
        schedule = function_body(AHCI, "static void ahci_schedule_media_discovery(")
        assert_statements_in_order(
            self,
            schedule,
            (
                "bDeviceInitComplete = FALSE;",
                "sata_media_reset(port_num);",
                "ahci_hotplug_pending[port_num] = TRUE;",
            ),
        )

        service = function_body(AHCI, "void ahci_service(")
        assert_statements_in_order(
            self,
            service,
            (
                "if (!ahci_hotplug_pending[port_num])",
                "PSSTS_DET_PHY_READY",
                "ahci_hotplug_pending[port_num] = FALSE;",
                "status = ahci_init_port(port_num);",
            ),
        )
        init_port = function_body(AHCI, "STATUS_T ahci_init_port(")
        assert_statements_in_order(
            self,
            init_port,
            (
                "sata_media_reset(port_num);",
                "status = ahci_identify_device(port_num);",
                "sata_media_prepare(port_num, &ata_dev[port_num])",
            ),
        )
        self.assertLess(
            MAIN.index("ahci_service();"),
            MAIN.index("if (ata_dev[0].callback_pending"),
        )


if __name__ == "__main__":
    unittest.main()
