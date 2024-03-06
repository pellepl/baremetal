#ifndef _UART_PRODTEST_H_
#define _UART_PRODTEST_H_

#include "uart_driver.h"

void uart_prodtest_init(void);
void uart_prodtest_deinit(void);
// returns -1 if no character, otherwise returns character
int uart_prodtest_poll(void);

#endif // _UART_PRODTEST_H_
