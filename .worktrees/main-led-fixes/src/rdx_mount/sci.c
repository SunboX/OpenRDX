/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : sci.c
//
// Project     : TUSB926x Firmware.
//
// Description : Serial Communication Interface module driver.
//
//   (C) Copyright 2009 by Texas Instruments Incorporated.
//   All rights reserved.
//
// Revision History:
//   MM/DD/YY
//   05/14/09 - Brian Quach - Calculated BRSR based on RTI clock rate.
//   07/14/09 - Brian Quach - Added circular print buffer option using TX interrupt.
//
//=======================================================================================

/*! @file
 * 
 * This file contains the implementation of the Serial Communication Interface module driver.
 *
 * A circular print buffer is used to minimize execution delays caused by debug prints.  If the 
 * interface cannot keep up with the debug, the output will be corrupted.  To avoid this,
 * the print buffer may be disabled but at the cost of execution speed.
 *
 * The interface is configured for the following: 
 * - BAUD rate: 115,200
 * - Data bits: 8
 * - Stop bits: 1
 * - Parity: None.
 * - Flow control: None.
 *  
 */

#include "sci.h"
#include "tusb9260_types.h"
#include "reg_io.h"
#include "wdt.h"

#define ENABLE_PRINT_BUFFER   1    // Set to 1 to enable circular UART debug print buffer.

#define PRINT_BUFFER_SIZE_KB  3    // Size of circular print buffer in KB.

#ifndef ENABLE_UART_RX
#define ENABLE_UART_RX 1
#endif


#if ENABLE_PRINT_BUFFER

#if DEBUG_LEVEL >= 1
UINT8_T print_buffer[PRINT_BUFFER_SIZE_KB * 1024];
#endif

UINT32_T write_offset;
UINT32_T read_offset;

BOOLEAN_T data_ready = FALSE;

#endif

/*****************************************************************************
 * Function: sci_init
 *************************************************************************//**
 * This function initializes the SCI module.
 *
 * @param None.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void sci_init(void)
{
#if ENABLE_PRINT_BUFFER
    write_offset = 0;
    read_offset = 0;
#endif

    /*Take the SCI Module OUT of Reset*/
    WRITE_REG32(SCI_GCR0, SCI_GCR0_RESET);

    /*We can now configue the SCI registers*/
    /*Tx Pin Functional*/
    WRITE_REG32(SCI_PIO0, SCI_PIO_TX_GPIO9);

#if ENABLE_UART_RX
    MODIFY_REG32(SCI_PIO0, SCI_PIO_RX_GPIO8, SCI_PIO_RX_GPIO8);
#else
    /* Set Rx data out HIGH */
    WRITE_REG32(SCI_PIO3, SCI_PIO_RX_GPIO8);

    /* Configure Rx pin as output GPIO */
    WRITE_REG32(SCI_PIO1, SCI_PIO_RX_GPIO8);
#endif 

    /*Async timing, No parity, One Stop Bit,
    Tx Enable, Internal Clock,
    in Reset state*/
    WRITE_REG32(SCI_GCR1,
                SCI_GCR1_TXENA |
                SCI_GCR1_CONT |
                SCI_GCR1_CLOCK |
                SCI_GCR1_TIMINGMODE);

#if ENABLE_UART_RX
    MODIFY_REG32(SCI_GCR1, SCI_GCR1_RXENA, SCI_GCR1_RXENA);
#endif 

    /*8 Bit Data*/
    WRITE_REG32(SCI_CHAR, 0x7);

    /*Set SCI Baud Rate to 115,200 */
    WRITE_REG32(SCI_BAUD, SCI_BRSR_VALUE);   /* 0xD @ 25 MHZ clock */

    /*Put SCI in Ready State*/
    MODIFY_REG32(SCI_GCR1, SCI_GCR1_SWNRST, SCI_GCR1_SWNRST);

    return;
}

#if DEBUG_LEVEL >= 1


#if ENABLE_PRINT_BUFFER

