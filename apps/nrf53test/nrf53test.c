/* Copyright (c) 2020 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "board.h"
#include "gpio_driver.h"

#if defined(NRF5340_XXAA_NETWORK)

int main(void) {
    uint32_t ix;
    cpu_init();
    board_init();
    gpio_init();
    for (ix = 0; ix < BOARD_LED_COUNT; ix++) {
        gpio_config(BOARD_LED_GPIO_PIN[ix], GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
    }
    while(1) {
        for (ix = 0; ix < BOARD_LED_COUNT; ix++) {
            gpio_set(BOARD_LED_GPIO_PIN[ix], BOARD_LED_GPIO_ACTIVE[ix]);
        }
        cpu_halt(500);
        for (ix = 0; ix < BOARD_LED_COUNT; ix++) {
            gpio_set(BOARD_LED_GPIO_PIN[ix], !BOARD_LED_GPIO_ACTIVE[ix]);
        }
        cpu_halt(500);
    }
}

#else // APP processor

#include "uart_driver.h"
#include "minio.h"
#include "cli.h"
#include "nrf5340_application.h"
#include "nrf5340_application_bitfields.h"

#define _str(x) __str(x)
#define __str(x) #x

static int cli_net(int argc, const char **argv) {
    NRF_RESET_S->NETWORK.FORCEOFF = RESET_NETWORK_FORCEOFF_FORCEOFF_Hold;
    printf("Start NET core\n");

#if 0 // not necessary
    printf(".. give NET GPIO LED SPU access\n");
    for (int i = 0; i < BOARD_LED_COUNT; i++) {
        uint16_t pin = BOARD_LED_GPIO_PIN[i];
        if  (pin < 32) {
            NRF_SPU_S->GPIOPORT[0].PERM |= 1 << pin;
        } else {
            NRF_SPU_S->GPIOPORT[1].PERM |= 1 << (pin & 0x1f);
        }
    }
#endif
    printf(".. config GPIO LEDs to belong to NET\n");
    for (int i = 0; i < BOARD_LED_COUNT; i++) {
        uint16_t pin = BOARD_LED_GPIO_PIN[i];
        if  (pin < 32) {
            NRF_P0_S->PIN_CNF[pin] = GPIO_PIN_CNF_MCUSEL_NetworkMCU << GPIO_PIN_CNF_MCUSEL_Pos;
        } else {
            NRF_P1_S->PIN_CNF[(pin & 0x1f)] = GPIO_PIN_CNF_MCUSEL_NetworkMCU << GPIO_PIN_CNF_MCUSEL_Pos;
        }
    }
#if 0 // not necessary
    printf(".. put NET in secure domain\n");
	NRF_SPU_S->EXTDOMAIN[0].PERM = 1 << 4;
#endif
    printf(".. release FORCE-OFF on NET\n");
    NRF_RESET_S->NETWORK.FORCEOFF = RESET_NETWORK_FORCEOFF_FORCEOFF_Release;
    return 0;
}
CLI_FUNCTION(cli_net, "net", "");

static int cli_help(int argc, const char **argv) {
    int len = cli_entry_list_length();
    cli_entry_t **list = cli_entry_list_start();
    for (int i = 0; i < len; i++) {
        printf("\t%s\n", list[i]->name);
    }
    return 0;
}
CLI_FUNCTION(cli_help, "help", "");

static int cli_reset(int argc, const char **argv) {
    cpu_reset();
    return 0;
}
CLI_FUNCTION(cli_reset, "reset", "");

static void cli_cb(const char *func_name, int res) {
    switch (res) {
        case 0:
            printf("OK\n");
            break;
        case ERR_CLI_NO_ENTRY:
            cli_help(0, (void *)0);
            printf("ERROR \"%s\" not found\n", func_name);
            break;
        case ERR_CLI_INPUT_OVERFLOW:
            printf("ERROR too long input\n");
            break;
        case ERR_CLI_ARGUMENT_OVERFLOW:
            printf("ERROR too many arguments\n");
            break;
        case ERR_CLI_SILENT:
            break;
        default:
            printf("ERROR (%d)\n", res);
            break;
    } 
}

int main(void) {
    cpu_init();
    board_init();
    gpio_init();
    uart_config_t cfg = {
        .baudrate = UART_BAUDRATE_115200,
        .parity = UART_PARITY_NONE,
        .stopbits = UART_STOPBITS_1,
        .flowcontrol = UART_FLOWCONTROL_NONE
    };
    uart_init(0, &cfg);

    printf("\nnrf53test cli\n");
    printf("target:\t%s %s/%s/%s@%s\n",
       _str(BUILD_INFO_TARGET_NAME),
       _str(BUILD_INFO_TARGET_ARCH), _str(BUILD_INFO_TARGET_FAMILY), _str(BUILD_INFO_TARGET_PROC),
       _str(BUILD_INFO_TARGET_BOARD));
    printf("rev:\t%s %s on %s\n",
       _str(BUILD_INFO_GIT_COMMIT), _str(BUILD_INFO_GIT_TAG), _str(BUILD_INFO_GIT_BRANCH))
    printf("build:\t%s@%s %s-%s %s\n",
       _str(BUILD_INFO_HOST_WHO), _str(BUILD_INFO_HOST_NAME), 
       _str(BUILD_INFO_HOST_WHEN_DATE), _str(BUILD_INFO_HOST_WHEN_TIME),
       _str(BUILD_INFO_HOST_WHEN_EPOCH));
    printf("cc:\t%s %s\n",
       _str(BUILD_INFO_CC_MACHINE),
       _str(BUILD_INFO_CC_VERSION));
    cli_init(cli_cb, "\r\n;", " ,", "", "");
    {
        uint32_t ix, times = 0;
        for (ix = 0; ix < BOARD_LED_COUNT; ix++) {
        gpio_config(BOARD_LED_GPIO_PIN[ix], GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
        }
        while(times++ < 5) {
            for (ix = 0; ix < BOARD_LED_COUNT; ix++) {
                gpio_set(BOARD_LED_GPIO_PIN[ix], BOARD_LED_GPIO_ACTIVE[ix]);
            }
            cpu_halt(100);
            for (ix = 0; ix < BOARD_LED_COUNT; ix++) {
                gpio_set(BOARD_LED_GPIO_PIN[ix], !BOARD_LED_GPIO_ACTIVE[ix]);
            }
            cpu_halt(100);
        }
    }

    while(1) {
        int rx = uart_getchar(UART_STD);
        if (rx > 0) {
            char c = (char)rx;
            cli_parse(&c, 1);
        }
    }
}

#endif // APP / NET processor