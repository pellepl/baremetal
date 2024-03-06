#include "uart_prodtest.h"
#include "uart_driver.h"
#include "uart_hal.h"
#include "targets.h"
#include "ringbuffer.h"

static volatile int uart_char = -1;

static uint8_t rx_buf[32];
static ringbuffer_t rb;

void uart_prodtest_init(void) {
    ringbuffer_init(&rb, rx_buf, sizeof(rx_buf));
    const target_t *t = target_get();
    uart_config_t cfg = {
        .baudrate = UART_BAUDRATE_115200,
        .parity = UART_PARITY_NONE,
        .stopbits = UART_STOPBITS_1,
        .flowcontrol = UART_FLOWCONTROL_NONE};

    uart_hal_init(0, &cfg, t->uart.pin_rx, t->uart.pin_tx, BOARD_PIN_UNDEF, BOARD_PIN_UNDEF);

    NRF_UART0->INTENSET = (UARTE_INTEN_RXDRDY_Enabled << UARTE_INTEN_RXDRDY_Pos) |
                          (UARTE_INTEN_RXTO_Enabled << UARTE_INTEN_RXTO_Pos);
    NVIC_EnableIRQ(UARTE0_UART0_IRQn);
}

void uart_prodtest_deinit(void) {
    const target_t *t = target_get();
    target_reset_pin(t->uart.pin_rx);
    target_reset_pin(t->uart.pin_tx);

    NVIC_DisableIRQ(UARTE0_UART0_IRQn);
    NRF_UART0->INTENCLR = (UARTE_INTEN_RXDRDY_Enabled << UARTE_INTEN_RXDRDY_Pos) |
                          (UARTE_INTEN_RXTO_Enabled << UARTE_INTEN_RXTO_Pos);
    NRF_UART0->TASKS_STOPRX = 1;
    NRF_UART0->TASKS_STOPTX = 1;
    NRF_UART0->ENABLE = 0;
}

int uart_prodtest_poll(void) {
    uint8_t x;
    return ringbuffer_getc(&rb, &x) < 0 ? -1 : x;
}

void UARTE0_UART0_IRQHandler(void);
void UARTE0_UART0_IRQHandler(void)
{
    if (NRF_UART0->EVENTS_RXDRDY) {
        char new_char = NRF_UART0->RXD;
        ringbuffer_putc(&rb, new_char);
        NRF_UART0->EVENTS_RXDRDY = 0;
    }
    if (NRF_UART0->EVENTS_RXTO) {
        NRF_UART0->EVENTS_RXTO = 0;
    }
}