/*****************************************************************************
 * Function: sci_tx
 *************************************************************************//**
 * This function takes data from the circular print buffer and writes it to the
 * SCI Tx data buffer.
 *
 * @param None.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void sci_tx(void)
{
    if (READ_REG32(SCI_FLR) & SCI_FLR_TX_RDY_BIT)
    {
        if (read_offset == write_offset)
        {
            // Clear data ready flag if we have output all buffered data.
            data_ready = FALSE;
        }

        if (data_ready)
        {
            // Write character to Tx Data buffer.
            WRITE_REG32(SCI_TD, print_buffer[read_offset++]);

            // Check for wrap around.
            if (read_offset >= sizeof(print_buffer))
            {
                read_offset = 0;
            }
        }
        else
        {
            // Disable Tx interrupt.
            WRITE_REG32(SCI_CLRINT, SCI_TX_INT_BIT); 
        }
    }

    return;
}

/*****************************************************************************
 * Function: sci_isr
 *************************************************************************//**
 * Interrupt service routine for the SCI module.
 *
 * @param None.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void sci_isr(void)
{
    sci_tx();

    return;
}

/*****************************************************************************
 * Function: putchar
 *************************************************************************//**
 * This function writes data to the circular print buffer and starts SCI Tx
 * if necessary.
 *
 * @param[in] bData byte data to be output.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void putchar(UINT8_T bData)
{
    // Store character in print buffer.
    print_buffer[write_offset++] = bData;

    // Check for wrap around.
    if (write_offset >= sizeof(print_buffer))
    {
        write_offset = 0;
    }

    // Check for line feed escape character.
    if (bData == '\n')
    {
        // Add carriage return.
        print_buffer[write_offset++] = '\r';
    }

    if (write_offset >= sizeof(print_buffer))
    {
        write_offset = 0;
    }

    // Set data ready flag.
    data_ready = TRUE;

    // Check for overwrite of buffered data.
    if (write_offset == read_offset)
    {
        read_offset++;

        // Check for wrap around.
        if (read_offset >= sizeof(print_buffer))
        {
            read_offset = 0;
        }
    }

    // Enable Tx interrupt if needed.
    if ((READ_REG32(SCI_SETINT) & SCI_TX_INT_BIT) == 0)
    {
        // Enable Tx interrupt.
        WRITE_REG32(SCI_SETINT, SCI_TX_INT_BIT); 

        sci_tx();
    }

    return;
}

#else

void sci_isr(void)
{
    // Do nothing.
}

void putchar(UINT8_T bData)
{
    // Reset watchdog timer.
    wdt_reset();

    // Wait for Tx ready to be asserted.
    while ( !(READ_REG32(SCI_FLR) & SCI_FLR_TX_RDY_BIT) ); 

    // Write character to Tx Data buffer.
    WRITE_REG32(SCI_TD, bData);

    // Write carriage return if new line was sent.
    if ( bData == '\n' )
    {
        while ( !(READ_REG32(SCI_FLR) & SCI_FLR_TX_RDY_BIT) );
        WRITE_REG32(SCI_TD, '\r');
    }

    return;
}


#endif

#if ENABLE_UART_RX
UINT32_T getchar(char *pcInChar)
{
    char cInChar;

    /* Did a charater come in? */
    if (READ_REG32(SCI_FLR) & SCI_FLR_RX_RDY_BIT)
    {
        cInChar = (char)READ_REG32(SCI_RD);

        /* We need a line feed if the character is a Return. */
        if ( cInChar == '\n' )
        {
            putchar('\n');
        }
    }
    else
    {
        cInChar = 0;
    }

    *pcInChar = cInChar;

    return cInChar;
}
#endif


#define DECIMAL_BASE    10
#define HEX_BASE        16

/*****************************************************************************
 * Function: print_strings
 *************************************************************************//**
 * This function sends the characters of a string to the putchar function for 
 * printing them. 
 *
 * @param[in] print_string pointer to character array.
 * @param[in] str_width minimum number of characters to be printed.
 * @param[in] zero_align flag to indicate padding the string with zeros.
 * @param[in] string_flag flag for printing a string (for the 's' format)
 *
 * @return The total number of printed characters.
 *
 ****************************************************************************** 
 */

UINT32_T print_strings(char *print_string, UINT32_T str_width, BOOLEAN_T zero_align, BOOLEAN_T string_flag)
{
    UINT32_T numPrintChars = 0;
    UINT32_T string_length = 0;
    INT32_T diff;

    while (*print_string != '\0')
    {
        if (string_flag == TRUE)
        {
            putchar(*print_string);
            numPrintChars++;
        }
        else
        {
            string_length++;
        }

        print_string++;
    }

    if (string_flag == FALSE)
    {
        diff = str_width - string_length;

        while (diff > 0)
        {
            putchar((zero_align) ? '0' : ' ');
    
            numPrintChars++;
            diff--;
        }
    
        while (string_length > 0)
        {
            print_string--;
            putchar(*print_string);
            numPrintChars++;
            string_length--;
        }
    }

    return numPrintChars;
}

