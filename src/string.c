/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : string.c
//
// Project     : TUSB9261 RDX Firmware.
//
// Description : Firmware procedures assigned to the string module.
//
//=======================================================================================

/*! @file
 * @brief Firmware procedures assigned to the string module.
 */

#include "../include/rdx_firmware.h"

/**
 * @brief Ti memset.
 *
 * @param destination Destination buffer.
 * @param fill_byte Byte value repeated across the destination.
 * @param byte_count Number of bytes to initialize.
 * @return Result produced by the procedure.
 */
uint32_t *ti_memset(destination, fill_byte, byte_count)
uint32_t *destination;
uint32_t fill_byte;
int byte_count;

{
    /* Ti memset using only caller-provided data. */
    uint32_t *word_cursor;
    uint32_t *write_cursor;
    uint32_t *buffer_end;
    int remaining_count;

    buffer_end = (uint32_t *)((int)destination + byte_count);
    write_cursor = destination;
    if (((uint32_t)destination & 3) == 0) {
        if ((uint32_t *)((uint32_t)buffer_end & 0xfffffffc) != destination) {
            remaining_count = (int)((uint32_t)buffer_end & 0xfffffffc) - (int)destination >> 2;
            word_cursor = destination;
            /* Repeat while remaining_count != 0; the body advances or polls the state needed to leave the loop. */
            do {
                remaining_count = remaining_count + -1;
                write_cursor = word_cursor + 1;
                *word_cursor = fill_byte | (fill_byte | (fill_byte | fill_byte << 8) << 8) << 8;
                word_cursor = write_cursor;
            } while (remaining_count != 0);
        }
    }
    if (buffer_end != write_cursor) {
        remaining_count = (int)buffer_end - (int)write_cursor;
        /* Repeat while remaining_count != 0; the body advances or polls the state needed to leave the loop. */
        do {
            remaining_count = remaining_count + -1;
            *(char *)write_cursor = (char)fill_byte;
            write_cursor = (uint32_t *)((int)write_cursor + 1);
        } while (remaining_count != 0);
    }
    return destination;
}

/**
 * @brief Ti memset entry wrapper.
 *
 * @param destination Destination buffer.
 * @param fill_byte Byte value repeated across the destination.
 * @param byte_count Number of bytes to initialize.
 * @return Result produced by the procedure.
 */
uint32_t *ti_memset_entry_wrapper(destination, fill_byte, byte_count)
uint32_t *destination;
uint32_t fill_byte;
int byte_count;

{
    /* Ti memset entry wrapper using only caller-provided data. */
    uint32_t *word_cursor;
    uint32_t *write_cursor;
    uint32_t *buffer_end;
    int remaining_count;

    buffer_end = (uint32_t *)((int)destination + byte_count);
    write_cursor = destination;
    if (((uint32_t)destination & 3) == 0) {
        if ((uint32_t *)((uint32_t)buffer_end & 0xfffffffc) != destination) {
            remaining_count = (int)((uint32_t)buffer_end & 0xfffffffc) - (int)destination >> 2;
            word_cursor = destination;
            /* Repeat while remaining_count != 0; the body advances or polls the state needed to leave the loop. */
            do {
                remaining_count = remaining_count + -1;
                write_cursor = word_cursor + 1;
                *word_cursor = fill_byte | (fill_byte | (fill_byte | fill_byte << 8) << 8) << 8;
                word_cursor = write_cursor;
            } while (remaining_count != 0);
        }
    }
    if (buffer_end != write_cursor) {
        remaining_count = (int)buffer_end - (int)write_cursor;
        /* Repeat while remaining_count != 0; the body advances or polls the state needed to leave the loop. */
        do {
            remaining_count = remaining_count + -1;
            *(char *)write_cursor = (char)fill_byte;
            write_cursor = (uint32_t *)((int)write_cursor + 1);
        } while (remaining_count != 0);
    }
    return destination;
}

/**
 * @brief Ti memset alternate entry wrapper.
 *
 * @param destination Destination buffer.
 * @param fill_byte Byte value repeated across the destination.
 * @param byte_count Number of bytes to initialize.
 * @return Result produced by the procedure.
 */
uint32_t *ti_memset_alternate_entry_wrapper(destination, fill_byte, byte_count)
uint32_t *destination;
uint32_t fill_byte;
int byte_count;

{
    /* Ti memset alternate entry wrapper using only caller-provided data. */
    uint32_t *word_cursor;
    uint32_t *write_cursor;
    uint32_t *buffer_end;
    int remaining_count;

    buffer_end = (uint32_t *)((int)destination + byte_count);
    write_cursor = destination;
    if (((uint32_t)destination & 3) == 0) {
        if ((uint32_t *)((uint32_t)buffer_end & 0xfffffffc) != destination) {
            remaining_count = (int)((uint32_t)buffer_end & 0xfffffffc) - (int)destination >> 2;
            word_cursor = destination;
            /* Repeat while remaining_count != 0; the body advances or polls the state needed to leave the loop. */
            do {
                remaining_count = remaining_count + -1;
                write_cursor = word_cursor + 1;
                *word_cursor = fill_byte | (fill_byte | (fill_byte | fill_byte << 8) << 8) << 8;
                word_cursor = write_cursor;
            } while (remaining_count != 0);
        }
    }
    if (buffer_end != write_cursor) {
        remaining_count = (int)buffer_end - (int)write_cursor;
        /* Repeat while remaining_count != 0; the body advances or polls the state needed to leave the loop. */
        do {
            remaining_count = remaining_count + -1;
            *(char *)write_cursor = (char)fill_byte;
            write_cursor = (uint32_t *)((int)write_cursor + 1);
        } while (remaining_count != 0);
    }
    return destination;
}
