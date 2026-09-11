/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t UINT8_T;
typedef uint32_t UINT32_T;
typedef int BOOLEAN_T;
typedef int STATUS_T;
#define STATUS_OK 0
#define STATUS_ERROR 1
#define ti_memcpy memcpy
#define ti_memset memset
#define READ_REG32(address) read_register(address)
#define WRITE_REG32(address, value) write_register(address, value)

/* PRODUCTION_DEFINES */

static UINT8_T flash_record[64];
static UINT32_T irq_bits;
static int spi_calls, failure_at;
static const UINT8_T expected_opcodes[5] = { 3, 6, 0x20, 6, 2 };

/** Report a failed production invariant without performing device I/O. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}

/** Model the VIM readable interrupt-enable bitset. */
static UINT32_T read_register(UINT32_T address)
{
    require(address == 0xFFFFFE30U, "unexpected register read");
    return irq_bits;
}

/** Model write-one-to-set/clear registers without overwriting other IRQ bits. */
static void write_register(UINT32_T address, UINT32_T value)
{
    require((value & ~0x00200000U) == 0U, "state writer changed unrelated IRQ bits");
    if (address == 0xFFFFFE30U)
    {
        require(value == 0U || spi_calls == (failure_at ? failure_at : 5),
                "USB was enabled before the complete state transaction ended");
        irq_bits |= value;
    }
    else if (address == 0xFFFFFE40U)
        irq_bits &= ~value;
    else
        require(0, "unexpected register write");
}

/** Observe the complete SPI transaction and inject each possible call failure. */
static STATUS_T SpiOps(UINT8_T opcode, UINT32_T address, UINT8_T *buffer,
                       UINT32_T length, UINT32_T chip_select)
{
    require((irq_bits & 0x00200000U) == 0U,
            "persistent-state SPI access occurred while USB interrupt was enabled");
    require(spi_calls < 5 && opcode == expected_opcodes[spi_calls],
            "state persistence changed its required SPI sequence");
    require(chip_select == 0U, "state write selected the wrong SPI chip");
    if (spi_calls == 0)
    {
        /* Another enabled interrupt may change its own mask while USB is
         * excluded. Restoring the entire stale bitset would lose that change. */
        irq_bits ^= 0x01000000U;
    }
    spi_calls++;
    if (spi_calls == failure_at)
        return STATUS_ERROR;
    if (opcode == OpcodeReadData || opcode == OpcodePageProgram)
    {
        require(address == 0x3F000U && length == 64U,
                "state record access moved outside its primary record");
        if (opcode == OpcodeReadData)
            memcpy(buffer, flash_record, length);
        else
            memcpy(flash_record, buffer, length);
    }
    else if (opcode == OpcodeSectorErase)
    {
        require(address == 0x3F000U, "wrong state sector erased");
        memset(flash_record, 0xFF, sizeof(flash_record));
    }
    return STATUS_OK;
}

/* PRODUCTION_FUNCTIONS */

/** Exercise a real state save with success or one injected SPI failure. */
int main(int argc, char **argv)
{
    UINT32_T prior_irq, index;
    UINT8_T before_record[64], expected_record[64];
    STATUS_T status;
    require(argc == 4, "missing state-save probe arguments");
    prior_irq = 0x80000060U | (atoi(argv[2]) ? 0x00200000U : 0U);
    irq_bits = prior_irq;
    failure_at = atoi(argv[3]);
    for (index = 0; index < sizeof(flash_record); index++)
        flash_record[index] = (UINT8_T)(index * 13U + 7U);
    for (index = 0; index < 4U; index++)
    {
        UINT32_T offset;
        flash_record[index] = (UINT8_T)(0x12345678U >> (8U * index));
        for (offset = 4U + index; offset < sizeof(flash_record); offset += 4U)
            flash_record[index] += flash_record[offset];
    }
    memcpy(before_record, flash_record, sizeof(before_record));
    if (strcmp(argv[1], "mode") == 0)
        status = rdx_control_save_operation_mode(2U);
    else
        status = rdx_control_save_drive_load_count(0x12345678U);
    require(status == (failure_at ? STATUS_ERROR : STATUS_OK),
            "state write returned incorrect SPI failure status");
    require(irq_bits == (prior_irq ^ 0x01000000U),
            "state save did not preserve prior USB state and the other IRQ change");
    require(spi_calls == (failure_at ? failure_at : 5),
            "state save continued after an SPI failure");
    if (!failure_at)
    {
        memcpy(expected_record, before_record, sizeof(expected_record));
        if (strcmp(argv[1], "mode") == 0)
            expected_record[13] = 2U;
        else
        {
            expected_record[4] = 0x78U;
            expected_record[5] = 0x56U;
            expected_record[6] = 0x34U;
            expected_record[7] = 0x12U;
        }
        for (index = 0; index < 4U; index++)
        {
            UINT32_T offset;
            expected_record[index] = (UINT8_T)(0x12345678U >> (8U * index));
            for (offset = 4U + index; offset < sizeof(expected_record); offset += 4U)
                expected_record[index] += expected_record[offset];
        }
        require(memcmp(flash_record, expected_record, sizeof(expected_record)) == 0,
                "state write changed another field or produced an invalid checksum");
    }
    return 0;
}
