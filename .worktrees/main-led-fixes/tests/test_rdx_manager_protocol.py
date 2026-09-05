# SPDX-FileCopyrightText: 2026 Andre Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Source-contract tests for the OpenRDX Manager protocol."""

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[1]
SCSI_HEADER = ROOT / "include" / "rdx_mount" / "scsi.h"
SCSI_SOURCE = ROOT / "src" / "rdx_mount" / "scsi.c"
SCSI_DATA = ROOT / "src" / "rdx_mount" / "scsi_data.c"
AHCI_HEADER = ROOT / "include" / "rdx_mount" / "ahci.h"
AHCI_SOURCE = ROOT / "src" / "rdx_mount" / "ahci.c"
PROTOCOL_SOURCE = ROOT / "src" / "rdx_mount" / "rdx_manager_protocol.c"
PROTOCOL_HEADER = ROOT / "include" / "rdx_mount" / "rdx_manager_protocol.h"
CONTROL_SOURCE = ROOT / "src" / "rdx_mount" / "rdx_manager_control.c"
HARDWARE_SOURCE = ROOT / "src" / "rdx_mount" / "rdx_hardware.c"
IDENTITY_HEADER = ROOT / "include" / "rdx_mount" / "rdx_manager_identity.h"
IDENTITY_SOURCE = ROOT / "src" / "rdx_mount" / "rdx_manager_identity.c"
LED_HEADER = ROOT / "include" / "rdx_mount" / "rdx_led.h"
LED_SOURCE = ROOT / "src" / "rdx_mount" / "rdx_led.c"
UNLOCK_SOURCE = ROOT / "src" / "rdx_mount" / "rdx_unlock.c"
SPI_SOURCE = ROOT / "src" / "rdx_mount" / "spi.c"
RTI_SOURCE = ROOT / "src" / "rdx_mount" / "rti.c"
UPDATE_SCRIPT = ROOT / "scripts" / "rdx_manager_firmware_update.ps1"


