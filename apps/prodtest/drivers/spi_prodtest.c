#include "targets.h"
#include "spi_prodtest.h"
#include "gpio_driver.h"
#include "minio.h"
#include "eventqueue.h"

#define SPI_CLK_FREQ_K125 0x02000000 // 125 kbps
#define SPI_CLK_FREQ_K250 0x04000000 // 250 kbps
#define SPI_CLK_FREQ_K500 0x08000000 // 500 kbps
#define SPI_CLK_FREQ_M1 0x10000000 // 1 Mbps
#define SPI_CLK_FREQ_M2 0x20000000 // 2 Mbps
#define SPI_CLK_FREQ_M4 0x40000000 // 4 Mbps
#define SPI_CLK_FREQ_M8 0x80000000 // 8 Mbps
#define SPI_CLK_FREQ_M16 0x0A000000 // 16 Mbps
#define SPI_CLK_FREQ_M32 0x14000000 // 32 Mbps

#define SPI_CFG_MSB_FIRST (0 << 0)
#define SPI_CFG_LSB_FIRST (1 << 0)
#define SPI_CFG_CPHA_LEAD (0 << 1)
#define SPI_CFG_CPHA_TRAIL (1 << 1)
#define SPI_CFG_CPOL_HIGH (0 << 2)
#define SPI_CFG_CPOL_LOW (1 << 2)

#define SPI_CFG_MODE_0 (SPIM_CONFIG_ORDER_MsbFirst | (SPIM_CONFIG_CPHA_Leading << SPIM_CONFIG_CPHA_Pos) | (SPIM_CONFIG_CPOL_ActiveHigh << SPIM_CONFIG_CPOL_Pos))
#define SPI_CFG_MODE_1 (SPIM_CONFIG_ORDER_MsbFirst | (SPIM_CONFIG_CPHA_Trailing << SPIM_CONFIG_CPHA_Pos) | (SPIM_CONFIG_CPOL_ActiveHigh << SPIM_CONFIG_CPOL_Pos))
#define SPI_CFG_MODE_2 (SPIM_CONFIG_ORDER_MsbFirst | (SPIM_CONFIG_CPHA_Leading << SPIM_CONFIG_CPHA_Pos) | (SPIM_CONFIG_CPOL_ActiveLow << SPIM_CONFIG_CPOL_Pos))
#define SPI_CFG_MODE_3 (SPIM_CONFIG_ORDER_MsbFirst | (SPIM_CONFIG_CPHA_Trailing << SPIM_CONFIG_CPHA_Pos) | (SPIM_CONFIG_CPOL_ActiveLow << SPIM_CONFIG_CPOL_Pos))

static const struct {
    NRF_SPIM_Type *hw;
    uint32_t irqn;
} bus_hw[MAX_SPI_BUSSES] = {
    [SPI_BUS_GENERIC] = {
        .hw = NRF_SPIM1,
        .irqn = SPIM1_SPIS1_TWIM1_TWIS1_SPI1_TWI1_IRQn},
    [SPI_BUS_FAST] = {.hw = NRF_SPIM3, .irqn = SPIM3_IRQn}};

static struct spi_state {
    bool init;
    spi_config_t cfg;
    const uint8_t *tx_data;
    uint16_t tx_len;
    uint8_t *rx_data;
    uint16_t rx_len;
    spi_cb_t cb;
    volatile enum {
        BUS_IDLE = 0,
        BUS_SETUP,
        BUS_RUNNING,
    } state;
} state[MAX_SPI_BUSSES];

