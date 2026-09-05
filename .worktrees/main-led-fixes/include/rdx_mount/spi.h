/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

/*****************************************************************************
* Filename    : spi.h
*
* Project     : TUSB926x Boot Loader.
*
* Description : SPI Register Defines.
*
*   (C) Copyright 2009 by Texas Instruments Incorporated.
*   All rights reserved.
*
* Revision History:
*   MM/DD/YY
*    06/25/09 - Kevin Harris - Creation / Added SPI model code to Boot Loader
*                              code base.  Altered code formatting to be
*                              consistent with the Boot Loader code base.
*
*****************************************************************************/

/*! @file
 * 
 * Header file for the Serial Peripheral Interface module driver
 *  
 */

#ifndef __SPI_H
#define __SPI_H

#include "tusb9260.h"
#include "tusb9260_types.h"

/*SPI register addresses*/
#define SPI_GCR0                  0xFFF7F800      /*global config 0*/
#define SPI_GCR1                  0xFFF7F804      /*global config 1*/
#define SPI_INT0                  0xFFF7F808      /*interrupt enable*/
#define SPI_LVL                   0xFFF7F80C
#define SPI_FLG                   0xFFF7F810      /*tx/rx flags*/
#define SPI_PC0                   0xFFF7F814      /*set SPI pin function*/
#define SPI_PC1                   0xFFF7F818      /*set SPI pin direction*/
#define SPI_PC2                   0xFFF7F81C      /*SPI pin data in*/
#define SPI_PC3                   0xFFF7F820      /*SPI pin data out*/
#define SPI_PC4                   0xFFF7F824      /*SPI pin data set*/
#define SPI_PC5                   0xFFF7F828      /*SPI pin data clear*/
#define SPI_PC7                   0xFFF7F830      /*Pull Disable*/
#define SPI_PC8                   0xFFF7F834      /*Pull Select*/
#define SPI_DAT0                  0xFFF7F838      /*tx data (lower 16 bits only), must have enable set inSPIGCR1.24*/
#define SPI_DAT1                  0xFFF7F83C      /*tx data configuration*/
#define SPI_BUF                   0xFFF7F840      /*tx/rx buffer info*/
#define SPI_EMU                   0xFFF7F844      /*SPI emulation - STATUS INFO??*/
#define SPI_DELAY                 0xFFF7F848      /*SPI chip select active to transmit delay*/
#define SPI_DEF                   0xFFF7F84c      /*SPI chip select inactive state*/
#define SPI_FMT0                  0xFFF7F850      /*SPI format 0*/
#define SPI_FMT1                  0xFFF7F854      /*SPI format 1*/
#define SPI_FMT2                  0xFFF7F858      /*SPI format 2*/
#define SPI_FMT3                  0xFFF7F85C      /*SPI format 3*/


#define SPI_GCR0_NRESET           0x00000001

#define SPI_GCR1_MASTER_MODE      0x00000001
#define SPI_GCR1_INT_CLK_MODE     0x00000002
#define SPI_GCR1_SPIEN            0x01000000

/* Pin control register bits */
#define SPI_PC_SCS0               0x00000001
#define SPI_PC_SCS1_GPIO10        0x00000002  
#define SPI_PC_SCS2_GPIO11        0x00000004  
#define SPI_PC_SCLK               0x00000200
#define SPI_PC_SIMO               0x00000400
#define SPI_PC_SOMI               0x00000800

/* SPI Flash opcodes */
#define OpcodeReadJEDEC           0x9F
#define OpcodeWriteEnable         0x06
#define OpcodeWriteDisable        0x04    /* Not required since WEL gets cleared after each page program */
#define OpcodeReadStatus          0x05
#define OpcodeWriteStatus         0x01
#define OpcodeReadData            0x03
#define OpcodeFastRead            0x0B    /*NEED TO TAKE DUMMY BYTE 5 INTO ACCOUNT TO USE THIS*/
#define OpcodePageProgram         0x02
#define OpcodeSectorErase         0x20
#define OpcodeBlockErase          0xD8
#define OpcodeChipErase           0xC7    
#define OpcodePowerDown           0xB9
#define OpcodeReleasePowerDown    0xAB    /*Also Read Device ID*/
#define OpcodeManufacturerID      0x90

#define SPICS_HOLD                0x10000000
#define SPI_WDEL                  0x04000000

/* Shared-bus configuration for the external flash (CS0) and MCP3008 (CS1). */
#define SPI_FUNCTION_MASK         0x00000E03UL
#define SPI_DIRECTION_MASK        0x00000601UL
#define SPI_DEFAULT_CHIP_SELECTS  0x00000003UL
#define SPI_FORMAT1_MCP3008       0x01002408UL

/**
 * @brief Initialize the shared SPI controller for flash and MCP3008 access.
 */
void SPI_Init(void);

/**
 * @brief Execute one supported SPI-flash operation.
 *
 * @param cOpcode Flash operation code.
 * @param iAddress Three-byte flash address where applicable.
 * @param pbuffer Transfer buffer where applicable.
 * @param size Number of bytes to transfer.
 * @param cs_num SPI chip-select number.
 * @return STATUS_OK on success or a STATUS_T error.
 */
STATUS_T SpiOps( UINT8_T cOpcode, UINT32_T iAddress, UINT8_T *pbuffer, UINT32_T size, UINT32_T cs_num );

/**
 * @brief Read one single-ended MCP3008 input using a three-word SPI frame.
 *
 * @param channel MCP3008 channel number from zero through seven.
 * @param sample Destination for the ten-bit conversion result.
 * @return STATUS_OK on success, STATUS_TIMEOUT when the SPI controller does
 *         not complete a transfer, or STATUS_ERROR for invalid input or an
 *         SPI receive error.
 */
STATUS_T rdx_mcp3008_read_channel(UINT8_T channel, UINT16_T *sample);

#endif
