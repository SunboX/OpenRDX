/*
 * SPDX-FileCopyrightText: 2019 Texas Instruments Incorporated
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*!
 * @file
 *
 * This file contains the Doxygen comments that create the
 * TUSB926x Firmware Functional Specification.
 *
 * @page intro_chapter Introduction
 *
 * The TUSB926x is a USB 3.0/2.0 to Serial ATA Bridge
 * Controller with an integrated ARM Cortex-M3 microcontroller.
 * The TUSB926x software consists of two separate parts:
 * bootcode which resides in ROM on the device and operational
 * firmware which is stored on external flash memory.  The
 * bootcode executes upon power-on reset and copies the device
 * firmware from external flash memory into internal device RAM
 * for execution.  This document describes the operational
 * firmware only.
 *
 * @section req_sec Supported Features
 *
 * The firmware supports the following functionality:  
 * - USB 3.0 SuperSpeed and USB 2.0 High-Speed and Full-Speed.
 * - USB Mass Storage Class (MSC) - USB Attached SCSI (UAS).
 * - USB Mass Storage Class (MSC) - Bulk-Only Transport (BOT) - including the 13 error cases.
 * - USB Mass Storage Specification for Bootability.
 * - USB Device Class Definition for Human Interface Devices (HID) - for firmware update.
 * - Serial ATA Advanced Host Controller Interface (SATA AHCI).
 * - Pulse Width Modulation (PWM) - for LED dimming control. 
 * - General Purpose Input/Output (GPIO) - for LED control & customer specific functions (i.e. one-touch backup).
 * - Serial Peripheral Interface (SPI) - for writing external SPI flash.
 * - Serial Communications Interface (SCI) - for debug output.
 *
 * @section ref_sec References
 *
 * - Armv7-M Architecture Reference Manual
 * - AT Attachment with Packet Interface - 6 (ATA/ATAPI-6), Revision 3b AT
 * - Attachment 8 - ATA/ATAPI Command Set (ATA8-ACS), Revision 6
 * - Cortex - M3 Technical Reference Manual, r2p0
 * - Multi-Media Commands 5 (MMC-5), Revision 4
 * - SCSI/ATA Translation 2 (SAT-2), Revision 9
 * - SCSI Block Commands 3 (SBC-3), Revision 16
 * - SCSI Primary Commands 4 (SPC-4), Revision 16
 * - Serial ATA Advanced Host Controller Interface (AHCI), Revision 1.1
 * - Serial ATA Specification, Revision 2.6
 * - Universal Serial Bus 2.0 Specification
 * - Universal Serial Bus 3.0 Specification
 * - USB Attached SCSI (UAS) [T10/2095-D], Revision 4
 * - USB Device Class Definition for Human Interface Devices (HID)
 * - USB Device Class Specification for Device Firmware Update
 * - USB Mass Storage Class Bulk-Only Transport (BOT), Revision 1.0
 * - USB Mass Storage Class USB Attached SCSI Protocol (UASP), Revision 1.0
 * - USB Mass Storage Class Compliance Specification, Revision 0.9a
 * - USB Mass Storage Class Specification Overview, Revision 1.4
 * - USB Mass Storage Specification for Bootability, Revision 1.0
 *
 * @section def_sec Definitions
 *
 *    @b AHCI    -  Advanced Host Controller Interface - a hardware mechanism that serves as an engine to move data between system memory and SATA devices and supports new SATA features such as NCQ and hot-plugging.
 * @n @b ATA	 -  Advanced Technology Attachment - a parallel interface standard for connecting storage drives to computers.
 * @n @b ATAPI   -  ATA Packet Interface - a protocol that allows the ATA interface to carry SCSI commands/responses embedded in packets.
 * @n @b BOS	 -  Binary device Object Store - a USB descriptor that describes device-level capabilities.
 * @n @b BOT	 -  Bulk-Only Transport - a USB mass storage class.
 * @n @b CBW	 -  Command Block Wrapper - a packet containing a command block for BOT.
 * @n @b CDB	 -  Command Descriptor Block.
 * @n @b CL	     -  Command List - memory structure defining up to 32 ATA/ATAPI command headers for SATA AHCI
 * @n @b CSW	 -  Command Status Wrapper - a packet containing the status of a command block for BOT.
 * @n @b CT	     -  Command Table - used for SATA AHCI.
 * @n @b D2H	 -  Device to Host.
 * @n @b DFU	 -  Device Firmware Update - a USB device class.
 * @n @b DMA	 -  Direct Memory Access.
 * @n @b EP	     -  Endpoint - typically followed by a number to designate the endpoint.
 * @n @b FIS	 -  Frame Information Structure - a packet transferred between host and device for SATA AHCI.
 * @n @b GIO     -  General-Purpose Input/Output.
 * @n @b H2D	 -  Host to Device.
 * @n @b HAL	 -  Hardware Abstraction Layer.
 * @n @b HBA	 -  Host Bus Adapter (refers to SATA AHCI host).                                                                                                            
 * @n @b HDD	 -  Hard Disk Drive.                                                                                                                                      
 * @n @b HID	 -  Human Interface Device - a USB device class.                                                                                                          
 * @n @b IU	     -  Information Unit - a packet containing command or status information for UAS.                                                                         
 * @n @b LUN	 -  Logical Unit Number.                                                                                                                                  
 * @n @b MCU	 -  Micro-Controller Unit.                                                                                                                                
 * @n @b MSC	 -  Mass Storage Class.                                                                                                                               
 * @n @b NVIC    -  Nested Vectored Interrupt Controller - an integrated feature of the ARM Cortex-M3 microcontroller.                                                
 * @n @b PDT	 -  Peripheral Device Type - as defined by SCSI Primary Commands specification.                                                                       
 * @n @b PRDT    -  Physical Region Descriptor Table - a scatter/gather DMA list for SATA AHCI.                                                                       
 * @n @b PWM	 -  Pulse Width Modulation.                                                                                                                            
 * @n @b RTI	 -  Real-Time Interrupt.                                                                                                                              
 * @n @b SATA    -  Serial ATA.                                                                                                                                       
 * @n @b SCI	 -  Serial Communications Interface.                                                                                                                  
 * @n @b SCSI    -  Small Computer System Interface - a set of standards for transferring data between computers and peripheral devices.                              
 * @n @b SID	 -  Stream ID.                                                                                                                                            
 * @n @b SPI	 -  Serial Peripheral Interface - a four-wire, full-duplex synchronous serial data bus.                                                                   
 * @n @b TAG	 -  (Initiator Port Transfer) Tag.                                                                                                                        
 * @n @b TRB	 -  Transfer Request Block - a DMA descriptor structure.                                                          
 * @n @b UAS	 -  USB Attached SCSI - a USB mass storage class which is designed to take advantage of the dual simplex design of USB 3.0 for higher bus utilization than BOT.
 * @n @b USB	 -  Universal Serial Bus.                                                                                                                                 
 * @n @b VIM     -  Vectored Interrupt Manager.
 * @n @b VPD     -  Vital Product Data - contains SCSI device information.
 * 
 *
 * @page hw_arch_chapter Hardware Architecture
 *
 * Figure 2.1 shows a high-level overview of the TUSB926x HW architecture.  
 * The TUSB926x HW peripherals are controlled by the Cortex-M3 microcontroller through memory mapped registers.   
 * The high-speed datapath RAM provides virtual memory windows for DMA data transfers between the SATA and USB controllers.
 * 
 * @image html hw_arch.png "Figure 2.1: Hardware Architecture"
 * @image latex hw_arch.eps "Hardware Architecture"
 *
 * @page sw_arch_chapter Software Architecture
 *  
 * The firmware is primarily interrupt driven.  Mass storage layer callbacks are queued for SATA 
 * interrupts and executed inside a main loop to minimize SATA interrupt servicing time.  Interrupt 
 * nesting and prioritization are configured in the Vectored Interrupt Manager (VIM) to avoid missing 
 * critical interrupts. Figure 3.1 shows the basic TUSB926x firmware architecture.
 *
 * @image html fw_arch.png "Figure 3.1: Firmware Architecture"
 * @image latex fw_arch.eps "Firmware Architecture" width=4.25in
 * 
 * @section func_blocks_section Functional Block Descriptions
 *
 * @subsection usb_hal_subsect USB HAL
 * 
 * The USB HAL provides low-level functions for configuring endpoints, controlling data transfers, and 
 * handling power management.  The HAL executes USB stack function callbacks to process incoming packets.
 *
 * @subsection usb_stack_subsect USB Stack
 * The USB stack implements the standard USB 2.0 and USB 3.0 protocol defined in the Chapter 9 of the 
 * specification and passes unhandled packets along to the appropriate higher layer for processing using callbacks.  
 * During initialization, higher layers register callbacks with the USB stack.  When a mass storage interface is set 
 * by the host, the USB stack sets the transfer handler function pointers to the callbacks that correspond to the 
 * active mass storage class.  The USB endpoints are also reconfigured for BOT or UAS at this time.  
 * 
 * @subsection bot_subsect USB Mass Storage Class: Bulk-Only Transfer
 * The bulk-only transfer block implements the protocol for interpreting incoming command block wrapper (CBW) packets, 
 * handling data transfer, and responding with command status wrapper (CSW) packets.  The BOT block also implements the 
 * 13 error conditions as defined in the BOT specification.  The BOT block directly handles class-specific requests 
 * for "Bulk-only mass storage reset" and "Get max LUN" and passes SCSI commands to the SCSI block for processing.  
 * The max LUN number will be hardcoded to zero since only a single SATA drive is supported.
 * @n
 * According to the BOT specification, the host must wait for a CSW for any outstanding CBW before sending another 
 * CBW to the device.  Consequently, command queuing is not possible when operating as a BOT mass storage device.  
 * @n
 * Three endpoints are required to support BOT:
 *  - Default control endpoint (0) - for enumeration, max LUN determination, and mass storage reset.
 *  - Bulk OUT endpoint - for receiving CBWs and data from host.
 *  - Bulk IN endpoint - for sending CSWs and data to host.
 *
 * @subsection uas_subsect USB Mass Storage Class: USB Attached SCSI
 *
 * Compared to BOT, USB Attached SCSI (UAS) improves bus utilization for both USB 2.0 and USB 3.0 and takes advantage 
 * of the dual simplex capability of USB 3.0.  The UAS block implements the protocol for interpreting Command Information 
 * Units (IUs), returning Sense IUs for status, and handling data transfer.
 * @n
 * Five endpoints are required to support UAS:
 *  - Default control endpoint (0) - for enumeration.
 *  - Bulk OUT endpoint - for receiving commands from host
 *  - Bulk IN endpoint - for sending status to host.
 *  - Bulk OUT endpoint - for receiving data from host.
 *  - Bulk IN endpoint - for sending data to host.
 * 
 * The separate pipes for IUs and data allow the USB device to receive commands and return status while data is being 
 * transferred.  UAS supports queued transfers by allowing the host to send multiple command IUs without waiting for a 
 * response IU.  The firmware does not support bi-directional data transfers.
 *
 * @subsubsection uas_hs_subsubsect High-Speed UAS Operation
 * The host sends Command IUs to the target device.  A unique initiator port transfer tag is provided with each command 
 * to identify the buffers associated with the data and status pipes for each transfer.  When the target device is ready 
 * to service a Command IU with data phase, it returns a Read Ready IU or a Write Ready IU on the status pipe.  The Ready IU 
 * will contain the initiator port transfer tag corresponding to the command IU being serviced.  After the data transfer 
 * for the command has completed, the device returns a Sense IU with the same tag to the host on the status pipe.  The UAS 
 * block maintains a structure to correlate the AHCI command slots to the UAS initiator port transfer tags which are 
 * required to send the Read Ready, Write Ready, and Sense IUs.
 *
 * @subsubsection uas_ss_subsubsect Superspeed UAS Operation
 * For Superspeed USB, UAS utilizes the bulk streaming protocol which manipulates the stream ID (SID) field in the 
 * packet header to associate host buffers.  Streams improve the UAS protocol efficiency by eliminating the need for 
 * Read/Write Ready IUs.  The initiator port transfer tag from the Command IU is used as the SID on the data and status 
 * pipes.  The UAS block maintains a structure to correlate the AHCI command slots to the UAS initiator port transfer 
 * tags which are required to set the SID.
 *
 * @subsection scsi_subsect SCSI/ATA Translation
 * The SCSI/ATA translation layer handles SCSI commands from the active mass storage class (BOT or UAS).
 * For ATA devices, the SCSI block responds directly or translates commands to their ATA counterparts according to the SAT-2 specification.  
 * For ATAPI devices, all SCSI commands are passed directly to the device as PACKET commands. 
 * All SCSI commands required by the USB MSC Compliance Specification for bootable direct-access block devices (PDT = 0x0) are supported.
 * @n
 * The following SCSI commands are supported for ATA devices (i.e. HDD):
 *  - INQUIRY (standard, supported VPD pages page, unit serial number, device ID, ATA info, block limits, block device characteristics, and logical block provisioning VPD page)
 *  - READ FORMAT CAPACITIES
 *  - MODE SENSE (6 and 10-byte)
 *  - MODE SELECT (6 and 10-byte) 
 *  - READ CAPACITY(10 and 16-byte)
 *  - READ (6, 10, 12, and 16-byte)
 *  - REQUEST SENSE
 *  - TEST UNIT READY
 *  - WRITE (6, 10, 12, and 16-byte)
 *  - VERIFY (10, 12, and 16-byte)
 *  - START STOP UNIT
 *  - REPORT LUNS
 *  - PREVENT ALLOW MEDIUM REMOVAL
 *  - SYNCHRONIZE CACHE (10 and 16-byte)
 *  - UNMAP
 *  - SEND DIAGNOSTIC (default self-test only)
 *  - FORMAT UNIT (fmt_data=0, defect_list_format=0 only)
 *  - SECURITY PROTOCOL IN/OUT
 *  - ATA PASS-THROUGH (12 and 16-byte)
 * 
 * For ATAPI devices such as CD/DVD/BD-ROM, all SCSI commands are passed directly to the device so command support 
 * depends on the device.
 *
 * @subsection ahci_subsect Serial-ATA AHCI Host Driver
 *
 * The AHCI-compliant SATA driver handles initialization of the HBA and attached SATA device.  The driver supports 
 * NCQ with a depth up to 32 commands.  USB mass storage class (BOT or UAS) blocks register callbacks with the SATA 
 * driver to handle command completions, errors, and medium changes.  The interrupt controller calls short handler 
 * functions in the SATA driver which add the mass storage layer callback functions to a callback queue for execution 
 * in the main loop.  Because SATA commands can be queued and executed out of order, the active AHCI command slot is 
 * returned with the callback.
 * @n
 * The driver supports queued and non-queued commands for a single ATA or ATAPI device.  SATA devices that do not support 
 * DMA are not supported.  Aggressive link power management (PARTIAL and SLUMBER modes) is disabled by default but can be 
 * enabled via a macro option. SATA hot plug is supported and will cause a system reset after a SATA device connection is 
 * detected.  Hot plug and link power management cannot be supported simultaneously due to removal detection issues (see 
 * SATA AHCI v1.1 specification pg. 73). 
 * @n
 * The following optional SATA features are not supported:
 *  - Staggered spin-up
 *  - Port multiplication
 *  - Non-zero buffer offsets for DMA
 *  - I/O Priority for NCQ
 * 
 * The following ATA/ATAPI commands can be issued directly from the SATA driver:
 *  - IDENTIFY DEVICE
 *  - IDENTIFY PACKET DEVICE
 *  - SET FEATURES (PIO/DMA Transfer mode & DMA Setup FIS Auto-Activate optimization)
 *  - STANDBY IMMEDIATE
 * 
 * Non-standard logical and physical sector sizes are supported using IDENTIFY DEVICE information as defined in the ATA8-ACS 
 * specification.  The default logical sector size for HDDs is 512 bytes.  File systems often cluster sectors to create 
 * larger allocation units, typically 4 KB or eight 512-byte sectors, however this formatting is firmware independent.
 *
 * @subsection hid_subsect Human Interface Device Class
 *
 * The optional HID class block reports human interface data to the host when polled by a HID class driver.  The typical 
 * external HDD application is a user-initiated data backup where the user pushes a dedicated button on the external USB 
 * storage device to backup data from the host computer (a.k.a. "one-touch backup").  A GPIO on the TUSB926x is used to 
 * detect the push-button press.
 * @n
 * Two endpoints are required to support HID:
 *  - Default control endpoint (0) - for enumeration.
 *  - Interrupt IN endpoint - for sending data to host.
 *  - Interrupt OUT endpoint - for receiving data from host.
 * 
 * The IN interrupt endpoint is required to report the push-button status and can also be used to report debug information 
 * for internal test use.  An OUT endpoint is optional per the HID specification and is not required for the user-initiated 
 * data backup application.  However, the OUT endpoint can be used to receive Set Report requests for requesting data or 
 * updating the device firmware.
 *
 * 
 * @page usb_desc_chapter USB Descriptors
 *
 * Figure 4.1 shows the USB descriptors for the TUSB926x.  The HID and UAS descriptors are optional.
 *
 * @image html usb_desc.png "Figure 4.1: USB Descriptors"
 * @image latex usb_desc.eps "USB Descriptors"
 *
 * 
 */
