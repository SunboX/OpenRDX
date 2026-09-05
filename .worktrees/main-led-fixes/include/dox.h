/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/*!
 * @file
 * @mainpage TUSB9261 RDX Firmware Functional Specification
 *
 * @section overview Overview
 * This project contains the TUSB9261 RDX firmware implementation. PlatformIO
 * orchestrates the build while TI ARM Code Generation Tools compile, assemble,
 * link, and convert the firmware image.
 *
 * @section architecture Architecture
 * The firmware coordinates USB BOT/UAS/HID handling, SCSI command processing,
 * AHCI/SATA transfers, SPI access, RTI timing, watchdog service, PWM/GIO
 * activity indication, and interrupt dispatch.
 *
 * @image html hw_arch.png "TUSB926x hardware architecture (TI reference figure)"
 * @image html fw_arch.png "TUSB926x firmware architecture (TI reference figure)"
 * @image html usb_desc.png "USB descriptor relationships (TI reference figure)"
 *
 * @section build Build constraints
 * The source requires the TI ARM9 ABI, TI assembly syntax, TI linker
 * command syntax, and the fixed target addresses defined by the project.
 */
