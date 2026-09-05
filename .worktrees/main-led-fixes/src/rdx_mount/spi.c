/*
 * SPDX-FileCopyrightText: 2009-2013 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

/*****************************************************************************
* Filename    : spi.c
*
* Project     : TUSB926x Firmware.
*
* Description : SPI Functions for communicating to the SPI Flash Memory.
*
*   (C) Copyright 2009-2013 by Texas Instruments Incorporated.
*   All rights reserved.
*
* Revision History:
*   MM/DD/YY
*   06/25/09 - Kevin Harris - Creation.
*   10/08/13 - Brian Quach  - Fixed chip select usage.
*                           - Added support for CS1 and CS2.
*                           - Code fixes, optimizations, and clean-up.  
*
*****************************************************************************/

/*! @file
 * 
 * This file contains the implementation of the Serial Peripheral Interface module driver.
 *  
 */

#include "spi.h"
#include "gio.h"
#include "reg_io.h"
#include "rti.h"
#include "sci.h"
#include "tusb9260.h"
#include "tusb9260_types.h"
#include "wdt.h"

#define ENABLE_CS2  0

#define SPI_FLAG_TX_AVAIL_BIT  0x00000200
#define SPI_FLAG_RX_VALID_BIT  0x00000100
#define SPI_FLAG_RX_ERROR_BIT  0x00000010

#define SPI_WAIT_LIMIT         1000000UL

#define MCP3008_START_WORD     0x11050001UL
#define MCP3008_CHANNEL_WORD   0x11050080UL
#define MCP3008_FINISH_WORD    0x01050000UL

/**
 * @brief Poll one SPI completion flag with a finite wait bound.
 *
 * Ten-microsecond polling intervals cap the wait at one million observations.
 * Resetting the watchdog keeps a failed peripheral from turning a bounded bus
 * error into an unrelated watchdog reset.
 *
 * @param completion_mask SPI flag that must become set.
 * @return STATUS_OK when the flag is observed or STATUS_TIMEOUT on expiry.
 */
static STATUS_T spi_wait_for_flag_bounded(UINT32_T completion_mask)
{
    UINT32_T iterations;

    iterations = 0UL;

    while (!(READ32(SPI_FLG) & completion_mask))
    {
        if (iterations >= SPI_WAIT_LIMIT)
        {
            return STATUS_TIMEOUT;
        }
        wdt_reset();
        usleep(10UL);
        iterations++;
    }

    return STATUS_OK;
}

/**
 * @brief Transfer one complete SPI DAT1 control-and-payload word.
 *
 * @param transmit_word SPI DAT1 control and payload word.
 * @param receive_word Destination for the complete SPI receive word.
 * @return STATUS_OK on success, STATUS_TIMEOUT on a bounded wait expiry, or
 *         STATUS_ERROR when the controller reports a receive error.
 */
static STATUS_T spi_transfer_word_bounded(
    UINT32_T transmit_word,
    UINT32_T *receive_word)
{
    STATUS_T status;

    status = spi_wait_for_flag_bounded(SPI_FLAG_TX_AVAIL_BIT);
    if (status != STATUS_OK)
    {
        return status;
    }

    WRITE_REG32(SPI_DAT1, transmit_word);
    status = spi_wait_for_flag_bounded(SPI_FLAG_RX_VALID_BIT);
    if (status != STATUS_OK)
    {
        return status;
    }

    *receive_word = READ32(SPI_BUF);
    if (READ32(SPI_FLG) & SPI_FLAG_RX_ERROR_BIT)
    {
        WRITE_REG32(SPI_FLG, SPI_FLAG_RX_ERROR_BIT);
        return STATUS_ERROR;
    }

    return STATUS_OK;
}

/**
 * @brief Read one single-ended MCP3008 input using a three-word SPI frame.
 *
 * @param channel MCP3008 channel number from zero through seven.
 * @param sample Destination for the ten-bit conversion result.
 * @return STATUS_OK on success, STATUS_TIMEOUT when the SPI controller does
 *         not complete a transfer, or STATUS_ERROR for invalid input or an
 *         SPI receive error.
 */
STATUS_T rdx_mcp3008_read_channel(UINT8_T channel, UINT16_T *sample)
{
    STATUS_T status;
    UINT32_T receive_word;
    UINT16_T upper_bits;

    if ((sample == NULL) || (channel > 7U))
    {
        return STATUS_ERROR;
    }

    *sample = 0U;
    status = spi_transfer_word_bounded(MCP3008_START_WORD, &receive_word);
    if (status != STATUS_OK)
    {
        return status;
    }

    status = spi_transfer_word_bounded(
        MCP3008_CHANNEL_WORD | ((UINT32_T)channel << 4U), &receive_word);
    if (status != STATUS_OK)
    {
        return status;
    }

    upper_bits = (UINT16_T)((receive_word & 3UL) << 8U);
    status = spi_transfer_word_bounded(MCP3008_FINISH_WORD, &receive_word);
    if (status != STATUS_OK)
    {
        return status;
    }

    *sample = (UINT16_T)(upper_bits | (receive_word & 0xFFUL));
    return STATUS_OK;
}

