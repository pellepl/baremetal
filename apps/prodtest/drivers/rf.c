
#include "cli.h"
#include "minio.h"
#include "rf.h"
#include "nrf.h"
#include "cpu.h"


#define BLE_CH_MIN          0
#define BLE_CH_MAX          39

static uint8_t cur_phy = LE_PHY_1M;
static volatile uint32_t radio_irqs;
static volatile uint32_t rx_pkt_count;
static volatile int dtm_rxing = 0;


void rf_enable(void) {
    radio_irqs = 0;
    NRF_POWER->DCDCEN = POWER_DCDCEN_DCDCEN_Enabled;
    NRF_RADIO->INTENSET = RADIO_INTENSET_END_Msk;
    NVIC_ClearPendingIRQ(RADIO_IRQn);
    NVIC_EnableIRQ(RADIO_IRQn);
}

void rf_disable(void) {
    dtm_rxing = 0;
    NVIC_DisableIRQ(RADIO_IRQn);
    NRF_POWER->DCDCEN = POWER_DCDCEN_DCDCEN_Disabled;
}

void rf_dtm_set_phy(uint8_t phy) {
    cur_phy = phy;
}

int rf_dtm_tx(uint8_t channel, uint8_t length, uint8_t pattern, int8_t power) {
    rf_enable();

    if (dtm_set_txpower(power) == false) {
        return -1;
    }
    if (dtm_init() != DTM_SUCCESS) {
        return -1;
    }
    if (dtm_cmd(LE_TEST_SETUP, LE_TEST_SETUP_SET_PHY, cur_phy, pattern) != DTM_SUCCESS) {
        return -1;
    }
    if (length >> 6) {
        if (dtm_cmd(LE_TEST_SETUP, LE_TEST_SETUP_SET_UPPER, length >> 6, pattern) != DTM_SUCCESS) {
            return -1;
        }
    }
    if (dtm_cmd(LE_TRANSMITTER_TEST, channel, length, pattern) != DTM_SUCCESS) {
        return -1;
    }

    return 0;
}

int rf_dtm_rx(uint8_t channel) {
    rf_enable();

    if (dtm_init() != DTM_SUCCESS) {
        return -1;
    }
    if (dtm_cmd(LE_TEST_SETUP, LE_TEST_SETUP_SET_PHY, cur_phy, 0) != DTM_SUCCESS) {
        return -1;
    }
    if (dtm_cmd(LE_RECEIVER_TEST, channel, 0, 0) != DTM_SUCCESS) {
        return -1;
    }

    dtm_rxing = 1;

    return 0;
}

int rf_dtm_end(void) {
    int res = 0;
    if (dtm_cmd(LE_TEST_END, 0, 0, 0) != DTM_SUCCESS) {
        res = -1;
    } else {
        dtm_event_t event;
        (void)dtm_event_get(&event);
    }
    rf_disable();
    NRF_CLOCK->TASKS_HFCLKSTOP = 1;
    return res;
}

#if NO_EXTRA_CLI == 0

static int translate_transmit_pattern(const char *arg) {
    if (0 == strcmp("prbs9",arg)) {
        return DTM_PKT_PRBS9;
    } else 
    if (0 == strcmp("0f",arg)) {
        return DTM_PKT_0X0F;
    } else 
    if (0 == strcmp("55",arg)) {
        return DTM_PKT_0X55;
    } else 
    if (0 == strcmp("ff",arg)) {
        return DTM_PKT_0XFF;
    }
    return -1;
}

static int translate_tx_power(const char *arg) {
if (NRF_FICR->INFO.PART == 0x52840 || NRF_FICR->INFO.PART == 0x52833) {
    switch (atoi(arg)) {
    case 8:
        return RADIO_TXPOWER_TXPOWER_Pos8dBm;
    case 7:
        return RADIO_TXPOWER_TXPOWER_Pos7dBm;
    case 6:
        return RADIO_TXPOWER_TXPOWER_Pos6dBm;
    case 5:
        return RADIO_TXPOWER_TXPOWER_Pos5dBm;
    case 2:
        return RADIO_TXPOWER_TXPOWER_Pos2dBm;
    }
    }
    switch (atoi(arg)) {
    case 3:
        return RADIO_TXPOWER_TXPOWER_Pos3dBm;
    case 4:
        return RADIO_TXPOWER_TXPOWER_Pos4dBm;
    case 0:
        return RADIO_TXPOWER_TXPOWER_0dBm;
    case -4:
        return RADIO_TXPOWER_TXPOWER_Neg4dBm;
    case -8:
        return RADIO_TXPOWER_TXPOWER_Neg8dBm;
    case -12:
        return RADIO_TXPOWER_TXPOWER_Neg12dBm;
    case -16:
        return RADIO_TXPOWER_TXPOWER_Neg16dBm;
    case -20:
        return RADIO_TXPOWER_TXPOWER_Neg20dBm;
    case -30:
        return RADIO_TXPOWER_TXPOWER_Neg30dBm;
    case -40:
        return RADIO_TXPOWER_TXPOWER_Neg40dBm;
    }
    return -1;
}

