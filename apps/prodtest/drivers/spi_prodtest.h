#ifndef _SPI_PRODTEST_H_
#define _SPI_PRODTEST_H_

#include "targets.h"

typedef enum {

    SPI_BUS_GENERIC = 0,
    // only on nrf52840
    SPI_BUS_FAST,
    MAX_SPI_BUSSES
} spi_bus_t;

typedef struct {
    struct {
        uint16_t clk;
        uint16_t mosi;
        uint16_t miso;
        uint16_t csn;
    } pins;
    uint32_t clk_freq;
    uint32_t mode;
} spi_config_t;

typedef void (* spi_cb_t)(spi_bus_t bus, int err, const uint8_t *tx_data, uint16_t tx_len, uint8_t *rx_data, uint16_t rx_len);

int spi_init(spi_bus_t bus, const spi_config_t *cfg);
int spi_transceive_async(spi_bus_t bus, const uint8_t *tx_data, uint16_t tx_len, uint8_t *rx_data, uint16_t rx_len, spi_cb_t cb);
int spi_transceive_blocking(spi_bus_t bus, const uint8_t *tx_data, uint16_t tx_len, uint8_t *rx_data, uint16_t rx_len);
void spi_deinit(spi_bus_t bus);

#endif // _SPI_PRODTEST_H_