/*****************************************************************************
 * Function: spi_transfer
 *************************************************************************//**
 * This function performs a SPI transfer. 
 *
 * @param[in] spi_dat1 Tx data byte and flags to be programmed to SPI_DAT1 register. WDEL flag is automatically set.
 *                    
 * @returns the received data byte from SPI_BUF register.
 *
 ****************************************************************************** 
 */

UINT8_T spi_transfer(UINT32_T spi_dat1)
{
    //SPI Interface is Full Duplex
    //Everytime You Write to SPI_DAT1 You Must Read from SPI_BUF
    //Before Writing to SPI_DAT1 Always Spin on TX Available
    //Before Reading from SPI_BUF Always Spin on RX Valid

    //Spin on TX Available
    while ( !(READ32(SPI_FLG) & SPI_FLAG_TX_AVAIL_BIT) );

    //Send the Command to Read the Status Register
    WRITE_REG32(SPI_DAT1, spi_dat1 | SPI_WDEL);

    //Spin on RX Valid
    while ( !(READ32(SPI_FLG) & SPI_FLAG_RX_VALID_BIT) );
    
    return (UINT8_T)(READ32(SPI_BUF) & 0xFF);
}


typedef enum SPI_STATUS_BITS
{
    BUSY_STATUS          = 0x1,
    WEL_STATUS           = (0x1 << 1),
    BLK_PROTECT          = (0x1 << 2),
    WRITE_PROTECT_STATUS = (0x1 << 3),
    ERASE_PROGRAM_ERROR  = (0x1 << 5),
    BLK_PROTECT_LOCKED   = (0x1 << 7)
} SPI_STATUS_BITS_T;

/*****************************************************************************
 * Function: WaitForStatus
 *************************************************************************//**
 * This function polls the status until the specified status bits match.
 *
 * @param[in] status_mask bitmask for status.
 * @param[in] status_bits status bits to wait for.
 * @param[in] csnr csnr bits of the spi_dat1 register.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void WaitForStatus(UINT8_T status_mask, UINT8_T status_bits, UINT32_T csnr)
{
    UINT8_T status;

    do
    {
        // Reset WDT.
        wdt_reset();

        // Send the Command to Read the Status Register.
        spi_transfer(csnr | SPICS_HOLD | OpcodeReadStatus);

        // Read Status.
        status = spi_transfer(csnr);

        // If the masked current status matches the desired status bits, exit the while loop.
        if ((status & status_mask) == status_bits)
        {
            break;
        }

        usleep(1);

    } while (1);
}


#define CSNR_OFFSET 16
#define DFSEL_OFFSET 24

/*****************************************************************************
 * Function: SpiOps
 *************************************************************************//**
 * This function executes the specificed SPI OPCODE. 
 *
 * @param[in] cOpcode operational code to execute.
 * @param[in] iAddress address of memory page to read or write.
 * @param[in] pbuffer pointer to memory location to store data from SPI read or to read data for a SPI write.
 * @param[in] size number of bytes to read or write.
 * @param[in] cs chip select number (0-2).
 *                    
 * @retval STATUS_OK when successful.
 * @retval STATUS_NOT_SUPPORTED when the OPCODE is not supported.
 *
 ****************************************************************************** 
 */