class RdxManagerProtocolTests(unittest.TestCase):
    """Verify the manager-facing discovery and update invariants."""

    @classmethod
    def setUpClass(cls):
        """Load the focused source files once for all contract checks."""
        cls.header = SCSI_HEADER.read_text(encoding="utf-8")
        cls.scsi = SCSI_SOURCE.read_text(encoding="utf-8")
        cls.data = SCSI_DATA.read_text(encoding="utf-8")
        cls.ahci_header = AHCI_HEADER.read_text(encoding="utf-8")
        cls.ahci = AHCI_SOURCE.read_text(encoding="utf-8")
        cls.protocol = PROTOCOL_SOURCE.read_text(encoding="utf-8")
        cls.protocol_header = PROTOCOL_HEADER.read_text(encoding="utf-8")
        cls.control = CONTROL_SOURCE.read_text(encoding="utf-8")
        cls.hardware = HARDWARE_SOURCE.read_text(encoding="utf-8")
        cls.identity_header = IDENTITY_HEADER.read_text(encoding="utf-8")
        cls.identity = IDENTITY_SOURCE.read_text(encoding="utf-8")
        cls.led_header = LED_HEADER.read_text(encoding="utf-8")
        cls.led = LED_SOURCE.read_text(encoding="utf-8")
        cls.unlock = UNLOCK_SOURCE.read_text(encoding="utf-8")
        cls.spi = SPI_SOURCE.read_text(encoding="utf-8")
        cls.rti = RTI_SOURCE.read_text(encoding="utf-8")
        cls.update_script = UPDATE_SCRIPT.read_text(encoding="utf-8")

    def test_manager_scsi_opcodes_are_dispatched(self):
        """LOG SENSE and WRITE BUFFER must reach focused handlers."""
        self.assertRegex(self.header, r"SCSI_LOG_SENSE\s+0x4D")
        self.assertRegex(self.header, r"SCSI_WRITE_BUFFER\s+0x3B")
        self.assertIn("case SCSI_LOG_SENSE:", self.scsi)
        self.assertIn("status = scsi_handle_log_sense_cmd();", self.scsi)
        self.assertIn("case SCSI_WRITE_BUFFER:", self.scsi)
        self.assertIn("status = scsi_handle_write_buffer_cmd();", self.scsi)
        self.assertNotIn("(cdb[2] & 0xC0U) != 0x40U", self.scsi)

    def test_standard_inquiry_contains_rdx_manager_extension(self):
        """The private bytes used by SupportsRDXTech remain exact."""
        self.assertIn("#define STANDARD_INQUIRY_DATA_LENGTH  64U", self.scsi)
        self.assertIn('ti_memcpy((void*)&scsi_resp_buff[32], "0001", 4U);', self.scsi)
        self.assertIn("scsi_resp_buff[36] = 0x38U;", self.scsi)
        self.assertIn('ti_memcpy((void*)&scsi_resp_buff[37], "RDX", 3U);', self.scsi)
        self.assertIn("scsi_resp_buff[40] = 0x02U;", self.scsi)
        self.assertIn("scsi_resp_buff[41] = 0x57U;", self.scsi)
        self.assertIn("scsi_resp_buff[42] = 0x50U;", self.scsi)

    def test_empty_dock_keeps_stable_no_media_lun(self):
        """SATA removal must leave a stable Manager-addressable USB dock."""
        self.assertRegex(
            self.ahci_header,
            r"#define\s+REMOVABLE_MEDIA_DEVICE\s+1\b",
        )
        link_change = self.ahci[
            self.ahci.index("static void ahci_handle_media_link_change("):
            self.ahci.index("#if DEBUG_LEVEL >= 1")
        ]
        self.assertIn("#else", link_change)
        self.assertIn("usb_hal_disconnect();", link_change)
        self.assertIn("scsi_resp_buff[1] = RMB_BIT;", self.scsi)
        self.assertIn(
            "scsi_set_sense_data(NOT_READY, MEDIUM_NOT_PRESENT, NO_ASCQ);",
            self.scsi,
        )
        failed_init = self.ahci[
            self.ahci.index("if (status == STATUS_OK)\n    {\n        // Set medium change flag."):
            self.ahci.index("// Call port init callback.")
        ]
        self.assertIn(
            "ata_dev[port_num].bDeviceInitTimedOut = TRUE;",
            failed_init,
        )
        init_port = self.ahci[
            self.ahci.index("STATUS_T ahci_init_port(UINT32_T port_num)"):
            self.ahci.index("Function: ahci_handle_sdb_fis")
        ]
        mask_error = init_port.index(
            "WRITE_REG32(VIM_REQMASKCLR0, 0x00000020);"
        )
        ready = init_port.index("bDeviceInitComplete = TRUE;")
        mask_publication = init_port.index(
            "AHCI_MEDIA_PUBLICATION_INTERRUPT_MASK", mask_error
        )
        unmask_sata = init_port.index(
            "WRITE_REG32(VIM_REQMASKSET0, AHCI_SATA_INTERRUPT_MASK);",
            ready,
        )
        unmask_usb = init_port.index(
            "WRITE_REG32(VIM_REQMASKSET0, AHCI_USB_INTERRUPT_MASK);",
            unmask_sata,
        )
        self.assertLess(mask_error, ready)
        self.assertLess(mask_publication, ready)
        self.assertLess(ready, unmask_sata)
        self.assertLess(unmask_sata, unmask_usb)
        port_interrupt = self.ahci[
            self.ahci.index("inline void ahci_port_intr_handler("):
            self.ahci.index("void ahci_service(void)")
        ]
        self.assertIn("ahci_handle_media_link_change(port_num);", port_interrupt)
        self.assertLess(
            port_interrupt.index("ahci_handle_media_link_change(port_num);"),
            port_interrupt.index("PORT_FATAL_ERROR_INTR"),
        )
        self.assertIn("return;", port_interrupt)
        self.assertIn(
            "media_was_ready = ata_dev[port_num].bDeviceInitComplete;",
            link_change,
        )
        self.assertIn(
            "ahci_schedule_media_discovery(port_num, media_was_ready);",
            link_change,
        )
        self.assertIn("USB_DEVICE_STATE_CONFIGURED", link_change)
        self.assertIn("pAtaErrorCallback", link_change)
        self.assertNotIn("ahci_init_port(port_num);", link_change)
        self.assertNotIn("Ignoring empty-bay", link_change)
        schedule_discovery = self.ahci[
            self.ahci.index("static void ahci_schedule_media_discovery("):
            self.ahci.index("#if DEBUG_LEVEL >= 1")
        ]
        init_invalidated = schedule_discovery.index(
            "ata_dev[port_num].bDeviceInitComplete = FALSE;"
        )
        timeout_marked = schedule_discovery.index(
            "ata_dev[port_num].bDeviceInitTimedOut = TRUE;"
        )
        media_reset = schedule_discovery.index("sata_media_reset(port_num);")
        pending = schedule_discovery.index(
            "ahci_hotplug_pending[port_num] = TRUE;"
        )
        self.assertLess(init_invalidated, timeout_marked)
        self.assertLess(timeout_marked, media_reset)
        self.assertLess(media_reset, pending)
        self.assertIn(
            "ahci_reinit_wait_for_callbacks[port_num] = wait_for_callbacks;",
            schedule_discovery,
        )
        foreground_hotplug = self.ahci[
            self.ahci.index("void ahci_service(void)"):
            self.ahci.index("Function: ahci_isr")
        ]
        self.assertIn("ahci_hotplug_pending[port_num]", foreground_hotplug)
        self.assertIn("PSSTS_DET_PHY_READY", foreground_hotplug)
        self.assertIn("ahci_init_port(port_num);", foreground_hotplug)
        self.assertIn("usb_hal_init(NULL, NULL, NULL);", foreground_hotplug)
        self.assertIn("usb_hal_connect();", foreground_hotplug)
        self.assertIn("rdx_hardware_eject_in_progress()", foreground_hotplug)
        self.assertLess(
            foreground_hotplug.index("rdx_hardware_eject_in_progress()"),
            foreground_hotplug.index("ahci_stop(port_num);"),
        )
        callback_drain = foreground_hotplug.index(
            "if (ahci_callbacks_are_pending(port_num))"
        )
        quiesce = foreground_hotplug.index(
            "ahci_hotplug_quiesced[port_num] = TRUE;", callback_drain
        )
        stop = foreground_hotplug.index("ahci_stop(port_num);", quiesce)
        ready_link = foreground_hotplug.index("PSSTS_DET_PHY_READY", stop)
        self.assertLess(callback_drain, quiesce)
        self.assertLess(quiesce, stop)
        self.assertLess(stop, ready_link)
        controller_init = self.ahci[
            self.ahci.index("STATUS_T ahci_init(void)"):
            self.ahci.index("#define ERROR_CNT_THRESHOLD")
        ]
        self.assertIn("ahci_hotplug_pending[port_num] = FALSE;", controller_init)
        self.assertIn("ahci_hotplug_pending[port_num] = TRUE;", controller_init)
        self.assertLess(
            controller_init.index("ahci_init_port(port_num)"),
            controller_init.index("ahci_hotplug_pending[port_num] = TRUE;"),
        )
        identify = self.ahci[
            self.ahci.index("STATUS_T ahci_identify_device(UINT32_T port_num)"):
            self.ahci.index("Function: ahci_get_TFD_info")
        ]
        self.assertIn("#if REMOVABLE_MEDIA_DEVICE", identify)
        self.assertLess(
            identify.index("#if REMOVABLE_MEDIA_DEVICE"),
            identify.index("system_reset();"),
        )
        rx_error = self.ahci[
            self.ahci.index("void ahci_rx_error_isr(void)"):
        ]
        pending_discovery = rx_error[
            rx_error.index("if (!ata_dev[0].bDeviceInitComplete"):
            rx_error.index("else\n                {", rx_error.index(
                "if (!ata_dev[0].bDeviceInitComplete"
            ))
        ]
        self.assertIn("#if REMOVABLE_MEDIA_DEVICE", pending_discovery)
        self.assertIn("ata_dev[0].bDeviceInitTimedOut = TRUE;", pending_discovery)
        no_media_gate = self.scsi[
            self.scsi.index("if (!ata_dev[scsi_cmd.pCmdInput->bLUN].bDeviceInitComplete"):
            self.scsi.index("// Call appropriate command handler.")
        ]
        self.assertIn("SCSI_SECURITY_PROTOCOL_IN", no_media_gate)
        self.assertIn("SCSI_WRITE_BUFFER", no_media_gate)

    def test_rdx_vpd_pages_are_advertised_and_built(self):
        """Manager-required C0 and C2 pages must be discoverable."""
        self.assertIn("VPD_RDX_MEDIA_ID_PAGE_CODE", self.data)
        self.assertIn("VPD_RDX_MEDIA_IDENTIFY_PAGE_CODE", self.data)
        self.assertIn('rdx_append_literal(buffer, buffer_size, &offset, "Cartridge:")', self.protocol)
        self.assertIn("rdx_store_be16(&buffer[2], 0x0200U);", self.protocol)
        self.assertIn("ata_dev[lun].wIdentifyDeviceInfo[index]", self.protocol)
        self.assertIn("RDX_MANUFACTURING_RECORD_ADDRESS     0x3E000U", self.identity)
        self.assertIn("rdx_manufacturing_record_is_valid", self.identity)
        self.assertIn("rdx_manager_get_drive_identity", self.protocol)
        self.assertIn("rdx_manager_build_drive_id_vpd", self.scsi)
        self.assertNotIn("bWorldWideNameValid", self.scsi[
            self.scsi.index("inline void scsi_build_dev_id_vpd_page"):
            self.scsi.index("#define MAX_UNMAP_DESC_SIZE")
        ])

    def test_rdx_media_identity_comes_from_authenticated_metadata(self):
        """Keep all four RDX C0 properties bound to validated metadata."""
        for record_type in (
            "RDX_METADATA_TYPE_VENDOR",
            "RDX_METADATA_TYPE_MODEL",
            "RDX_METADATA_TYPE_SERIAL",
            "RDX_METADATA_TYPE_BARCODE",
        ):
            self.assertIn(record_type, self.unlock)
        self.assertIn("rdx_capture_media_identity(port_num, sector);", self.unlock)
        self.assertIn("rdx_load_media_identity(port_num, device)", self.unlock)
        self.assertIn("RDX_METADATA_TYPE_CONTEXT3", self.unlock)
        self.assertIn("rdx_metadata_identity_link_types", self.unlock)
        self.assertIn("rdx_get_media_identity(lun)", self.protocol)
        self.assertIn(
            "capacity_blocks = (UINT32_T)ata_dev[lun].ddTrueMaxLBA;",
            self.protocol,
        )
        c0_builder = self.protocol[
            self.protocol.index("rdx_manager_build_media_id_vpd"):
            self.protocol.index("rdx_manager_build_media_identify_vpd")
        ]
        self.assertNotIn("ddTrueMaxLBA >> 11U", c0_builder)
        self.assertNotIn('rdx_append_literal(buffer, buffer_size, &offset, "TANDBERG")', c0_builder)
        for field in ("vendor", "model", "serial", "barcode"):
            self.assertIn(f"media_identity->{field}", c0_builder)

    def test_generic_media_brand_uses_standardized_ata_model_text(self):
        """Give admitted generic media a useful C0 brand without guessing."""
        c0_builder = self.protocol[
            self.protocol.index("rdx_manager_build_media_id_vpd"):
            self.protocol.index("rdx_manager_build_media_identify_vpd")
        ]
        generic_start = c0_builder.index(
            "sata_media_get_kind(lun) == SATA_MEDIA_KIND_GENERIC"
        )
        generic = c0_builder[
            generic_start:c0_builder.index("\n    else\n", generic_start)
        ]

        self.assertIn('#include "sata_media.h"', self.protocol)
        self.assertIn("model, 40U,", generic)
        self.assertIn("RDX_MEDIA_VENDOR_LENGTH", generic)
        self.assertEqual(3, generic.count(
            'rdx_append_literal(buffer, buffer_size, &offset, "")'
        ))
        self.assertNotIn("serial", generic)
        self.assertNotIn("SATA_MEDIA_KIND_RDX", generic)

    def test_media_id_preserves_fixed_field_padding(self):
        """The C0 builder stops at NUL but preserves significant spaces."""
        append_text = self.protocol[
            self.protocol.index("static BOOLEAN_T rdx_append_ata_text"):
            self.protocol.index("static BOOLEAN_T rdx_append_hex32")
        ]
        self.assertIn("text[length] != 0U", append_text)
        self.assertNotIn("text[length - 1U] == ' '", append_text)

    def test_required_log_and_mode_pages_are_present(self):
        """All pages required by Test_DockInfo::RefreshDockInfo exist."""
        for page in (
            "RDX_LOG_SENSE_DRIVE_PAGE_CODE",
            "RDX_LOG_SENSE_CARTRIDGE_PAGE_CODE",
            "RDX_LOG_SENSE_TEMPERATURE_PAGE_CODE",
            "RDX_MODE_PAGE_DRIVE_CONTROL",
            "RDX_MODE_PAGE_VENDOR_SHORT",
            "RDX_MODE_PAGE_OPERATION",
        ):
            self.assertIn(page, self.protocol + self.control)
        self.assertIn("#define RDX_DEFAULT_OPERATION_MODE", self.control)
        self.assertIn("buffer[4] = RDX_DEFAULT_OPERATION_MODE;", self.control)
        self.assertIn("#define PAGE_CODE_HEADER_BLOCK_DESC 0x00", self.scsi)
        self.assertIn("page_code == PAGE_CODE_HEADER_BLOCK_DESC", self.scsi)
        self.assertIn("data_length = index;", self.scsi)
        self.assertIn("mode_sense_data[7] = 8U;", self.scsi)
        self.assertIn("dSectorSize >> 16U", self.scsi)

    def test_tandberg_security_queries_are_not_forwarded_to_sata(self):
        """No-security records must answer Manager's 10h/21h probes."""
        self.assertIn("rdx_manager_build_security_protocol_in", self.scsi)
        self.assertIn("case 0x0010U:", self.protocol)
        self.assertIn("case 0x0021U:", self.protocol)
        self.assertIn("rdx_store_be32(&buffer[40], 0x80010400UL);", self.protocol)
        self.assertIn("buffer[12] = 1U;", self.protocol)
        intercept = self.scsi.index("rdx_manager_build_security_protocol_in")
        trusted = self.scsi.index("ATA_CMD_TRUSTED_RECEIVE_DMA", intercept)
        self.assertLess(intercept, trusted)

    def test_update_authenticates_compatibility_and_openrdx_containers(self):
        """The updater pins its compatibility image and hashes marked payloads."""
        self.assertIn("#define RDX_UPDATE_CONTAINER_LENGTH         0xF29EU", self.protocol)
        self.assertIn("#define RDX_UPDATE_PAYLOAD_OFFSET           0x018CU", self.protocol)
        self.assertIn("#define RDX_UPDATE_PAYLOAD_END              0xF29AU", self.protocol)
        digest_match = re.search(
            r"rdx_compatibility_image_sha256\[32\]\s*=\s*\{(?P<body>.*?)\};",
            self.protocol,
            re.DOTALL,
        )
        self.assertIsNotNone(digest_match)
        digest = bytes(
            int(value, 16)
            for value in re.findall(r"0x([0-9A-Fa-f]{2})", digest_match.group("body"))
        )
        self.assertEqual(
            digest.hex(),
            "73d528801aefc032d3a53637b035f65d72809151b2f127c6b2050e9bacc76f3b",
        )
        self.assertIn("offset != rdx_update.next_container_offset", self.protocol)
        self.assertIn("length > RDX_UPDATE_MAX_CHUNK", self.protocol)
        self.assertIn("STATUS_SCSI_AUTHENTICATION_FAILURE", self.protocol)
        self.assertIn("RDX_UPDATE_CUSTOM_AUTH_OFFSET       0x0108U", self.protocol)
        self.assertIn("'O', 'P', 'E', 'N', 'R', 'D', 'X', '1'", self.protocol)
        self.assertIn("rdx_capture_custom_auth(offset, payload, length);", self.protocol)
        self.assertIn("rdx_hash_container_payload(offset, payload, length);", self.protocol)
        self.assertIn("rdx_custom_update_is_valid(payload_digest)", self.protocol)

    def test_manager_mode_select_pages_are_applied_and_saved(self):
        """Vendor mode setters must accept Manager PF/SP and preserve state."""

        self.assertIn("rdx_manager_handle_mode_select(", self.scsi)
        self.assertIn("page_code == RDX_MODE_PAGE_DRIVE_CONTROL", self.control)
        self.assertIn("RDX_MODE_PAGE_VENDOR_SHORT", self.control)
        self.assertIn("RDX_MODE_PAGE_OPERATION", self.control)
        self.assertIn("RDX_STATE_RECORD_ADDRESS", self.control)
        self.assertIn("rdx_control_save_operation_mode", self.control)
        self.assertIn("rdx_control_save_drive_load_count", self.control)
        self.assertIn("rdx_manager_increment_drive_load_count", self.control)
        self.assertIn("OpcodeSectorErase", self.control)
        self.assertIn("rdx_manager_get_drive_load_count()", self.protocol)

    def test_page31_controls_logical_unload_and_reload(self):
        """Require both advertised drive-control bits to affect host LOEJ."""

        for token in (
            "RDX_DRIVE_CONTROL_LOGICAL_UNLOAD        0x08U",
            "RDX_DRIVE_CONTROL_AUTO_RELOAD           0x10U",
            "RDX_DRIVE_CONTROL_POLICY_MASK           0x18U",
            "RDX_DRIVE_CONTROL_REJECTED_MASK         0xE2U",
        ):
            self.assertIn(token, self.control)
        self.assertIn("rdx_manager_apply_host_eject_policy(void)", self.control)
        self.assertIn("(void)rdx_hardware_logical_unload();", self.control)
        self.assertIn("(void)rdx_hardware_logical_reload();", self.control)
        self.assertIn(
            "if (!rdx_hardware_logical_reload())",
            self.control,
        )
        self.assertIn(
            "if (!rdx_manager_apply_host_eject_policy())",
            self.scsi,
        )
        self.assertLess(
            self.scsi.index("if (!rdx_manager_apply_host_eject_policy())"),
            self.scsi.index("rdx_hardware_request_eject();"),
        )
        self.assertIn(
            "BOOLEAN_T rdx_manager_apply_host_eject_policy(void);",
            self.protocol_header,
        )
        self.assertIn("void rdx_manager_clear_host_eject_policy(void);", self.protocol_header)
        self.assertGreaterEqual(
            self.hardware.count("rdx_manager_clear_host_eject_policy();"),
            2,
        )

    def test_page33_round_trips_and_gates_physical_eject(self):
        """Keep the physical-button inhibit in byte three from setter to use."""

        self.assertIn("RDX_PHYSICAL_EJECT_INHIBIT              0x01U", self.control)
        self.assertIn("buffer[3] = RDX_PHYSICAL_EJECT_INHIBIT;", self.control)
        self.assertIn(
            "buffer[3] = rdx_control.physical_eject_inhibited ? 1U : 0U;",
            self.control,
        )
        self.assertIn("(page[1] != 2U) || (page[2] != 0U)", self.control)
        self.assertIn(
            "(page[3] & (UINT8_T)~RDX_PHYSICAL_EJECT_INHIBIT)",
            self.control,
        )
        self.assertIn(
            "rdx_control.physical_eject_inhibited =",
            self.control,
        )
        self.assertIn(
            "BOOLEAN_T rdx_manager_physical_eject_is_enabled(void);",
            self.protocol_header,
        )
        self.assertIn(
            "if (!rdx_manager_physical_eject_is_enabled())",
            self.hardware,
        )
        self.assertNotIn("eject_poll_state", self.control)

    def test_page85_is_acknowledgement_only(self):
        """Validate and acknowledge page 85h without changing eject policy."""

        page85 = self.control[
            self.control.index("(parameter_list[0] == RDX_DIAGNOSTIC_EJECT_PAGE)") :
            self.control.index("return STATUS_SCSI_INVALID_CMD_FIELD;", self.control.index(
                "(parameter_list[0] == RDX_DIAGNOSTIC_EJECT_PAGE)"
            ))
        ]
        self.assertIn("return STATUS_OK;", page85)
        for unsupported_effect in (
            "rdx_hardware_",
            "rdx_mechanism_",
            "drive_control",
            "physical_eject_inhibited",
            "eject_mode =",
        ):
            self.assertNotIn(unsupported_effect, page85)

    def test_hardware_profile_comes_from_validated_manufacturing_record(self):
        """Expose the raw offset-A4 profile with its exact 38h fallback."""

        self.assertIn("RDX_MANUFACTURING_PROFILE_OFFSET     0x00A4U", self.identity)
        self.assertIn("RDX_DEFAULT_HARDWARE_PROFILE         0x0038U", self.identity)
        self.assertIn("rdx_hardware_profile = RDX_DEFAULT_HARDWARE_PROFILE;", self.identity)
        self.assertIn(
            "(UINT16_T)record[RDX_MANUFACTURING_PROFILE_OFFSET] |",
            self.identity,
        )
        self.assertIn("rdx_manager_identity_init();", self.protocol)
        self.assertIn("rdx_manager_get_hardware_profile", self.identity)
        self.assertIn("rdx_manager_get_hardware_profile", self.identity_header)
        self.assertIn("rdx_hardware_profile == 0xFFFFU", self.identity)
        self.assertIn("return 0x0037U;", self.identity)
        self.assertIn('#include "rdx_manager_identity.h"', self.protocol_header)

    def test_manager_diagnostic_commands_are_dispatched(self):
        """LED, acknowledgement, and unit-reset forms reach local handlers."""

        self.assertIn("case SCSI_SEND_DIAGNOSTIC:", self.scsi)
        self.assertIn("rdx_manager_handle_send_diagnostic(", self.scsi)
        self.assertIn("RDX_DIAGNOSTIC_LED_PAGE", self.control)
        self.assertIn("RDX_DIAGNOSTIC_EJECT_PAGE", self.control)
        self.assertIn("RDX_DIAGNOSTIC_UNIT_RESET_FLAGS", self.control)
        self.assertIn("usb_hal_disconnect();", self.control)
        self.assertIn("system_reset();", self.control)
        self.assertIn(
            "rdx_led_set_diagnostic(parameter_list[7] == 0U);",
            self.control,
        )
        self.assertIn("rdx_led_tick();", self.control)
        self.assertNotIn("saved_gio_output", self.control)
        self.assertNotIn("saved_sci_function", self.control)
        self.assertNotIn("LED_ON(", self.control)
        self.assertIn("rdx_manager_control_tick();", self.rti)

    def test_led_controller_matches_required_mapping_and_timing(self):
        """The production LED port must retain its pin and phase contract."""

        self.assertRegex(
            self.led,
            r"RDX_LED_DOCK_FIRST_MASK\s+SCI_PIO_RX_GPIO8",
        )
        self.assertRegex(
            self.led,
            r"RDX_LED_DOCK_SECOND_MASK\s+SCI_PIO_TX_GPIO9",
        )
        self.assertIn("1UL << RDX_CARTRIDGE_GREEN_GIO_NUM", self.led)
        self.assertIn("1UL << RDX_CARTRIDGE_AMBER_GIO_NUM", self.led)
        self.assertRegex(self.led, r"RDX_LED_NORMAL_SERVICE_TICKS\s+5U")
        self.assertRegex(self.led, r"RDX_LED_NORMAL_BLINK_PHASES\s+6U")
        self.assertRegex(self.led, r"RDX_LED_FORCED_SECOND_PHASES\s+8U")
        self.assertRegex(self.led, r"RDX_LED_FAST_BLINK_PHASES\s+20U")
        self.assertIn("controller->forced_second_phases--;", self.led)
        self.assertIn("controller->blink_phases--;", self.led)
        self.assertIn("controller->phase = !controller->phase;", self.led)
        selector_setter = self.led[
            self.led.index("void rdx_led_select_second"):
            self.led.index("void rdx_led_set_steady")
        ]
        self.assertIn("if (selected)", selector_setter)
        self.assertIn(".steady = TRUE;", selector_setter)

        preload = self.led.index("MODIFY_REG32(GIOOUT0_REG_OFF")
        direction = self.led.index("MODIFY_REG32(GIODIR0_REG_OFF")
        sci_preload = self.led.index("MODIFY_REG32(SCI_PIO3")
        sci_function = self.led.index("MODIFY_REG32(SCI_PIO0")
        self.assertLess(preload, direction)
        self.assertLess(sci_preload, sci_function)

    def test_led_diagnostic_uses_mode_one_ring(self):
        """Mode 1 must cycle off, second, off, first without raw pin I/O."""

        diagnostic = self.led[
            self.led.index("static void rdx_led_advance_diagnostic"):
            self.led.index("static void rdx_led_service_controller")
        ]
        transitions = (
            "RDX_LED_DIAGNOSTIC_SECOND_PENDING_STATE;",
            "RDX_LED_STATE_SECOND;",
            "RDX_LED_DIAGNOSTIC_FIRST_PENDING_STATE;",
            "RDX_LED_STATE_FIRST;",
        )
        positions = [diagnostic.index(item) for item in transitions]
        self.assertEqual(positions, sorted(positions))
        self.assertEqual(diagnostic.count("controller->phase = FALSE;"), 2)
        self.assertEqual(diagnostic.count("controller->phase = TRUE;"), 2)
        self.assertRegex(self.led, r"RDX_LED_MODE_DIAGNOSTIC\s+1U")
        self.assertRegex(self.led, r"RDX_LED_MODE_FAST\s+2U")

        for api in (
            "rdx_led_init",
            "rdx_led_tick",
            "rdx_led_select_second",
            "rdx_led_set_steady",
            "rdx_led_start_blink",
            "rdx_led_start_second_blink",
            "rdx_led_start_fast_blink",
            "rdx_led_fast_blink_is_active",
            "rdx_led_show_gesture_selection",
            "rdx_led_confirm_gesture",
            "rdx_led_cancel_gesture",
            "rdx_led_take_gesture_confirmation",
            "rdx_led_set_diagnostic",
            "rdx_led_diagnostic_is_active",
        ):
            self.assertIn(api, self.led_header)

    def test_compatibility_installation_is_guarded_and_manifest_pinned(self):
        """Installation must validate its late authentication result and ROM target."""

        self.assertIn("Enable-RdxCompatibilityHeaderCheckBypass", self.update_script)
        self.assertIn(
            "@(0x55, 0xAA, 0x25, 0x46, 0x04, 0xC1, 0x1D, 0xB7)",
            self.update_script,
        )
        self.assertIn(
            "-Mode 0x02 -BufferId 0x80 -Offset 0 -Length 10",
            self.update_script,
        )
        self.assertIn(
            "-Mode 0x02 -BufferId 0x82 -Offset 0x020FF8 -Length 2",
            self.update_script,
        )
        self.assertIn(
            "$compatibilityDiskPnpPrefix = "
            "'USBSTOR\\DISK&VEN_TANDBERG&PROD_RDX&REV_0283\\'",
            self.update_script,
        )
        self.assertNotRegex(
            self.update_script,
            r"\$[A-Za-z0-9_]*Serial(?:Number)?\s*=\s*'\d{10}'",
        )
        self.assertIn("[string] $TargetSerialNumber", self.update_script)
        self.assertIn("[string] $ValidateFirmwareKind = 'OpenRDX'", self.update_script)
        self.assertEqual(
            self.update_script.count("Enable-RdxCompatibilityHeaderCheckBypass"),
            2,
        )
        self.assertIn("-SerialNumber $selectedTargetSerialNumber", self.update_script)
        self.assertIn(
            "ParameterSetName = 'InstallOpenRDXOnCompatibilityReceiver'",
            self.update_script,
        )
        self.assertIn("[switch] $InstallOpenRDXOnCompatibilityReceiver", self.update_script)
        self.assertIn("function Test-RdxSignatureFailure", self.update_script)
        self.assertIn("$sense[12].ToUpperInvariant() -eq '74'", self.update_script)
        self.assertIn("$sense[13].ToUpperInvariant() -eq '08'", self.update_script)
        self.assertIn(
            "$romBootloaderPnpPrefix = 'USB\\VID_0451&PID_926B\\'",
            self.update_script,
        )
        self.assertIn("function Wait-RdxPnpPrefix", self.update_script)
        self.assertIn("function Get-RdxUsbAncestorInstanceId", self.update_script)
        self.assertIn("DEVPKEY_Device_Parent", self.update_script)
        self.assertIn("DEVPKEY_Device_LocationPaths", self.update_script)
        self.assertIn("-RequiredLocationPaths $selectedLocationPaths", self.update_script)
        self.assertIn("-RequireSingleGlobalMatch", self.update_script)
        self.assertIn("index-based programming is unsafe", self.update_script)
        self.assertIn("TI FlashBurner returned success", self.update_script)
        self.assertIn("completed without J7", self.update_script)
        self.assertIn("requires an elevated PowerShell session", self.update_script)
        self.assertIn("$manifest.authentication_scheme", self.update_script)
        self.assertIn("'openrdx-sha256-v1'", self.update_script)
        self.assertIn("$manifest.requires_openrdx_receiver", self.update_script)
        self.assertIn("$manifest.installation_requires_rom_loader", self.update_script)
        self.assertIn(
            "[System.IO.Path]::GetFileName($flashHexName)", self.update_script
        )
        self.assertIn(
            "name its FlashBurner HEX as a file in the same directory",
            self.update_script,
        )
        self.assertIn("Resolve-Path -LiteralPath $ManifestPath", self.update_script)
        self.assertIn(
            "required stable no-media RDX disk LUN did not",
            self.update_script,
        )

    def test_updater_revalidates_selected_hardware_before_first_write(self):
        """Bind a generic selection to one port and close the pre-write race."""

        self.assertIn("function Confirm-RdxSelectedTarget", self.update_script)
        self.assertIn("function Assert-RdxEmptyBay", self.update_script)
        empty_bay = self.update_script[
            self.update_script.index("function Assert-RdxEmptyBay"):
            self.update_script.index("function Get-RdxMountedVolumes")
        ]
        self.assertIn("[bool]$Disk.MediaLoaded", empty_bay)
        self.assertIn("[UInt64]$Disk.Size -ne 0", empty_bay)
        self.assertIn("$volumes.Count -ne 0", empty_bay)
        self.assertIn("function Test-RdxStorageDiskNoMedia", empty_bay)
        self.assertIn("Get-Disk -Number ([UInt32]$Disk.Index)", empty_bay)
        self.assertIn("$storageSerial.Equals($expectedSerial", empty_bay)
        self.assertIn("$operationalStatus -contains 'No Media'", empty_bay)
        self.assertIn("([UInt64]$storageDisk.Size -eq 0)", empty_bay)
        self.assertIn(
            "[bool]$Disk.MediaLoaded -and -not $storageReportsNoMedia",
            empty_bay,
        )
        validation_exit = self.update_script[
            self.update_script.index(
                "if (-not $Update -and -not $InstallOpenRDX"
            ):
            self.update_script.index(
                "Assert-RdxEmptyBay -Disk $target",
                self.update_script.index(
                    "if (-not $Update -and -not $InstallOpenRDX"
                ),
            ) + len("Assert-RdxEmptyBay -Disk $target")
        ]
        self.assertIn("Assert-RdxEmptyBay -Disk $target", validation_exit)
        self.assertIn("$selectedTargetPnpDeviceId", self.update_script)
        self.assertIn("&MI_", self.update_script)
        self.assertIn(
            "The selected RDX disk identity changed before transfer",
            self.update_script,
        )

        image_validated = self.update_script.index(
            'Write-Host "Validated image: $ImagePath"'
        )
        identity_rechecked = self.update_script.index(
            "$target = Confirm-RdxSelectedTarget", image_validated
        )
        bay_rechecked = self.update_script.index(
            "Assert-RdxEmptyBay -Disk $target", identity_rechecked
        )
        bypass_called = self.update_script.index(
            "Enable-RdxCompatibilityHeaderCheckBypass", identity_rechecked
        )
        transfer_started = self.update_script.index(
            "Beginning Manager-compatible WRITE BUFFER transfer", bypass_called
        )
        self.assertLess(image_validated, identity_rechecked)
        self.assertLess(identity_rechecked, bay_rechecked)
        self.assertLess(bay_rechecked, bypass_called)
        self.assertLess(bypass_called, transfer_started)

        # Product/revision prefixes describe a compatible family. No concrete
        # unit suffix may be embedded as a selection shortcut.
        firmware_prefix = re.search(
            r"\$compatibilityDiskPnpPrefix\s*=\s*'(?P<value>[^']+)'",
            self.update_script,
        )
        self.assertIsNotNone(firmware_prefix)
        self.assertTrue(firmware_prefix.group("value").endswith("\\"))
        self.assertNotRegex(
            self.update_script,
            r"REV_0283\\[0-9A-Za-z]+&0'",
        )

    def test_update_is_power_loss_resistant_at_activation_boundary(self):
        """The boot vector is withheld until validation and activation."""
        program_payload = self.protocol.index("rdx_program_container_payload")
        withhold = self.protocol.index("rdx_update.first_vector_word", program_payload)
        validate = self.protocol.index("rdx_update.image_valid =")
        commit = self.protocol.rindex("rdx_program_flash(0U, rdx_update.first_vector_word")
        self.assertLess(withhold, validate)
        self.assertLess(validate, commit)
        self.assertIn("case OpcodeSectorErase:", self.spi)
        self.assertIn("rdx_update_reset_ticks = RDX_UPDATE_RESET_DELAY_TICKS;", self.protocol)
        tick = self.protocol[
            self.protocol.index("void rdx_manager_protocol_tick(void)"):
            self.protocol.index("rdx_manager_build_media_id_vpd")
        ]
        self.assertIn("usb_hal_disconnect();", tick)
        self.assertIn("system_reset();", tick)
        self.assertIn("rdx_manager_protocol_tick();", self.rti)


if __name__ == "__main__":
    unittest.main()
