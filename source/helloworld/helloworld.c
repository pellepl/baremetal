#include "board.h"
#include "gpio_driver.h"
#include "uart_driver.h"
#include "minio.h"

int main(void) {
    uint32_t ix;
    cpu_init();
    board_init();
    gpio_init();
    for (ix = 0; ix < BOARD_LED_COUNT; ix++) {
        gpio_config(BOARD_LED_GPIO_PIN[ix], GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
    }
    uart_config_t cfg = {
        .baudrate = UART_BAUDRATE_115200,
        .parity = UART_PARITY_NONE,
        .stopbits = UART_STOPBITS_1,
        .flowcontrol = UART_FLOWCONTROL_NONE
    };
    uart_init(UART_STD, &cfg);

    int d = 0;
    while(1) {
        printf("Hello world %d!\n", d++);
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