STATUS_T SpiOps( UINT8_T cOpcode, UINT32_T iAddress, UINT8_T *pbuffer, UINT32_T size, UINT32_T cs_num )
{
    STATUS_T status = STATUS_OK;
    UINT32_T csnr;
    UINT32_T qBytesToRead;  /*Number of bytes expected to be returned on a read.*/
    UINT32_T qBytesToWrite;
    UINT32_T qindex;

    csnr = (0xFF & ~(1 << cs_num)) << CSNR_OFFSET;

    /*************************************************************************
     * If using data formats other than format 0, add DFSEL bits to csnr
     * depending on the cs_num.
     * Example:   csnr |= (data_format_num << DFSEL_OFFSET)
     ************************************************************************/

    switch ( cOpcode )
    {
        case OpcodeWriteEnable:

            spi_transfer(csnr | cOpcode);

            // Wait for WEL = 1.
            WaitForStatus(WEL_STATUS, WEL_STATUS, csnr);

            break;

        case OpcodeReadData:
            qBytesToRead = size;

            spi_transfer(csnr | SPICS_HOLD | cOpcode);

            // Send address (3-bytes).
            for ( qindex = 0; qindex < 3; qindex++ )
            {
                spi_transfer(csnr | SPICS_HOLD | ( (iAddress >> (16 - (qindex * 8))) & 0x000000FF));
            }

            qindex = 0;

            // Read the data.
            while ( qBytesToRead )
            {
                qBytesToRead--;
                pbuffer[qindex] = spi_transfer(csnr | ((qBytesToRead) ? SPICS_HOLD : 0));
                qindex++;
            }

            break;

        case OpcodePageProgram:

            qBytesToWrite = size;

            spi_transfer(csnr | SPICS_HOLD | cOpcode);

            // Send address (3-bytes).
            for ( qindex = 0; qindex < 3; qindex++ )
            {
                spi_transfer(csnr | SPICS_HOLD | ( (iAddress >> (16 - (qindex * 8) ) ) & 0x000000FF));
            }

            qindex = 0;

            // Write the data.
            while ( qBytesToWrite )
            {
                qBytesToWrite--;

                spi_transfer(csnr | ((qBytesToWrite) ? SPICS_HOLD : 0) | pbuffer[qindex]);

                qindex++;
            }

            // Wait for program to finish.
            WaitForStatus(BUSY_STATUS, 0, csnr);

            break;

        case OpcodeChipErase:
            spi_transfer(csnr | cOpcode);
            CRIT("Erasing SPI flash.\n");
            // Wait for erase to finish.
            WaitForStatus(BUSY_STATUS, 0, csnr);

            break;

        case OpcodeBlockErase:
        case OpcodeSectorErase:
            spi_transfer(csnr | SPICS_HOLD | cOpcode);

            // Send address (3-bytes).
            for ( qindex = 0; qindex < 3; qindex++ )
            {
                spi_transfer(csnr | ((qindex < 2) ? SPICS_HOLD : 0)| ( (iAddress >> (16 - (qindex * 8) ) ) & 0x000000FF));
            }
            // Wait for erase to finish.
            WaitForStatus(BUSY_STATUS, 0, csnr);

            break;

        default:
            status = STATUS_NOT_SUPPORTED;
            break;
    }

    return status;
}


/*****************************************************************************
 * Function: SPI_Init
 *************************************************************************//**
 * This function initializes the SPI module.
 *
 * @param None.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void SPI_Init(void)
{
    // Take SPI module out of reset.
    WRITE_REG32(SPI_GCR0, SPI_GCR0_NRESET);

    // Configure as SPI Master with internal clock and clear SPIEN.
    WRITE_REG32(SPI_GCR1, SPI_GCR1_MASTER_MODE | SPI_GCR1_INT_CLK_MODE);
    
    // CS0 selects the external flash and CS1 selects the MCP3008 ADC.
    MODIFY_REG32(SPI_PC0, SPI_FUNCTION_MASK, SPI_FUNCTION_MASK);

#if ENABLE_CS2
    MODIFY_REG32(SPI_PC0, SPI_PC_SCS2_GPIO11, SPI_PC_SCS2_GPIO11);
#endif

    // Drive SCS0, SCS1, SCLK, and SIMO; keep SOMI configured as an input.
    MODIFY_REG32(SPI_PC1, SPI_DIRECTION_MASK, SPI_DIRECTION_MASK);

    /* Setup CS timing */
    /* Per Grant Ley:
       SPI Delay Register (SPIDELAY)
       C2TDELAY = 2 => tC2TDELAY = (C2TDELAY + 2) * VCLK Period => Chip-select-active-to-transmit-start-delay is 53 ns @ 75MHz clk.
       T2CDELAY = 2 => tT2CDELAY = (T2CDELAY + 1)* VCLK Period => Transmit-end-to-chip-select-inactive-delay is 40 ns @ 75MHz clk.
       T2EDELAY = 0
       C2EDELAY = 0 */
    WRITE_REG32(SPI_DELAY, 0x02020000);

    /* Set default CS pattern - all high */
    WRITE_REG32(SPI_DEF, SPI_DEFAULT_CHIP_SELECTS);

    // Disable pulls on the shared bus while retaining the GPIO11 setting.
    WRITE_REG32(SPI_PC7, SPI_PC_SCS2_GPIO11);

    /* Setup data format 0 for SPI Mode 0 */
    /* Per Grant Ley:
       SPI Data Format 0 (SPIFMT0) = 0x01010308 for silicon (75 MHz clk). 
       WDELAY0 = 1 (40 ns delay @ 75 MHz clk)
       POLARITY0 = 0 (SPI clock signal is low-inactive)
       PHASE0 = 1 (SPI clock signal is delayed by a half SPI clock cycle versus the transmit / receive data stream) 
       PRESCALE0 = 4 (18.75 MHz SPI clock @ 75 MHz clk)
       CHARLEN0 = 8 */
    WRITE_REG32(SPI_FMT0, 0x01010308);

    // Data format 1 supplies the MCP3008 clock and word framing on CS1.
    WRITE_REG32(SPI_FMT1, SPI_FORMAT1_MCP3008);

    /*Set interrupt levels to INT0*/
    WRITE_REG32(SPI_LVL, 0x00000000);

    /*Enable Interrupts*/
    /*WRITE_REG32(SPI_INT0, 0x00000300);*/

    // Enable SPI communication.
    MODIFY_REG32(SPI_GCR1, SPI_GCR1_SPIEN, SPI_GCR1_SPIEN);

    return;
}


