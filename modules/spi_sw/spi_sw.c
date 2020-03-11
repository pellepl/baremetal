/* Copyright (c) 2020 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "spi_sw.h"

#ifndef CONFIG_SPI_SW_BUSES
#error Number of spi software buses must be defined in CONFIG_SPI_SW_BUSES
#endif

static spi_sw_t _hdl[CONFIG_SPI_SW_BUSES];

#ifndef NO_SPI_SW_DEFAULT_GPIO
#include "gpio_driver.h"

static int spi_sw_gpio_get(uint16_t pin) {
    return gpio_read(pin);
}

static void spi_sw_gpio_set(uint16_t pin, uint8_t state) {
    gpio_set(pin, state);
}

#endif // !NO_SPI_SW_DEFAULT_GPIO

static void spi_sw_delay(spi_sw_t *hdl) {
    volatile unsigned int t = hdl->delay;
    while (t--);
}

void spi_sw_init(unsigned int port, spi_sw_mode_t mode,  spi_sw_transmission_t endianess_tx,  spi_sw_transmission_t endianess_rx,
                 uint16_t mosi_pin, uint16_t miso_pin, uint16_t clk_pin, unsigned int delay) {
    spi_sw_t *hdl= &_hdl[port];
    hdl->mode = mode;
    hdl->endianess_tx = endianess_tx;
    hdl->endianess_rx = endianess_rx;
    hdl->mosi_pin = mosi_pin;
    hdl->miso_pin = miso_pin;
    hdl->clk_pin = clk_pin;
    hdl->delay = delay;
    spi_sw_gpio_set(clk_pin, hdl->mode == SPI_MODE_2 || hdl->mode == SPI_MODE_3);
}

int spi_sw_txrx(unsigned int port, const uint8_t *tx_data, uint8_t *rx_data, uint16_t len) {
    spi_sw_t *hdl= &_hdl[port];
    int cpol = hdl->mode == SPI_MODE_2 || hdl->mode == SPI_MODE_3;
    int cpha = hdl->mode == SPI_MODE_1 || hdl->mode == SPI_MODE_3;
    uint16_t clk_pin = hdl->clk_pin;
    uint16_t mosi_pin = hdl->mosi_pin;
    uint16_t miso_pin = hdl->miso_pin;

    while (len--) {
        uint8_t tx = tx_data ? (*tx_data++) : 0;
        uint8_t rx = 0;
        register uint8_t tx_bitmask_lsb = hdl->endianess_tx == SPI_TRANSMISSION_LSB_FIRST;
        register uint8_t rx_bitmask_lsb = hdl->endianess_rx == SPI_TRANSMISSION_LSB_FIRST;
        register uint8_t tx_bitmask = tx_bitmask_lsb ? 0x01 : 0x80;
        register uint8_t rx_bitmask = rx_bitmask_lsb ? 0x01 : 0x80;
        for (register uint8_t b = 0; b < 8; b++) {
            if (cpha) spi_sw_gpio_set(clk_pin, !cpol);
            if (tx_data) spi_sw_gpio_set(mosi_pin, tx & tx_bitmask);
            if (tx_bitmask_lsb)
                tx_bitmask <<= 1;
            else
                tx_bitmask >>= 1;
            spi_sw_delay(hdl);
            if (!cpha) spi_sw_gpio_set(clk_pin, !cpol);

            if (rx_data) rx |= spi_sw_gpio_get(miso_pin) ? rx_bitmask : 0;
            if (rx_bitmask_lsb)
                rx_bitmask <<= 1;
            else
                rx_bitmask >>= 1;
            if (cpha) spi_sw_gpio_set(clk_pin, cpol);
            spi_sw_delay(hdl);
            if (!cpha) spi_sw_gpio_set(clk_pin, cpol);
        }
        if (rx_data) *rx_data++ = rx;
    }
    spi_sw_gpio_set(clk_pin, cpol);
    return 0;
}
