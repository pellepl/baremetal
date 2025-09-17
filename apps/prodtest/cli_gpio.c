/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "gpio_driver.h"
#include "cli.h"
#include "minio.h"
#include "board.h"
#include "targets.h"

#if NO_EXTRA_CLI == 0

static int cli_gpio_set(int argc, const char **argv)
{
    if (argc != 2)
    {
        return ERR_CLI_EINVAL;
    }
    int pin = strtol(argv[0], 0, 0);
    int set = strtol(argv[1], 0, 0);
    return gpio_set(pin, set);
}
CLI_FUNCTION(cli_gpio_set, "gpio_set", "set gpio: <pin> 0|1");

static int cli_gpio_read(int argc, const char **argv)
{
    if (argc != 1)
    {
        return ERR_CLI_EINVAL;
    }
    int pin = strtol(argv[0], 0, 0);
    int val = gpio_read(pin);
    if (val >= 0)
    {
        printf("%d\n", val ? 1 : 0);
    }
    else
    {
        return val;
    }
    return 0;
}
CLI_FUNCTION(cli_gpio_read, "gpio_read", "read gpio: <pin>");

static int cli_gpio_config(int argc, const char **argv)
{
    if (argc < 2 || argc > 4)
    {
        return ERR_CLI_EINVAL;
    }
    int pin = strtol(argv[0], 0, 0);
    gpio_direction_t dir = argv[1][0] == 'o' ? GPIO_DIRECTION_OUTPUT : GPIO_DIRECTION_INPUT;
    gpio_pull_t pull = GPIO_PULL_NONE;
    uint32_t drive = GPIO_PIN_CNF_DRIVE_S0S1;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "up") == 0) {
            pull = GPIO_PULL_UP;
        } else if (strcmp(argv[i], "down") == 0) {
            pull = GPIO_PULL_DOWN;
        } else if (strcmp(argv[i], "none") == 0) {
            pull = GPIO_PULL_NONE;
        } else if (strcmp(argv[i], "s0s1") == 0) {
            drive = GPIO_PIN_CNF_DRIVE_S0S1;
        } else if (strcmp(argv[i], "h0s1") == 0) {
            drive = GPIO_PIN_CNF_DRIVE_H0S1;
        } else if (strcmp(argv[i], "s0h1") == 0) {
            drive = GPIO_PIN_CNF_DRIVE_S0H1;
        } else if (strcmp(argv[i], "h0h1") == 0) {
            drive = GPIO_PIN_CNF_DRIVE_H0H1;
        } else if (strcmp(argv[i], "d0s1") == 0) {
            drive = GPIO_PIN_CNF_DRIVE_D0S1;
        } else if (strcmp(argv[i], "d0h1") == 0) {
            drive = GPIO_PIN_CNF_DRIVE_D0H1;
        } else if (strcmp(argv[i], "s0d1") == 0) {
            drive = GPIO_PIN_CNF_DRIVE_S0D1;
        } else if (strcmp(argv[i], "h0d1") == 0) {
            drive = GPIO_PIN_CNF_DRIVE_H0D1;
        } else {
            return ERR_CLI_EINVAL;
        }
    }
    int err = gpio_config(pin, dir, pull);
    if (!err) {
        NRF_GPIO_Type *port = pin < 32 ? NRF_P0 : NRF_P1;
        port->PIN_CNF[pin & 0x1f] &= ~GPIO_PIN_CNF_DRIVE_Msk;
        port->PIN_CNF[pin & 0x1f] |= drive << GPIO_PIN_CNF_DRIVE_Pos;
    }

    return err;
}
CLI_FUNCTION(cli_gpio_config, "gpio_config", "configure gpio: <pin> out|in (up|down|none) (s0s1|h0s1|s0h1|h0h1|d0s1|d0h1|s0d1|h0d1)");

static void gpio_dump(uint16_t pin)
{
    uint8_t port = pin >> 5;
    NRF_GPIO_Type *hw = pin < 32 ? NRF_P0 : NRF_P1;
    pin &= 0x1f;
    uint32_t cfg = hw->PIN_CNF[pin];
    printf("P%d.%02d  ", port, pin);
    printf("%s  ", (cfg & GPIO_PIN_CNF_DIR_Msk) == (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) ? "out" : "in ");
    printf("%s  ", (cfg & GPIO_PIN_CNF_INPUT_Msk) == (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos) ? "   " : "con");
    printf("%s ", (const char *[4]){"    ", "down", "2?  ", "up  "}[((cfg & GPIO_PIN_CNF_PULL_Msk) >> GPIO_PIN_CNF_PULL_Pos)]);
    printf("%s ", (const char *[8]){"s0s1", "h0s1", "s0h1", "h0h1", "d0s1", "d0h1", "s0d1", "h0d1"}[((cfg & GPIO_PIN_CNF_DRIVE_Msk) >> GPIO_PIN_CNF_DRIVE_Pos)]);
    printf("%s  ", (const char *[4]){"dis", "1? ", "hi ", "lo "}[((cfg & GPIO_PIN_CNF_SENSE_Msk) >> GPIO_PIN_CNF_SENSE_Pos)]);
    printf("%d\r\n", (hw->IN & (1 << pin)) ? 1 : 0)
}

static int cli_gpio_dump(int argc, const char **argv)
{
    uint32_t pins = (NRF_FICR->INFO.PART == 0x52840 || NRF_FICR->INFO.PART == 0x52833) ? 48 : 32;
    printf("PIN    DIR  INP  PULL DRIV SENS STAT\r\n");
    if (argc == 0)
    {
        for (uint16_t p = 0; p < pins; p++)
        {
            gpio_dump(p);
        }
    }
    else
    {
        for (int i = 0; i < argc; i++)
        {
            uint16_t p = strtol(argv[i], NULL, 0);
            if (p < pins)
            {
                gpio_dump(p);
            }
        }
    }
    return ERROR_OK;
}
CLI_FUNCTION(cli_gpio_dump, "gpio_dump", "dump nrf52 pin config: (<pin>)*");

#endif
