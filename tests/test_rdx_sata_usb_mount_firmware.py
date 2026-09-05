# SPDX-FileCopyrightText: 2026 André Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Structural tests for the OpenRDX SATA-to-USB mount firmware."""

from pathlib import Path
import re
import struct
import unittest


PROJECT_ROOT = Path(__file__).resolve().parents[1]
SOURCE_ROOT = PROJECT_ROOT / "src" / "rdx_mount"


class RdxSataUsbMountFirmwareTests(unittest.TestCase):
    """Verify the OpenRDX USB mount firmware structure."""

    def test_build_uses_only_the_openrdx_mount_tree(self):
        """Compile the maintained implementation without external payloads."""
        adapter = (PROJECT_ROOT / "scripts" / "ti_cgt_build.py").read_text(
            encoding="utf-8"
        )

        self.assertIn('SOURCE_DIR = PROJECT_DIR / "src" / "rdx_mount"', adapter)
        self.assertIn(
            'PROJECT_INCLUDE = PROJECT_DIR / "include" / "rdx_mount"', adapter
        )
        self.assertNotIn("firmware_absolute_symbols.cmd", adapter)
        self.assertNotIn("payload.bin", adapter)

    def test_production_build_uses_required_ti_optimization(self):
        """Keep the interrupt-fed MWW path at its validated -O3 setting."""
        adapter = (PROJECT_ROOT / "scripts" / "ti_cgt_build.py").read_text(
            encoding="utf-8"
        )

        self.assertIn('common + ["-O3", str(source_path)]', adapter)
        self.assertNotIn('common + ["-O0", str(source_path)]', adapter)

    def test_main_discovers_sata_before_usb_publication(self):
        """Initialize both transports, discover media, then publish USB."""
        source = (SOURCE_ROOT / "main.c").read_text(encoding="utf-8")
        self.assertNotIn("hid_init();", source)
        usb_stack = source.index("usb_stack_init();")
        bot = source.index("ums_bot_init();", usb_stack)
        uas = source.index("ums_uas_init();", bot)
        sata = source.index("ahci_init();", uas)
        publish = source.index("usb_hal_connect();", sata)
        self.assertLess(usb_stack, bot)
        self.assertLess(bot, uas)
        self.assertLess(uas, sata)
        self.assertLess(sata, publish)

    def test_datapath_and_shared_transport_initialization(self):
        """Initialize transfer RAM before the shared BOT and UAS buffers."""
        main = (SOURCE_ROOT / "main.c").read_text(encoding="utf-8")
        fill = main.index(
            "ti_memset((void *)datapath_ram, 0xEE, DATAPATH_RAM_SIZE);"
        )
        mww = main.index("mww_init();", fill)
        bot = main.index("ums_bot_init();", mww)
        uas = main.index("ums_uas_init();", bot)

        self.assertLess(fill, mww)
        self.assertLess(bot, uas)

    def test_datapath_dma_layout_is_compiler_enforced(self):
        """Keep every DMA buffer at its hardware-visible fixed offset."""
        header = (
            PROJECT_ROOT / "include" / "rdx_mount" / "tusb9260.h"
        ).read_text(encoding="utf-8")

        self.assertIn("#define EVNT_BUFFER_SIZE    512", header)
        self.assertIn("#define AHCI_NCQ_DEPTH      8", header)
        self.assertIn("#define UMS_UAS_CMD_DEPTH   (AHCI_NCQ_DEPTH + 1)", header)
        self.assertIn("sizeof(AHCI_MEMORY_T) == 0x0A00U", header)
        self.assertIn("trb_ring_IN) == 0x11290U", header)
        self.assertIn("trb_ring_OUT) == 0x11340U", header)
        self.assertIn("ums_cmd_buffer) == 0x113D0U", header)
        self.assertIn("ums_status_buffer) == 0x114F0U", header)
        self.assertIn("normal_data_buffer) == 0x11610U", header)
        self.assertIn("scsi_response_buffer) == 0x12610U", header)
        self.assertIn("#define SCSI_RESPONSE_BUFF_SIZE  0x1014", header)

        for source_name in ("ahci.c", "rdx_unlock.c", "scsi.c", "ums_bot.c"):
            source = (SOURCE_ROOT / source_name).read_text(encoding="utf-8")
            self.assertNotIn("wrap_window_memory_placeholder", source)

    def test_callback_ring_depth_is_preserved(self):
        """Keep all 16 entries required by the asynchronous ATA path."""
        header = (
            PROJECT_ROOT / "include" / "rdx_mount" / "ahci.h"
        ).read_text(encoding="utf-8")

        self.assertIn("#define ATA_CALLBACK_QUEUE_DEPTH   16", header)
        self.assertNotIn(
            "#define ATA_CALLBACK_QUEUE_DEPTH   (AHCI_NCQ_DEPTH * 3)",
            header,
        )

    def test_main_services_one_ata_callback_per_pass(self):
        """Yield to USB and BOT processing after each ATA completion."""
        source = (SOURCE_ROOT / "main.c").read_text(encoding="utf-8")
        single_port = source[source.index("#if (NUM_AHCI_PORTS == 1)") :]
        single_port = single_port[: single_port.index("#else")]

        self.assertIsNone(re.search(r"\bdo\s*\{", single_port))
        self.assertNotIn("while (ata_dev[0].callback_pending", single_port)
        self.assertEqual(single_port.count("pAtaCallbackQueue["), 1)
        callback_pending_positions = [
            match.start()
            for match in re.finditer(r"callback_pending\[", single_port)
        ]
        self.assertEqual(len(callback_pending_positions), 2)
        self.assertLess(
            single_port.index("pAtaCallbackQueue["),
            single_port.index("WRITE_REG32(VIM_REQMASKSET0"),
        )
        self.assertLess(
            single_port.index("WRITE_REG32(VIM_REQMASKSET0"),
            callback_pending_positions[1],
        )

    def test_smart_enable_follows_successful_f2(self):
        """Enable SMART after access and metadata, before selecting RDX."""
        access = (SOURCE_ROOT / "rdx_unlock.c").read_text(encoding="utf-8")
        media = (SOURCE_ROOT / "sata_media.c").read_text(encoding="utf-8")
        unlock_start = access.index("RDX_ACCESS_RESULT_T rdx_unlock_media(")
        unlock_end = access.index("\n}\n", unlock_start) + 3
        unlock_body = access[unlock_start:unlock_end]
        select_start = media.index("static BOOLEAN_T sata_media_select_rdx(")
        select_end = media.index("\n}\n", select_start) + 3
        select_body = media[select_start:select_end]

        self.assertLess(
            unlock_body.index("rdx_issue_security_unlock(port_num, password)"),
            unlock_body.index("rdx_finish_media_mount(port_num, device)"),
        )
        self.assertLess(
            select_body.index("sata_media_enable_smart(port_num, device)"),
            select_body.index(
                "sata_media_kind[port_num] = SATA_MEDIA_KIND_RDX;"
            ),
        )
        self.assertIn("#define SATA_ATA_SMART", media)
        self.assertIn("#define SATA_ATA_SMART_ENABLE", media)
        self.assertIn("ata_cmd.fis.LBA_mid = SATA_ATA_SMART_LBA_MID;", media)
        self.assertIn("ata_cmd.fis.LBA_high = SATA_ATA_SMART_LBA_HIGH;", media)

    def test_sync_executor_is_shared_without_extra_tfd_wait(self):
        """Share one bounded executor for unlock, metadata reads, and SMART."""
        source = (SOURCE_ROOT / "rdx_unlock.c").read_text(encoding="utf-8")
        helper_start = source.index(
            "rdx_execute_initialization_sync_command(\n"
            "    UINT32_T port_num"
        )
        helper_end = source.index("\n}\n", helper_start) + 3
        helper = source[helper_start:helper_end]

        self.assertIn(
            "ahci_wait_complete(\n"
            "            PxCI(port_num), 0x01U, 0U, RDX_SYNC_TIMEOUT_MS)",
            helper,
        )
        self.assertIn("READ32(PxTFD(port_num)) & PTFD_STS_FAILURE_MASK", helper)
        self.assertIn("RDX_INITIALIZATION_COMMAND_TRANSPORT_FAILURE", helper)
        self.assertIn("RDX_INITIALIZATION_COMMAND_REJECTED", helper)
        self.assertIn("RDX_INITIALIZATION_COMMAND_SUCCESS", helper)
        self.assertNotIn("PTFD_STS_BSY_BIT", source)
        self.assertNotIn("PTFD_STS_DRQ_BIT", source)
        self.assertNotIn("30000", source)
        # Port initialization calls the helper with interrupts already
        # disabled. Foreground SMART and eject sessions wrap the helper with
        # their own PxIE/PxIS ownership checks so the ISR cannot race it.
        self.assertNotIn("PxIE(port_num)", helper)
        self.assertNotIn("PxIS(port_num)", helper)
        self.assertEqual(
            source.count(
                "rdx_execute_initialization_sync_command(port_num, &ata_cmd)"
            ),
            2,
        )
        media = (SOURCE_ROOT / "sata_media.c").read_text(encoding="utf-8")
        smart_start = media.index("static BOOLEAN_T sata_media_enable_smart(")
        smart_end = media.index("\n}\n", smart_start) + 3
        smart = media[smart_start:smart_end]
        self.assertIn(
            "sata_media_execute_sync_command(port_num, &ata_cmd)", smart
        )

    def test_store_forward_diagnostic_respects_real_buffer_size(self):
        """Limit the diagnostic path to the allocated 4-KiB RDX buffer."""
        scsi = (SOURCE_ROOT / "scsi.c").read_text(encoding="utf-8")

        self.assertIn(
            "sizeof(datapath_ram->normal_data_buffer) /\n"
            "                    ata_dev[scsi_cmd.pCmdInput->bLUN].dTrueSectorSize",
            scsi,
        )
        self.assertNotIn("scsi_cmd.dXferLength = 0x80;", scsi)

    def test_metadata_extent_controls_capacity_and_host_lbas(self):
        """Expose only the checksummed RDX user-data extent to the host."""
        unlock = (SOURCE_ROOT / "rdx_unlock.c").read_text(encoding="utf-8")
        media = (SOURCE_ROOT / "sata_media.c").read_text(encoding="utf-8")
        scsi = (SOURCE_ROOT / "scsi.c").read_text(encoding="utf-8")

        self.assertIn("#define RDX_METADATA_FRONT_SEED      0x12345678U", unlock)
        self.assertIn("#define RDX_METADATA_REAR_SEED       0x87654321U", unlock)
        self.assertIn("#define RDX_METADATA_REAR_DISTANCE   0x0FFFU", unlock)
        self.assertIn("0x04U, 0x06U, 0x0BU", unlock)
        self.assertIn("RDX_METADATA_TYPE_LBA32      0x02U", unlock)
        self.assertIn("RDX_METADATA_TYPE_LBA64      0x1BU", unlock)
        self.assertIn(
            "rear_lba = last_lba - RDX_METADATA_REAR_DISTANCE + relative_lba;",
            unlock,
        )
        self.assertIn(
            "device->ddMaxLBA = record.second - record.first + 1U;", unlock
        )
        self.assertIn(
            "logical_lba + rdx_media_lba_offset[port_num]", unlock
        )
        self.assertIn("rdx_translate_media_lba(port_num, logical_lba)", media)
        self.assertIn("sata_media_translate_lba(", scsi)
        self.assertIn("ata_dev[scsi_cmd.pCmdInput->bLUN].ddMaxLBA", scsi)

    def test_metadata_checksum_reference_sector(self):
        """Exercise the four independent metadata checksum lanes."""
        sector = bytearray(512)
        sector[4:16] = bytes.fromhex("02010c0010000000efbeadde")
        seed = 0x12345678
        lanes = bytearray(struct.pack("<I", seed))

        for index, value in enumerate(sector[:508]):
            lane = index & 3
            lanes[lane] = (lanes[lane] + value) & 0xFF
        sector[508:512] = lanes

        self.assertEqual(
            struct.unpack("<I", sector[508:512])[0],
            0xF0ED1579,
        )

    def test_usb_connect_has_no_invented_ata_authorization_gate(self):
        """USB publication must remain independent of ATA media tests."""
        source = (SOURCE_ROOT / "usb_hal.c").read_text(encoding="utf-8")
        connect = re.search(
            r"void usb_hal_connect\(void\)\s*\{(?P<body>.*?)\n\}",
            source,
            re.DOTALL,
        )

        self.assertIsNotNone(connect)
        self.assertNotIn("rdx_auth", connect.group("body"))

    def test_hotplug_reinitializes_ahci_without_detaching_removable_usb(self):
        """Keep the RDX USB LUN stable while AHCI follows cartridge changes."""
        source = (SOURCE_ROOT / "ahci.c").read_text(encoding="utf-8")
        header = (PROJECT_ROOT / "include" / "rdx_mount" / "ahci.h").read_text(
            encoding="utf-8"
        )

        self.assertRegex(
            header,
            r"#define\s+REMOVABLE_MEDIA_DEVICE\s+1\b",
        )
        self.assertIn("usb_hal_disconnect();", source)
        self.assertIn(
            "ahci_init_port(port_num);",
            source,
        )

    def test_unlock_has_no_model_or_removable_media_filter(self):
        """Use protocol state rather than a disk-specific model shortcut."""
        source = (SOURCE_ROOT / "rdx_unlock.c").read_text(encoding="utf-8")

        self.assertNotIn("bRemovableMediaDevice", source)
        self.assertNotIn("MQ01", source)
        self.assertNotIn("TOSHIBA", source)

    def test_media_policy_runs_after_features_and_gates_media_ready(self):
        """Keep USB published but expose media only after policy admission."""
        source = (SOURCE_ROOT / "ahci.c").read_text(encoding="utf-8")
        set_features = source.index(
            "status = ahci_set_features_xfer_mode(port_num, FALSE);"
        )
        admission = source.index("sata_media_prepare(port_num", set_features)
        ready = source.index("bDeviceInitComplete = TRUE;", admission)

        self.assertLess(set_features, admission)
        self.assertLess(admission, ready)
        self.assertIn("!sata_media_prepare", source)

    def test_f2_path_does_not_invent_an_identify_security_gate(self):
        """Do not add an IDENTIFY-word shortcut to the F2 protocol path."""
        source = (SOURCE_ROOT / "rdx_unlock.c").read_text(encoding="utf-8")

        self.assertNotIn("RDX_ATA_SECURITY_STATUS_WORD", source)
        self.assertNotIn("RDX_ATA_SECURITY_SUPPORTED", source)
        self.assertNotIn("RDX_ATA_SECURITY_ENABLED", source)
        self.assertNotIn("RDX_ATA_SECURITY_LOCKED", source)
        self.assertNotIn("RDX_ATA_SECURITY_FROZEN", source)
        self.assertNotIn("RDX_ATA_SECURITY_EXPIRED", source)
        self.assertIn("device->bPacketDevice", source)

    def test_link_reinitialization_reissues_f2_sequence(self):
        """Do not reuse authentication state across an RDX SATA link reset."""
        source = (SOURCE_ROOT / "rdx_unlock.c").read_text(encoding="utf-8")

        self.assertNotIn("rdx_unlock_attempted", source)
        self.assertNotIn("rdx_unlock_result", source)
        self.assertIn(
            "command_result = rdx_issue_security_unlock(port_num, password);",
            source,
        )
        self.assertIn(
            "if (command_result == RDX_INITIALIZATION_COMMAND_REJECTED)",
            source,
        )

    def test_password_known_answer_vector(self):
        """Detect changes to the model, capacity, and serial transform."""
        delta = 0x9E3779B9
        key = bytes.fromhex("42fba7fb2b0932005e1bbec2db824516")
        model = b"TOSHIBA MQ01ABD100".ljust(40)
        serial = b"12345678901234567890"
        max_lba = 625137345
        material = model + struct.pack("<Q", max_lba)

        def encrypt(block, word_count, key_bytes):
            words = list(struct.unpack("<{}I".format(word_count), block))
            key_words = struct.unpack("<4I", key_bytes)
            previous = words[-1]
            total = 0
            for _ in range(52 // word_count + 6):
                total = (total + delta) & 0xFFFFFFFF
                for position in range(word_count):
                    index = ((total & 0x0F) >> 2) ^ (position & 3)
                    mixed = (total + key_words[index]) ^ (
                        previous
                        + (((previous << 4) & 0xFFFFFFFF) ^ (previous >> 5))
                    )
                    words[position] = (words[position] + mixed) & 0xFFFFFFFF
                    previous = words[position]
            return struct.pack("<{}I".format(word_count), *words)

        for offset in range(0, 48, 16):
            key = encrypt(material[offset : offset + 16], 4, key)
        password = encrypt(b"Password:" + serial + bytes(3), 8, key)

        self.assertEqual("6ddd96857281afc21864ef05b457b3c0", key.hex())
        self.assertEqual(
            "f7c30660db8f8496c8e0f1b6dd529bef"
            "0a4b884db9b7fd59b7d624b618bff057",
            password.hex(),
        )

        source = (SOURCE_ROOT / "rdx_unlock.c").read_text(encoding="utf-8")
        self.assertIn("&device->ddTrueMaxLBA", source)
        self.assertNotIn("0x54U, 0xF4U", source)

    def test_embedded_seed_unwrap_reference_vector(self):
        """Verify the clear seed produced by the stored key permutation."""
        stored_delta = bytes.fromhex("0dce8df5")
        clear_delta = bytes(
            (value - 0x54 - index) & 0xFF
            for index, value in enumerate(stored_delta)
        )
        delta = struct.unpack("<I", clear_delta)[0]
        wrapped = bytes.fromhex("1018d6b652bd209b2c64aa3476086cf4")
        table = bytes.fromhex("18d671947b215631756e8f12f96c2943")
        permutation = (9, 12, 5, 1, 10, 3, 13, 7, 6, 15, 11, 0, 4, 8, 2, 14)
        key = bytes(table[index] for index in permutation)
        words = list(struct.unpack("<4I", wrapped))
        key_words = struct.unpack("<4I", key)
        total = (delta * (52 // len(words) + 6)) & 0xFFFFFFFF

        while total:
            selector = (total >> 2) & 3
            for position in range(len(words) - 1, 0, -1):
                previous = words[position - 1]
                mixed = (total + key_words[(position & 3) ^ selector]) ^ (
                    previous
                    + (((previous << 4) & 0xFFFFFFFF) ^ (previous >> 5))
                )
                words[position] = (words[position] - mixed) & 0xFFFFFFFF
            previous = words[-1]
            mixed = (total + key_words[selector]) ^ (
                previous
                + (((previous << 4) & 0xFFFFFFFF) ^ (previous >> 5))
            )
            words[0] = (words[0] - mixed) & 0xFFFFFFFF
            total = (total - delta) & 0xFFFFFFFF

        self.assertEqual(
            "42fba7fb2b0932005e1bbec2db824516",
            struct.pack("<4I", *words).hex(),
        )
        self.assertEqual(0x9E3779B9, delta)

        source = (SOURCE_ROOT / "rdx_unlock.c").read_text(encoding="utf-8")
        self.assertIn("0x9E3779B9U", source)
        self.assertNotIn("0xF58DCE0DU", source)
        self.assertIn(
            "0xFBA7FB42U, 0x0032092BU, 0xC2BE1B5EU, 0x164582DBU",
            source,
        )

    def test_rdx_pwm_outputs_are_owned_by_rdx_hardware(self):
        """Keep PWM0 on the fan and PWM1 on the motor, never TI LEDs."""
        main = (SOURCE_ROOT / "main.c").read_text(encoding="utf-8")
        pwm = (SOURCE_ROOT / "pwm.c").read_text(encoding="utf-8")
        rti = (SOURCE_ROOT / "rti.c").read_text(encoding="utf-8")
        hardware = (SOURCE_ROOT / "rdx_hardware.c").read_text(encoding="utf-8")
        mechanism = (SOURCE_ROOT / "rdx_mechanism.c").read_text(encoding="utf-8")

        self.assertIn("rdx_hardware_init();", main)
        self.assertIn("rdx_hardware_service();", main)
        self.assertNotIn("pwm_init(HDD_ACTIVITY_LED_PWM_NUM);", main)
        self.assertNotIn("pwm_", rti)
        self.assertIn(
            "pwm_run(RDX_FAN_PWM_NUM, duty, RDX_FAN_PWM_PERIOD_US);",
            hardware,
        )
        self.assertIn(
            "pwm_run(RDX_MOTOR_PWM_NUM, 100U, RDX_MOTOR_PWM_PERIOD_US);",
            mechanism,
        )
        self.assertIn("WRITE32(PWM_PH1D_REG_OFF(pwm_num), 0U);", pwm)
        self.assertIn("WRITE32(PWM_CFG_REG_OFF(pwm_num), PWM_CFG_RDX_IDLE);", pwm)

    def test_usb_and_scsi_identity_is_stable(self):
        """Expose the stable removable RDX bridge identity, not the SATA disk."""
        usb = (SOURCE_ROOT / "usb_chap9.c").read_text(encoding="utf-8")
        scsi = (SOURCE_ROOT / "scsi.c").read_text(encoding="utf-8")
        ahci = (PROJECT_ROOT / "include" / "rdx_mount" / "ahci.h").read_text(
            encoding="utf-8"
        )

        self.assertIn("0x5A,0x1A", usb)
        self.assertIn("0x05,0x00", usb)
        self.assertIn("0x83,0x02", usb)
        self.assertIn("'T', 0, 'A', 0, 'N', 0, 'D', 0", usb)
        self.assertIn("'R', 0, 'D', 0, 'X', 0", usb)
        # The removable-target path keeps the dock's Windows disk PDO stable
        # while an empty bay reports MEDIUM NOT PRESENT.
        self.assertIn("#define REMOVABLE_MEDIA_DEVICE   1", ahci)
        self.assertIn("scsi_resp_buff[1] = RMB_BIT;", scsi)
        self.assertIn("rdx_vendor_id[8]", scsi)
        self.assertIn("rdx_product_id[16]", scsi)

    def test_usb_serial_uses_the_manufacturing_unit_identity(self):
        """Publish the receiver-specific unit serial with its USB prefix."""
        usb = (SOURCE_ROOT / "usb_chap9.c").read_text(encoding="utf-8")
        helper_signature = "static void rdx_apply_unit_serial_to_usb_descriptor("
        self.assertIn(helper_signature, usb)
        helper_start = usb.index(helper_signature)
        helper_end = usb.index("\n}\n", helper_start) + 3
        helper = usb[helper_start:helper_end]
        process_start = usb.index("void process_usb_descriptors(void)")
        process_end = usb.index("\n}\n", process_start) + 3
        process = usb[process_start:process_end]

        self.assertIn('#include "rdx_manager_identity.h"', usb)
        self.assertIn("identity = rdx_manager_get_drive_identity();", helper)
        self.assertIn("serial_ptr[0] = '0';", helper)
        self.assertIn("serial_ptr[1] = 0xCAU;", helper)
        self.assertIn("serial_ptr[2] = '0';", helper)
        self.assertIn("index < sizeof(identity->serial)", helper)
        self.assertIn(
            "serial_ptr[(index + 2U) * 2U] = identity->serial[index];",
            helper,
        )
        self.assertIn(
            "serial_ptr[((index + 2U) * 2U) + 1U] = 0U;",
            helper,
        )
        identity_index = process.index(
            "usb_dev.bSerialNumStringDescIndex = desc_ptr[16];"
        )
        apply_index = process.index(
            "rdx_apply_unit_serial_to_usb_descriptor(\n"
            "            usb_dev.bSerialNumStringDescIndex);"
        )
        self.assertLess(identity_index, apply_index)

    def test_usb_product_and_bos_descriptors_match_required_layout(self):
        """Keep the Chapter 9 descriptor fields and padding stable."""
        source = (SOURCE_ROOT / "usb_chap9.c").read_text(encoding="utf-8")
        product = re.search(
            r"/\* Product:.*?\*/\s*"
            r"\(2\+32\),\s*USB_DT_STRING,"
            r"(?P<payload>.*?)"
            r"/\* Serial template\.",
            source,
            re.DOTALL,
        )

        self.assertIsNotNone(product)
        characters = re.findall(
            r"'((?:\\.|[^']))'\s*,\s*(?:0|0x[0-9A-Fa-f]+)",
            product.group("payload"),
        )
        self.assertEqual("RDX".ljust(16), "".join(characters))
        self.assertRegex(
            source,
            r"0x0A,\s*0x00,\s*/\*[^*]*U2 exit latency[^*]*\*/",
        )


if __name__ == "__main__":
    unittest.main()
