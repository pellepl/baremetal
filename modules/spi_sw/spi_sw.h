/* Copyright (c) 2020 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#ifndef _SPI_SW_H_
#define _SPI_SW_H_

// bit banging spi master

#include "bmtypes.h"

#ifndef NO_SPI_SW_DEFAULT_GPIO
#include "gpio_driver.h"
#endif

#ifndef ERR_SPI_SW_BASE
#define ERR_SPI_SW_BASE (30)
#endif

typedef enum
{
    SPI_MODE_0 = 0,
    SPI_MODE_1,
    SPI_MODE_2,
    SPI_MODE_3
} spi_sw_mode_t;

typedef enum
{
    SPI_TRANSMISSION_LSB_FIRST = 0,
    SPI_TRANSMISSION_MSB_FIRST
} spi_sw_transmission_t;

typedef struct
{
    spi_sw_mode_t mode;
    spi_sw_transmission_t endianess_tx;
    spi_sw_transmission_t endianess_rx;
    uint16_t mosi_pin;
    uint16_t miso_pin;
    uint16_t clk_pin;
    unsigned int delay; // busy loop delay between pin transitions.
#ifndef NO_SPI_SW_DEFAULT_GPIO
    uint8_t gpio_config;
    gpio_direction_t mosi_pin_dir;
    gpio_direction_t miso_pin_dir;
    gpio_pull_t mosi_pin_pull;
    gpio_pull_t miso_pin_pull;
#endif
} spi_sw_t;

void spi_sw_init(unsigned int port, spi_sw_mode_t mode, spi_sw_transmission_t endianess_tx, spi_sw_transmission_t endianess_rx,
                 uint16_t mosi_pin, uint16_t miso_pin, uint16_t clk_pin, unsigned int delay);
int spi_sw_txrx(unsigned int port, const uint8_t *tx_data, uint8_t *rx_data, uint16_t len);
int spi_sw_txrx_3wire(unsigned int port, const uint8_t *tx_data, uint8_t *rx_data, uint16_t len_tx, uint16_t len_rx);

#ifdef NO_SPI_SW_DEFAULT_GPIO
extern int spi_sw_gpio_get(uint16_t pin);
extern void spi_sw_gpio_set(uint16_t pin, uint8_t state);
#else
void spi_sw_force_pin_config(unsigned int port,
                             gpio_direction_t mosi_pin_dir,
                             gpio_direction_t miso_pin_dir,
                             gpio_pull_t mosi_pin_pull,
                             gpio_pull_t miso_pin_pull);
#endif // NO_SPI_SW_DEFAULT_GPIO

#endif /* _SPI_SW_H_ */