static int cli_dtm_set_phy(int argc, const char **argv) {
    if (argc != 1) {
        printf("[1mb|2mb|codeds8|codeds2]\n");
        return -1;
    }
    if (0 == strcmp("1mb", argv[0])) {
        cur_phy = LE_PHY_1M;
    } else
    if (0 == strcmp("2mb", argv[0])) {
        cur_phy = LE_PHY_2M;
    } else
    if (0 == strcmp("codeds8", argv[0])) {
        cur_phy = LE_PHY_LE_CODED_S8;
    } else
    if (0 == strcmp("codeds2", argv[0])) {
        cur_phy = LE_PHY_LE_CODED_S2;
    } else {
        printf("illegal phy \"%s\"\n", argv[0]);
        return -1;
    }
    return 0;
}
CLI_FUNCTION(cli_dtm_set_phy, "dtm_set_phy", "select phy: 1mb|2mb|codeds8|codeds2");

static int cli_dtm_tx(int argc, const char **argv) {
    if (argc != 4) {
        printf("[<channel>, <length>, prbs9|0f|55|ff, <tx-power>]\n");
        return -1;
    }
    int channel = atoi(argv[0]);
    if (channel < BLE_CH_MIN || channel > BLE_CH_MAX) {
        printf("illegal channel \"%s\"\n", argv[0]);
        return -1;
    }
    int length = atoi(argv[1]);
    int pattern = translate_transmit_pattern(argv[2]);
    if (pattern < 0) {
        printf("illegal pattern \"%s\"\n", argv[2]);
        return -1;
    }
    int tx_pow = translate_tx_power(argv[3]);
    if (tx_pow < 0) {
        printf("illegal power \"%s\"\n", argv[3]);
        return -1;
    }

    return rf_dtm_tx(channel, length, pattern, tx_pow);    
}
CLI_FUNCTION(cli_dtm_tx, "dtm_tx", "transmit: <channel:0-39> <length> prbs9|0f|55|ff <tx-power:4|3|0|-4|-8|-12|-16|-20|-30|-40 (8|7|6|5|2 for nrf52840 and nrf52833)>");

static int cli_dtm_rx(int argc, const char **argv) {
    if (argc != 1) {
        printf("[<channel>]\n");
        return -1;
    }

    int channel = atoi(argv[0]);
    if (channel < BLE_CH_MIN || channel > BLE_CH_MAX) {
        printf("illegal channel \"%s\"\n", argv[0]);
        return -1;
    }

    return rf_dtm_rx(channel);
}
CLI_FUNCTION(cli_dtm_rx, "dtm_rx", "receive: <channel:0-39>");

static int cli_dtm_end(int argc, const char **argv) {
    printf("radio irqs:  %d\n", radio_irqs);
    printf("rx pkt count:%d\n", rx_pkt_count);
    return rf_dtm_end();
}
CLI_FUNCTION(cli_dtm_end, "dtm_end", "disables radio and prints rf info");

#endif

void RADIO_IRQHandler(void);
void RADIO_IRQHandler(void) {
    if (NRF_RADIO->EVENTS_END && (NRF_RADIO->INTENSET & RADIO_INTENSET_END_Msk)) {
        NRF_RADIO->EVENTS_END = 0;
        radio_irqs++;
        //(void)dtm_wait();
        if (dtm_rxing) {
            NRF_RADIO->TASKS_RXEN = 1;
            if ((NRF_RADIO->CRCSTATUS == 1))// && check_pdu())
            {
                rx_pkt_count++;
            }
        }
        NVIC_ClearPendingIRQ(RADIO_IRQn);
    }
}
