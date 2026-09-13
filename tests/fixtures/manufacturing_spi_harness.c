/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "registers.h"
typedef uint8_t UINT8_T;
typedef uint16_t UINT16_T;
typedef uint32_t UINT32_T;
typedef int BOOLEAN_T;
typedef int STATUS_T;
#define FALSE 0
#define TRUE 1
#define STATUS_OK 0
#define STATUS_ERROR 1
#define STATUS_TIMEOUT 2
#define STATUS_NOT_SUPPORTED 3
#define RTIFRC0_REG_OFF 0x1234U
#define CRIT(...) ((void)0)
static UINT32_T time_us, transmitted, receive_value, frame_position;
static UINT8_T opcode, flash_status;
static BOOLEAN_T tx_stalled, rx_stalled, busy_forever, no_wel, rx_error, ignore_wrdi;

/** Abort on a low-level receiver contract failure. */
static void require(int condition, const char *message)
{
    if (!condition) { fprintf(stderr, "%s\n", message); exit(1); }
}

/** Return moving RTI time so busy and controller flag timeouts are executable. */
static UINT32_T read_register(UINT32_T address)
{
    if (address == RTIFRC0_REG_OFF) { time_us += 100; return time_us; }
    if (address == SPI_FLG)
        return (tx_stalled ? 0 : 0x200) | (rx_stalled ? 0 : 0x100) | (rx_error ? 0x10 : 0);
    if (address == SPI_BUF) return receive_value;
    return 0;
}

/** Model CS framing and flash WEL/BUSY status, recording exact transferred bytes. */
static void write_register(UINT32_T address, UINT32_T value)
{
    if (address == SPI_FLG) { rx_error = FALSE; return; }
    if (address != SPI_DAT1) return;
    transmitted++;
    if (frame_position++ == 0) opcode = (UINT8_T)value;
    receive_value = 0;
    if (opcode == OpcodeReadStatus && frame_position > 1)
        receive_value = busy_forever ? 1 : flash_status;
    if (!(value & SPICS_HOLD)) {
        if (opcode == OpcodeWriteEnable && !no_wel) flash_status = 2;
        if ((opcode == OpcodeWriteDisable && !ignore_wrdi) ||
            opcode == OpcodePageProgram || opcode == OpcodeSectorErase)
            flash_status = 0;
        frame_position = 0;
    }
}
#define READ_REG32(address) read_register(address)
#define READ32(address) read_register(address)
#define WRITE_REG32(address, value) write_register(address, value)
#define MODIFY_REG32(address, mask, value) write_register(address, (read_register(address) & ~(mask)) | (value))
/** Advance a legacy polling delay for successful SpiOps characterization. */
static void usleep(UINT32_T microseconds) { time_us += microseconds; }
/** The watchdog reset is a hardware boundary with no effect on emulated RTI. */
static void wdt_reset(void) { }

BOOLEAN_T rdx_spi_acquire(void);
void rdx_spi_release(void);
STATUS_T SpiOpsBounded(UINT8_T opcode, UINT32_T address, UINT8_T *buffer,
                       UINT32_T length, UINT32_T chip);
STATUS_T SpiOps(UINT8_T opcode, UINT32_T address, UINT8_T *buffer,
                UINT32_T length, UINT32_T chip);
STATUS_T rdx_mcp3008_read_channel(UINT8_T channel, UINT16_T *sample);
void SPI_Init(void);

/** A failed frame must block later flash/ADC work until controller initialization. */
static void reset_after_fault(void)
{
    rdx_spi_release();
    require(!rdx_spi_acquire(), "Faulted bus admitted another transaction");
    SPI_Init();
    require(rdx_spi_acquire(), "Controller initialization did not clear fault");
    frame_position = 0;
}

/** Verify finite waits, input bounds, and contention with legacy flash and ADC. */
int main(void)
{
    UINT8_T page[256] = {0};
    UINT16_T sample;
    UINT32_T before;
    require(SpiOpsBounded(OpcodeReadData, 0, page, 256, 0) != STATUS_OK,
            "Bounded operation accepted missing ownership");
    require(rdx_spi_acquire(), "Initial acquisition failed");
    require(!rdx_spi_acquire(), "Double SPI acquisition succeeded");
    before = transmitted;
    require(SpiOps(OpcodeReadData, 0, page, 256, 0) != STATUS_OK, "Legacy SPI ignored ownership");
    require(rdx_mcp3008_read_channel(4, &sample) != STATUS_OK, "ADC ignored ownership");
    require(transmitted == before, "Contended call touched SPI");
    require(SpiOpsBounded(OpcodeWriteEnable, 0, NULL, 0, 0) == STATUS_OK, "WREN failed");
    require(SpiOpsBounded(OpcodePageProgram, 0x3e000, page, 256, 0) == STATUS_OK, "Page failed");
    require(SpiOpsBounded(OpcodeSectorErase, 0x3e000, NULL, 0, 0) == STATUS_OK, "Erase failed");
    before = transmitted;
    require(SpiOpsBounded(OpcodePageProgram, 0x3e001, page, 256, 0) != STATUS_OK, "Cross-page write admitted");
    require(SpiOpsBounded(OpcodeSectorErase, 0x3e001, NULL, 0, 0) != STATUS_OK, "Unaligned erase admitted");
    require(SpiOpsBounded(OpcodeReadData, 0, NULL, 256, 0) != STATUS_OK, "Null read admitted");
    require(SpiOpsBounded(OpcodeChipErase, 0, NULL, 0, 0) != STATUS_OK, "Unbounded erase admitted");
    require(transmitted == before, "Malformed operation touched SPI");
    tx_stalled = TRUE;
    require(SpiOpsBounded(OpcodeReadData, 0, page, 256, 0) == STATUS_TIMEOUT, "TX stall did not time out");
    tx_stalled = FALSE; reset_after_fault(); rx_stalled = TRUE;
    require(SpiOpsBounded(OpcodeReadData, 0, page, 256, 0) == STATUS_TIMEOUT, "RX stall did not time out");
    rx_stalled = FALSE; reset_after_fault(); busy_forever = TRUE;
    require(SpiOpsBounded(OpcodeSectorErase, 0x3e000, NULL, 0, 0) == STATUS_TIMEOUT, "BUSY did not time out");
    busy_forever = FALSE; reset_after_fault(); flash_status = 0; no_wel = TRUE;
    require(SpiOpsBounded(OpcodeWriteEnable, 0, NULL, 0, 0) == STATUS_TIMEOUT, "Missing WEL did not time out");
    no_wel = FALSE; reset_after_fault(); rx_error = TRUE;
    require(SpiOpsBounded(OpcodeReadData, 0, page, 256, 0) == STATUS_ERROR, "RX error accepted");
    reset_after_fault();
    require(SpiOpsBounded(OpcodeWriteEnable, 0, NULL, 0, 0) == STATUS_OK, "WREN failed");
    ignore_wrdi = TRUE;
    require(SpiOpsBounded(OpcodeWriteDisable, 0, NULL, 0, 0) == STATUS_TIMEOUT,
            "Write-disable did not verify that WEL cleared");
    ignore_wrdi = FALSE; reset_after_fault(); rdx_spi_release();
    require(rdx_spi_acquire(), "Ownership was not released");
    rdx_spi_release();
    return 0;
}
