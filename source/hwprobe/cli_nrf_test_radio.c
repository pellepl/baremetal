#if FAMILY==nrf52 && CONFIG_NRF52_TEST_RADIO==1

#include "cli.h"
#include "minio.h"
#include "radio_test.h"
#include "nrf.h"
#include "cpu.h"

static uint8_t mode = RADIO_MODE_MODE_Ble_1Mbit;     /**< Radio mode. Data rate and modulation. */
static uint8_t txpower = RADIO_TXPOWER_TXPOWER_0dBm; /**< Radio output power. */
static uint8_t channel_start = 0;                    /**< Radio start channel (frequency). */

extern volatile transmit_pattern_t g_pattern;        /**< Radio transmission pattern. */
extern uint8_t g_rx_packet[RADIO_MAX_PAYLOAD_LEN];   /**< Buffer for the radio RX packet. */ 

static int set_mode(const char *arg) {
    if (0 == strcmp("nrf1mb", arg)) {
        mode = RADIO_MODE_MODE_Nrf_1Mbit;
    } else 
    if (0 == strcmp("nrf2mb", arg)) {
        mode = RADIO_MODE_MODE_Nrf_2Mbit;
    } else 
#ifdef  NRF52832_XXAA
    if (0 == strcmp("nrf250kb", arg)) {
        mode = RADIO_MODE_MODE_Nrf_250Kbit;
    } else 
#endif
    if (0 == strcmp("1mb", arg)) {
        mode = RADIO_MODE_MODE_Ble_1Mbit;
    } else 
    if (0 == strcmp("2mb", arg)) {
        mode = RADIO_MODE_MODE_Ble_2Mbit;
    } else 
#if defined(NRF52840_XXAA) || defined(NRF52833_XXAA)
    if (0 == strcmp("125kb", arg)) {
        mode = RADIO_MODE_MODE_Ble_LR125Kbit;
    } else 
    if (0 == strcmp("500kb", arg)) {
        mode = RADIO_MODE_MODE_Ble_LR500Kbit;
    } else 
    if (0 == strcmp("250kb", arg)) {
        mode = RADIO_MODE_MODE_Ieee802154_250Kbit;
    } else 
#endif
    {
        printf("unknown mode \"%s\"\n", arg);
        return -1;
    }
    return 0;
}

static int set_txpower(const char *arg) {
#if defined(NRF52840_XXAA) || defined(NRF52833_XXAA)
    if (0 == strcmp("+8", arg)) {
        txpower = RADIO_TXPOWER_TXPOWER_Pos8dBm;
    } else 
    if (0 == strcmp("+7", arg)) {
        txpower = RADIO_TXPOWER_TXPOWER_Pos7dBm;
    } else
    if (0 == strcmp("+6", arg)) {
        txpower = RADIO_TXPOWER_TXPOWER_Pos6dBm;
    } else 
    if (0 == strcmp("+5", arg)) {
        txpower = RADIO_TXPOWER_TXPOWER_Pos5dBm;
    } else 
    if (0 == strcmp("+2", arg)) {
        txpower = RADIO_TXPOWER_TXPOWER_Pos2dBm;
    } else 
#endif
    if (0 == strcmp("+3", arg)) {
        txpower = RADIO_TXPOWER_TXPOWER_Pos3dBm;
    } else 
    if (0 == strcmp("+4", arg)) {
        txpower = RADIO_TXPOWER_TXPOWER_Pos4dBm;
    } else 
    if (0 == strcmp("0", arg)) {
        txpower = RADIO_TXPOWER_TXPOWER_0dBm;
    } else 
    if (0 == strcmp("-4", arg)) {
        txpower = RADIO_TXPOWER_TXPOWER_Neg4dBm;
    } else 
    if (0 == strcmp("-8", arg)) {
        txpower = RADIO_TXPOWER_TXPOWER_Neg8dBm;
    } else 
    if (0 == strcmp("-12", arg)) {
        txpower = RADIO_TXPOWER_TXPOWER_Neg12dBm;
    } else 
    if (0 == strcmp("-16", arg)) {
        txpower = RADIO_TXPOWER_TXPOWER_Neg16dBm;
    } else 
    if (0 == strcmp("-20", arg)) {
        txpower = RADIO_TXPOWER_TXPOWER_Neg20dBm;
    } else 
    if (0 == strcmp("-30", arg)) {
        txpower = RADIO_TXPOWER_TXPOWER_Neg30dBm;
    } else 
    if (0 == strcmp("-40", arg)) {
        txpower = RADIO_TXPOWER_TXPOWER_Neg40dBm;
    } else 
    {
        printf("unknown power \"%s\"\n", arg);
        return -1;
    }
    return 0;
}

static int ieee_channel_check(uint8_t channel) {
#if defined(NRF52840_XXAA) || defined(NRF52833_XXAA)
    if (mode == RADIO_MODE_MODE_Ieee802154_250Kbit) {
        if ((channel < IEEE_MIN_CHANNEL) || (channel > IEEE_MAX_CHANNEL)) {
            printf("for IEEE802154, channels %d to %d apply\n", IEEE_MIN_CHANNEL, IEEE_MAX_CHANNEL);
            return -1;
        }
    }
#endif // defined(NRF52840_XXAA) || defined(NRF52833_XXAA)
    return 0;
}

static int radio_tx_carrier(int argc, const char **argv, int modulated) {
    if (argc != 3) {
        printf("[<power>, <mode>, <channel>]\n");
        return -1;
    }
    int res;
    res = set_txpower(argv[0]);
    res |= set_mode(argv[1]);
    channel_start = atoi(argv[2]);
    res |= ieee_channel_check(channel_start);
    if (res == 0) {
        if (modulated) {
            radio_modulated_tx_carrier(txpower, mode, channel_start);
        } else {
            radio_unmodulated_tx_carrier(txpower, mode, channel_start);
        }
    }
    return res;
}

