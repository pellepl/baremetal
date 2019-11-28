#include "board.h"
#include "gpio_driver.h"
#include "uart_driver.h"
#include "flash_driver.h"
#include "minio.h"
#include "cli.h"

static int cli_help(int argc, const char **argv) {
    int len = cli_entry_list_length();
    cli_entry_t **list = cli_entry_list_start();
    for (int i = 0; i < len; i++) {
        printf("\t%s\n", list[i]->name);
    }
    return 0;
}
CLI_FUNCTION(cli_help, "help");

static int cli_reset(int argc, const char **argv) {
    cpu_reset();
    return 0;
}
CLI_FUNCTION(cli_reset, "reset");

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
        default:
            printf("ERROR (%d)\n", res);
            break;
    } 
}

int main(void) {
    cpu_init();
    board_init();
    gpio_init();
    flash_init();
    uart_config_t cfg = {
        .baudrate = UART_BAUDRATE_115200,
        .parity = UART_PARITY_NONE,
        .stopbits = UART_STOPBITS_1,
        .flowcontrol = UART_FLOWCONTROL_NONE
    };
    uart_init(0, &cfg);

    printf("\nhwprobe cli\n");
    cli_init(cli_cb, "\r\n;", " ,", "", "");

    while(1) {
        int rx = uart_getchar(UART_STD);
        if (rx > 0) {
            char c = (char)rx;
            cli_parse(&c, 1);
        }
    }
}
