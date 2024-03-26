/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "prodtest.h"
#include "gpio_driver.h"
#include "uart_prodtest.h"
#include "minio.h"
#include "events.h"
#include "cli.h"

static void cli_cb(const char *func_name, int res)
{
    switch (res)
    {
    case 0:
        printf("OK\r\n");
        break;
    case ERR_CLI_NO_ENTRY:
        printf("ERROR \"%s\" not found\r\n", func_name);
        break;
    case ERR_CLI_INPUT_OVERFLOW:
        printf("ERROR too long input\r\n");
        break;
    case ERR_CLI_EINVAL:
        printf("ERROR invalid input\r\n");
        break;
    case ERR_CLI_ARGUMENT_OVERFLOW:
        printf("ERROR too many arguments\r\n");
        break;
    case ERR_CLI_SILENT:
        break;

    case -ENOTSUP:
        printf("ERROR not supported\r\n");
        break;
    case -ETIMEDOUT:
    case -ETIME:
        printf("ERROR timed out\r\n");
        break;
    case -EALREADY:
        printf("ERROR already in progress\r\n");
        break;
    case -EAGAIN:
        printf("ERROR temporarily unavailable\r\n");
        break;
    case -EINVAL:
        printf("ERROR invalid argument\r\n");
        break;
    case -EIO:
        printf("ERROR I/O error\r\n");
        break;
    case -EPROTO:
        printf("ERROR protocol error\r\n");
        break;
    case -EBADF:
        printf("ERROR bad state\r\n");
        break;
    case -ENXIO:
        printf("ERROR no such device\r\n");
        break;
    case -ENOMEM:
        printf("ERROR cannot allocate memory\r\n");
        break;

    default:
        printf("ERROR %d\r\n", res);
        break;
    }
}

static void handle_generic_event(uint32_t event, void *arg) {
    printf("GENERIC EVENT %08x\r\n", event);
}

int main(void)
{
    cpu_init();
    board_init();
    gpio_init();

    const target_t *t = target_set_by_uicr();

    if (t->target_init)
        t->target_init(t);

    eventq_init(handle_generic_event);

    uart_prodtest_init();

    printf("\r\nPRODTEST %s\r\n\r\n", t->name);
    cli_init(cli_cb, "\n\r;", " ,=", "", "");

    print_commands(true, 6);

    while (1)
    {
        __WFI();
        int rx;
        while ((rx = uart_prodtest_poll()) != -1) {
            char c = (char)rx;
            cli_parse(&c, 1);
        }
        yield_for_events();
    }
}

void yield_for_events(void) {
    while (eventq_run());
}

static bool is_upper(const char *s) {
    return s != NULL && (s[0] >= 'A' && s[0] <= 'Z');
}

void print_commands(bool upper, int nbr_of_spaces)
{
    char spaces[nbr_of_spaces + 1];
    memset(spaces, ' ', nbr_of_spaces);
    spaces[nbr_of_spaces] = 0;
    int len = cli_entry_list_length();
    cli_entry_t **list = cli_entry_list_start();
    for (int i = 0; i < len; i++)
    {
        if (is_upper(list[i]->name) == upper) {
            int f_len = strlen(list[i]->name);
            int spaces_ix = nbr_of_spaces - f_len;
            printf("%s%s%s\r\n", list[i]->name, spaces_ix >= 0 ? &spaces[f_len] : "", list[i]->help);
        }
    }
}

int prodtest_output_lfclk(bool enable) {
    const target_t *target = target_get();
    if (target->pin_test_clk == BOARD_PIN_UNDEF) {
        return -ENOTSUP;
    }

    if (enable) {
        // start up lf xtal
        NRF_CLOCK->TASKS_LFCLKSTOP = 1;
        NRF_CLOCK->LFCLKSRC = CLOCK_LFCLKSRC_SRC_Xtal;
        NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
        NRF_CLOCK->TASKS_LFCLKSTART = 1;
        volatile int spoonguard = cpu_core_clock_freq()/4;
        while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0 && spoonguard--);
        if (spoonguard == 0) {
            NRF_CLOCK->TASKS_LFCLKSTOP = 1;
            return -ETIME;
        }

        // wire RTC1 tick to gpio toggle task via PPI
        gpio_config(target->pin_test_clk, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
        NRF_GPIOTE->CONFIG[7] = 
            (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
            (target->pin_test_clk << GPIOTE_CONFIG_PSEL_Pos) |
            (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) |
            (GPIOTE_CONFIG_OUTINIT_Low << GPIOTE_CONFIG_OUTINIT_Pos);
        NRF_PPI->CH[0].EEP = (uint32_t)&NRF_RTC1->EVENTS_TICK;
        NRF_PPI->CH[0].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[7];
        NRF_PPI->CHENSET = 1;

        // start RTC1
        NRF_RTC1->PRESCALER = 0;
		NRF_RTC1->EVTENSET = RTC_EVTEN_TICK_Msk;
        NRF_RTC1->TASKS_START = 1;
	} else {
        // stop RTC1, disable PPI channel
        NRF_CLOCK->TASKS_LFCLKSTOP = 1;
        NRF_RTC1->TASKS_STOP = 1;
		NRF_RTC1->EVTENCLR = RTC_EVTEN_TICK_Msk;
        NRF_PPI->CHENCLR = 1;
        NRF_GPIOTE->CONFIG[7] = 0;
        gpio_config(target->pin_test_clk, GPIO_DIRECTION_INPUT, GPIO_PULL_NONE);
	}

	return ERROR_OK;
}

NRF_GPIO_Type *prodtest_port_for_pin(uint16_t pin)
{
    return pin < 32 ? NRF_P0 : NRF_P1;
}