int spi_init(spi_bus_t spi_bus, const spi_config_t *cfg)
{
    if (spi_bus == SPI_BUS_FAST && NRF_FICR->INFO.PART != 0x52833 && NRF_FICR->INFO.PART != 0x52840) {
        return -ENOTSUP;
    }
    struct spi_state *bus = &state[spi_bus];
    NRF_SPIM_Type *hw = bus_hw[spi_bus].hw;

    if (bus->init)
        return -EALREADY;
    bus->init = true;
    memcpy(&bus->cfg, cfg, sizeof(spi_config_t));
    bus->state = BUS_IDLE;

    gpio_set(bus->cfg.pins.csn, 1);
    gpio_set(bus->cfg.pins.clk, cfg->mode <= 1 ? 0 : 1);
    gpio_set(bus->cfg.pins.mosi, 0);
    gpio_config(bus->cfg.pins.clk, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
    NRF_GPIO_Type *port = bus->cfg.pins.clk < 32 ? NRF_P0 : NRF_P1;
    port->PIN_CNF[bus->cfg.pins.clk & 0x1f] |= GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos;
    gpio_config(bus->cfg.pins.mosi, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
    gpio_config(bus->cfg.pins.miso, GPIO_DIRECTION_INPUT, GPIO_PULL_DOWN);
    gpio_config(bus->cfg.pins.csn, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
    hw->PSEL.SCK = bus->cfg.pins.clk;
    hw->PSEL.MOSI = bus->cfg.pins.mosi;
    hw->PSEL.MISO = bus->cfg.pins.miso;
    hw->PSEL.CSN = -1;
    hw->PSELDCX = -1;

    hw->INTENSET =
        (SPIM_INTENSET_STOPPED_Disabled << SPIM_INTENSET_STOPPED_Pos) |
        (SPIM_INTENSET_ENDRX_Disabled << SPIM_INTENSET_ENDRX_Pos) |
        (SPIM_INTENSET_ENDTX_Disabled << SPIM_INTENSET_ENDTX_Pos) |
        (SPIM_INTENSET_END_Enabled << SPIM_INTENSET_END_Pos) |
        (SPIM_INTENSET_STARTED_Enabled << SPIM_INTENSET_STARTED_Pos);

    switch (bus->cfg.mode)
    {
    case 0:
        hw->CONFIG = SPI_CFG_MODE_0;
        break;
    case 1:
        hw->CONFIG = SPI_CFG_MODE_1;
        break;
    case 2:
        hw->CONFIG = SPI_CFG_MODE_2;
        break;
    default:
    case 3:
        hw->CONFIG = SPI_CFG_MODE_3;
        break;
    }

    // only SPI_BUS_FAST can handle >8MHz
    if (bus->cfg.clk_freq <= 125000)
        hw->FREQUENCY = SPI_CLK_FREQ_K125;
    else if (bus->cfg.clk_freq <= 250000)
        hw->FREQUENCY = SPI_CLK_FREQ_K250;
    else if (bus->cfg.clk_freq <= 500000)
        hw->FREQUENCY = SPI_CLK_FREQ_K500;
    else if (bus->cfg.clk_freq <= 1000000)
        hw->FREQUENCY = SPI_CLK_FREQ_M1;
    else if (bus->cfg.clk_freq <= 2000000)
        hw->FREQUENCY = SPI_CLK_FREQ_M2;
    else if (bus->cfg.clk_freq <= 4000000)
        hw->FREQUENCY = SPI_CLK_FREQ_M4;
    else if (bus->cfg.clk_freq <= 8000000 || spi_bus == SPI_BUS_GENERIC)
        hw->FREQUENCY = SPI_CLK_FREQ_M8;
    else if (bus->cfg.clk_freq <= 16000000)
        hw->FREQUENCY = SPI_CLK_FREQ_M16;
    else
        hw->FREQUENCY = SPI_CLK_FREQ_M32;

    NVIC_ClearPendingIRQ(bus_hw[spi_bus].irqn);
    NVIC_EnableIRQ(bus_hw[spi_bus].irqn);

    hw->ENABLE = (SPIM_ENABLE_ENABLE_Enabled << SPIM_ENABLE_ENABLE_Pos);

    return ERROR_OK;
}

void spi_deinit(spi_bus_t spi_bus) {
    if (spi_bus == SPI_BUS_FAST && NRF_FICR->INFO.PART != 0x52833 && NRF_FICR->INFO.PART != 0x52840) {
        return;
    }
    struct spi_state *bus = &state[spi_bus];
    NRF_SPIM_Type *hw = bus_hw[spi_bus].hw;

    if (!bus->init)
        return;
    NVIC_DisableIRQ(bus_hw[spi_bus].irqn);
    hw->INTENCLR = 0xffffffff;
    NVIC_ClearPendingIRQ(bus_hw[spi_bus].irqn);
    hw->EVENTS_STOPPED = 0;
    hw->TASKS_STOP = 1;
    while (hw->EVENTS_STOPPED == 0);
    hw->EVENTS_STOPPED = 0;
    hw->ENABLE = (SPIM_ENABLE_ENABLE_Disabled << SPIM_ENABLE_ENABLE_Pos);

    gpio_set(bus->cfg.pins.csn, 1);
    target_reset_pin(bus->cfg.pins.csn);
    target_reset_pin(bus->cfg.pins.mosi);
    target_reset_pin(bus->cfg.pins.miso);
    target_reset_pin(bus->cfg.pins.clk);

    bus->init = false;
}

static void start(spi_bus_t spi_bus, const uint8_t *tx_data, uint16_t tx_len, uint8_t *rx_data, uint16_t rx_len, spi_cb_t cb)
{
    struct spi_state *bus = &state[spi_bus];
    NRF_SPIM_Type *hw = bus_hw[spi_bus].hw;
    bus->tx_data = tx_data;
    bus->tx_len = tx_len;
    bus->rx_data = rx_data;
    bus->rx_len = rx_len;
    bus->cb = cb;
    if (tx_data == NULL) tx_len = 0;
    if (rx_data == NULL) rx_len = 0;
    hw->TXD.PTR = (uint32_t)tx_data;
    hw->TXD.MAXCNT = (uint32_t)tx_len;
    hw->RXD.PTR = (uint32_t)rx_data;
    hw->RXD.MAXCNT = (uint32_t)rx_len;
    gpio_set(bus->cfg.pins.csn, 0);
    hw->EVENTS_END = 0;
    hw->TASKS_START = 1;
}

int spi_transceive_async(spi_bus_t spi_bus, const uint8_t *tx_data, uint16_t tx_len, uint8_t *rx_data, uint16_t rx_len, spi_cb_t cb) {
    if (spi_bus == SPI_BUS_FAST && NRF_FICR->INFO.PART != 0x52833 && NRF_FICR->INFO.PART != 0x52840) {
        return -ENOTSUP;
    }
    struct spi_state *bus = &state[spi_bus];
    if (bus->state != BUS_IDLE) {
        return -EAGAIN;
    }
    bus->state = BUS_SETUP;
    start(spi_bus, tx_data, tx_len, rx_data, rx_len, cb);
    return ERROR_OK;
}

int spi_transceive_blocking(spi_bus_t spi_bus, const uint8_t *tx_data, uint16_t tx_len, uint8_t *rx_data, uint16_t rx_len) {
    NRF_SPIM_Type *hw = bus_hw[spi_bus].hw;
    if (spi_bus == SPI_BUS_FAST && NRF_FICR->INFO.PART != 0x52833 && NRF_FICR->INFO.PART != 0x52840) {
        return -ENOTSUP;
    }
    struct spi_state *bus = &state[spi_bus];
    if (bus->state != BUS_IDLE) {
        return -EAGAIN;
    }
    volatile int a = cpu_core_clock_freq();
    bus->state = BUS_SETUP;
    start(spi_bus, tx_data, tx_len, rx_data, rx_len, NULL);
    while (bus->state != BUS_IDLE && --a)
        ;

    gpio_set(bus->cfg.pins.csn, 1);

    if (a == 0)
    {
        hw->TASKS_STOP = 1;
        bus->state = BUS_IDLE;
        return -ETIME;
    }

    return ERROR_OK;
}

static void ev_spi_done(uint32_t ev, void *arg) {
    spi_bus_t spi_bus = (spi_bus_t)arg;
    struct spi_state *bus = &state[spi_bus];
    if (bus->cb)
        bus->cb(ERROR_OK, spi_bus, bus->tx_data, bus->tx_len, bus->rx_data, bus->rx_len);
}

static void handle_irq(spi_bus_t spi_bus)
{
    NRF_SPIM_Type *hw = bus_hw[spi_bus].hw;
    struct spi_state *bus = &state[spi_bus];
    if (hw->EVENTS_STARTED) {
        hw->EVENTS_STARTED = 0;
        state[spi_bus].state = BUS_RUNNING;
    }
    if (hw->EVENTS_ENDTX) {
        hw->EVENTS_ENDTX = 0;
    }
    if (hw->EVENTS_ENDRX) {
        hw->EVENTS_ENDRX = 0;
    }
    if (hw->EVENTS_END) {
        hw->EVENTS_END = 0;
        state[spi_bus].state = BUS_IDLE;
        gpio_set(bus->cfg.pins.csn, 1);
        eventq_add(0, (void *)spi_bus, ev_spi_done);
    }
    if (hw->EVENTS_STOPPED) {
        hw->EVENTS_STOPPED = 0;
    }
}

void SPIM1_SPIS1_TWIM1_TWIS1_SPI1_TWI1_IRQHandler(void);
void SPIM1_SPIS1_TWIM1_TWIS1_SPI1_TWI1_IRQHandler(void)
{
    handle_irq(SPI_BUS_GENERIC);
}

void SPIM3_IRQHandler(void);
void SPIM3_IRQHandler(void) {
    handle_irq(SPI_BUS_FAST);
}
