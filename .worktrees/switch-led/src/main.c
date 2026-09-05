/*! @file
 * @brief Standalone lock-slider-to-green-LED test firmware.
 */

#include "switch_test.h"

/**
 * @brief Poll one SPI completion flag with a bounded iteration count.
 *
 * @param completion_mask SPI flag that must become set.
 * @return Zero on success, or one on timeout.
 */
static uint32_t spi_wait_for_flag(uint32_t completion_mask)
{
    uint32_t iterations;

    iterations = 0UL;
    while ((SPI_FLG & completion_mask) == 0UL) {
        iterations++;
        if (iterations >= SPI_WAIT_LIMIT) {
            return 1UL;
        }
    }
    return 0UL;
}

/**
 * @brief Transfer one word using the recovered TUSB9261 SPI transaction path.
 *
 * @param transmit_word SPI DAT1 control and payload word.
 * @param receive_word Destination for the received SPI buffer value.
 * @return Zero on success, or one on timeout or receive error.
 */
static uint32_t spi_transfer_word(
    uint32_t transmit_word,
    uint32_t *receive_word)
{
    if (spi_wait_for_flag(SPI_TX_READY_FLAG) != 0UL) {
        return 1UL;
    }

    SPI_DAT1 = transmit_word;
    if (spi_wait_for_flag(SPI_RX_READY_FLAG) != 0UL) {
        return 1UL;
    }

    *receive_word = SPI_BUF;
    if ((SPI_FLG & SPI_RX_ERROR_FLAG) != 0UL) {
        SPI_FLG = SPI_RX_ERROR_FLAG;
        return 1UL;
    }
    return 0UL;
}

/**
 * @brief Read one ten-bit MCP3008 channel using the recovered three-word frame.
 *
 * @param channel MCP3008 channel number from zero through seven.
 * @param sample Destination for the ten-bit conversion result.
 * @return Zero on success, or one when an SPI transfer fails.
 */
static uint32_t mcp3008_read_channel(uint32_t channel, uint32_t *sample)
{
    uint32_t receive_word;
    uint32_t upper_bits;

    if (spi_transfer_word(MCP3008_START_WORD, &receive_word) != 0UL) {
        return 1UL;
    }
    if (spi_transfer_word(
            MCP3008_CHANNEL_WORD | ((channel & 7UL) << 4),
            &receive_word) != 0UL) {
        return 1UL;
    }

    upper_bits = (receive_word & 3UL) << 8;
    if (spi_transfer_word(MCP3008_FINISH_WORD, &receive_word) != 0UL) {
        return 1UL;
    }

    *sample = upper_bits | (receive_word & 0xFFUL);
    return 0UL;
}

/**
 * @brief Drive the active-low green front LED channel.
 *
 * @param green_on Nonzero to illuminate the green channel.
 */
static void front_green_led_write(uint32_t green_on)
{
    if (green_on != 0UL) {
        GIOOUT0 &= ~FRONT_GREEN_LED_MASK;
    } else {
        GIOOUT0 |= FRONT_GREEN_LED_MASK;
    }

}

/**
 * @brief Configure only the front LEDs and recovered MCP3008 SPI path.
 */
static void diagnostic_hardware_init(void)
{
    GIOGCR0 = 1UL;
    GIOOUT0 |= FRONT_GREEN_LED_MASK | FRONT_AMBER_LED_MASK;
    GIODIR0 |= FRONT_GREEN_LED_MASK | FRONT_AMBER_LED_MASK;
    GIOPULDIS0 |= FRONT_GREEN_LED_MASK | FRONT_AMBER_LED_MASK;

    SPI_GCR0 = 1UL;
    SPI_GCR1 = SPI_MASTER_INTERNAL_CLOCK;
    SPI_PC0 = (SPI_PC0 & ~SPI_FUNCTION_MASK) | SPI_FUNCTION_MASK;
    SPI_PC1 = (SPI_PC1 & ~SPI_DIRECTION_MASK) | SPI_DIRECTION_MASK;
    SPI_DELAY = SPI_DELAY_CONFIGURATION;
    SPI_DEF = SPI_DEFAULT_CHIP_SELECTS;
    SPI_PC7 = SPI_PULL_DISABLE_CONFIGURATION;
    SPI_FMT0 = SPI_FORMAT_ZERO_CONFIGURATION;
    SPI_FMT1 = SPI_FORMAT_ONE_CONFIGURATION;
    SPI_LVL = 0UL;
    SPI_GCR1 |= SPI_ENABLE_BIT;
}

/**
 * @brief Display the confirmed lock-slider input continuously.
 */
void main(void)
{
    uint32_t channel_four;

    diagnostic_hardware_init();
    for (;;) {
        if (mcp3008_read_channel(
                LOCK_SLIDER_ADC_CHANNEL, &channel_four) == 0UL) {
            front_green_led_write(
                channel_four > LOCK_SLIDER_ADC_THRESHOLD);
        } else {
            front_green_led_write(0UL);
        }
    }
}