/*****************************************************************************
 * Function: number_to_string
 *************************************************************************//**
 * This function converts a number to a string
 *
 * @param[in] arg number to be converted into a string.
 * @param[in] base base of the number (Dec or Hex).
 * @param[in] sign flag to indicate converting a signed number. 
 * @param[in] lower_case flag to indicate the character case to convert the number to. 
 * @param[in] print_string pointer to the converted character array.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */
void number_to_string(INT32_T arg, UINT32_T base, BOOLEAN_T sign, BOOLEAN_T lower_case, char *print_string)
{

    UINT32_T quotient = 0;
    UINT32_T reminder = 0;
    UINT32_T dividend = arg;  

    if (arg == 0)
    {
        *print_string = '0';
        print_string++;
        *print_string = '\0';
    }
    else
    {
        if ((arg < 0) && (sign == TRUE))
        {
            dividend = -arg;
        }

        while (dividend)
        {
            quotient = dividend / base;
            reminder = dividend - (quotient * base);

            if ((reminder > 9) && (lower_case == FALSE))
                reminder += 7; /*Adding offset for getting the upper cases on the ASCII code*/

            if ((reminder > 9) && (lower_case == TRUE))
                reminder += 39; /*Adding offset for getting the lower cases on the ASCII code*/

            reminder += '0';

            *(print_string++) = reminder;

            dividend /= base;
        }

        if ((arg < 0) && (sign == TRUE))
        {
            *(print_string++) = '-';
        }

        *(print_string++) = '\0';
    }
}

/*****************************************************************************
 * Function: kprintf
 *************************************************************************//**
 * This function outputs to the SCI port a sequence of data formatted as the 
 * format_str argument specifies.
 *
 * Supported format specifiers: 'd' 'u' 'c' 's' 'x' 'X'.
 * Zero padding and field width are also supported.
 *
 * @param[in] format_str string that contains the text to sent to the SCI port.
 * @param[in] ... subsequent arguments to be formatted as specified by the format parameters.
 *  
 * @return The total number of printed characters.
 *
 ****************************************************************************** 
 */

UINT32_T kprintf(const char *format_str, ...)
{
    INT32_T *arguments = (INT32_T *)(&format_str);
    char *pFormat_str= (char *)(*arguments++);
    char print_string[12];
    char *str;
    UINT32_T numPrintChars = 0;
    BOOLEAN_T zero_align;
    UINT32_T str_width;

    while (*pFormat_str != '\0')
    {
        if (*pFormat_str != '%')
        {
            putchar(*pFormat_str);
            ++numPrintChars;
            pFormat_str++;
            continue;
        }

        str_width = 0;
        zero_align = FALSE;
        pFormat_str++;

        if (*pFormat_str == '0')
        {
            zero_align = TRUE;
            pFormat_str++;

            while ((*pFormat_str >= '0') && (*pFormat_str <= '9'))
            {
                str_width *= 10;
                str_width += *pFormat_str - '0';
                pFormat_str++;
            }
        }

        switch (*pFormat_str)
        {
            case '%':
                putchar('%');
                numPrintChars++;
                break;

            case '\0':
                break;

            case 'd':
                number_to_string(*arguments++, DECIMAL_BASE, TRUE, FALSE, print_string);
                break;

            case 'u':
                number_to_string(*arguments++, DECIMAL_BASE, FALSE, FALSE, print_string);
                break;

            case 'x':
                number_to_string(*arguments++, HEX_BASE, FALSE, TRUE, print_string);
                break;

            case 'X':
                number_to_string(*arguments++, HEX_BASE, FALSE, FALSE, print_string);
                break;

            case 's':
                str = *((char **)arguments++);
                print_strings(str, str_width, zero_align, TRUE);
                break;

            case 'c':
                print_string[0] = *arguments++;
                print_string[1] = '\0';
                break;

            default:
                // Do nothing.
                break;
        }

        if ((*pFormat_str == 'd') || (*pFormat_str == 'u') || (*pFormat_str == 'x') || 
            (*pFormat_str == 'X') || (*pFormat_str == 'c'))
        {
            print_strings(print_string, str_width, zero_align, FALSE);
        }

        pFormat_str++;
    }

    return numPrintChars;
}


#endif /* DEBUG_LEVEL >= 1 */