static int cli_radio_tx_carrier_unmodulated(int argc, const char **argv) {
    return radio_tx_carrier(argc, argv, 0);
}
CLI_FUNCTION(cli_radio_tx_carrier_unmodulated, "rf_tx_carr_unmod");

static int cli_radio_tx_carrier_modulated(int argc, const char **argv) {
    return radio_tx_carrier(argc, argv, 1);
}
CLI_FUNCTION(cli_radio_tx_carrier_modulated, "rf_tx_carr_mod");

static int cli_radio_tx_pattern(int argc, const char **argv) {
    if (argc != 1) {
        printf("[rnd|11110000|11001100]\n");
        return -1;
    }
    if (0 == strcmp("rnd", argv[0])) {
        g_pattern = TRANSMIT_PATTERN_RANDOM;
    } else 
    if (0 == strcmp("11110000", argv[0])) {
        g_pattern = TRANSMIT_PATTERN_11110000;
    } else 
    if (0 == strcmp("11001100", argv[0])) {
        g_pattern = TRANSMIT_PATTERN_11001100;
    } else 
    {
        printf("unkown pattern \"%s\"\n", argv[0]);
        return -1;
    }
    return 0;
}
CLI_FUNCTION(cli_radio_tx_pattern, "rf_tx_pattern");

static int cli_toggle_dc(int argc, const char ** argv) {
    if (argc != 1) {
        printf("[0|1]\n");
        return -1;
    }
    toggle_dcdc_state(atoi(argv[0]) ? 1 : 0);

#ifdef NRF52840_XXAA
    uint32_t dcdcen = NRF_POWER->DCDCEN;

    printf("DCDC REG0:%u\n"
           "DCDC REG1:%u\n"
           "'0' to toggle REG0\n"
           "'1' to toggle REG1\n",
        NRF_POWER->DCDCEN0,
        dcdcen);
#else
    printf("DCDC:%u\n"
           "'1':enable, '0':disable\n",
                    NRF_POWER->DCDCEN);
#endif
    return 0;
}
CLI_FUNCTION(cli_toggle_dc, "rf_dcdc");

static int cli_radio_rx(int argc, const char **argv) {
    if (argc != 2) {
        printf("[<mode>, <channel>]\n");
        return -1;
    }
    int res;
    res = set_mode(argv[0]);
    channel_start = atoi(argv[1]);
    res |= ieee_channel_check(channel_start);
    if (res == 0) {
        memset(g_rx_packet, 0, sizeof(g_rx_packet));
        radio_rx(mode, channel_start);
    }
    return res;
}
CLI_FUNCTION(cli_radio_rx, "rf_rx");

static int cli_rx_dump(int argc, const char **argv) {
    uint32_t size;
#if defined(NRF52840_XXAA) || defined(NRF52833_XXAA)
    if (mode == RADIO_MODE_MODE_Ieee802154_250Kbit) {
        size = IEEE_MAX_PAYLOAD_LEN;
    }
    else
#endif // defined(NRF52840_XXAA) || defined(NRF52833_XXAA)
    {
        size = sizeof(g_rx_packet);
    }

    fprint_mem(UART_STD, g_rx_packet, size);
    return 0;
}
CLI_FUNCTION(cli_rx_dump, "rf_rx_dump");

static int cli_radio_stop(int argc, const char **argv) {
    radio_sweep_end();
    return 0;
}
CLI_FUNCTION(cli_radio_stop, "rf_stop");

static int cli_radio_get_rssi(int argc, const char **argv) {
    if (argc != 2) {
        printf("[<mode>, <channel>]\n");
        return -1;
    }
    int res;
    res = set_mode(argv[0]);
    channel_start = atoi(argv[1]);
    res |= ieee_channel_check(channel_start);
    if (res == 0) {
        int32_t rssi = radio_get_rssi(mode, channel_start);
        printf("rssi: %d\n", rssi);
    }
    return res;
}
CLI_FUNCTION(cli_radio_get_rssi, "rf_rssi");

static int cli_radio_spectrum(int argc, const char **argv) {
    if (argc < 2 || argc > 3) {
        printf("[<channel-start>, <channel-end>(, <times>)]\n");
        return -1;
    }
    uint8_t ch_start = atoi(argv[0]);
    uint8_t ch_end = atoi(argv[1]);
    uint32_t times = argc < 3 ? 0 : atoi(argv[2]);
    printf("rssi sweep ch %d--%d\n", ch_start, ch_end);
    if (times == 0) {
        // until press, flush uart input
        do {
            cpu_halt(10);
        } while (uart_pollchar(UART_STD) >= 0);
        printf("abort by press a key\n");
    }
    do {
        printf("spec: ");
        for (uint8_t ch = ch_start; ch < ch_end; ch++) {
            int32_t rssi = radio_get_rssi(RADIO_MODE_MODE_Ble_1Mbit, ch);
            printf("%02x ", (-rssi) & 0xff);
        }
        printf("\n");
        if (times) {
            times--;
            if (times == 0) {
                break;
            }
        } else {
            if (uart_pollchar(UART_STD) >= 0) {
                break;
            }
        }
    } while (1);
    return 0;
}
CLI_FUNCTION(cli_radio_spectrum, "rf_spectrum");

static int cli_radio_init(int argc, const char **argv) {
    radio_init();
    return 0;
}
CLI_FUNCTION(cli_radio_init, "rf_init");

#endif