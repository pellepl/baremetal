#include "msp430.h"
#include "uart_hal.h"
#include "gpio_driver.h"
#include "board_common.h"

int uart_hal_init(uint32_t hdl, const uart_config_t *config, uint16_t rx_pin, uint16_t tx_pin, uint16_t rts_pin, uint16_t cts_pin) {
    gpio_config(rx_pin, GPIO_DIRECTION_FUNCTION_OUT, GPIO_PULL_UP);
    gpio_config(tx_pin, GPIO_DIRECTION_FUNCTION_OUT, GPIO_PULL_UP);
    UCA0CTL0 = 0 |
        (config->parity == UART_PARITY_NONE ? 0 : UCPEN) |
        (config->parity == UART_PARITY_EVEN ? 0 : UCPAR) |
        (config->stopbits == UART_STOPBITS_1 ? 0 : UCSPB);
    UCA0CTL1 |= UCSSEL_2; // SMCLK
    UCA0BR0 = 0x08; // 1MHz 115200
    UCA0BR1 = 0x00; // 1MHz 115200
    UCA0MCTL = UCBRS2 + UCBRS0; // Modulation UCBRSx = 5
    UCA0CTL1 &= ~UCSWRST; // **Initialize USCI state machine**
    return 0;
}

int uart_hal_tx(uint32_t hdl, char x) {
    while ((IFG2 & UCA0TXIFG) == 0);
    UCA0TXBUF = x;
    return 0;
}

int uart_hal_rx(uint32_t hdl) {
    while ((IFG2 & UCA0RXIFG) == 0);
    return UCA0RXBUF;
}

int uart_hal_rxpoll(uint32_t hdl) {
    if ((IFG2 & UCA0RXIFG) == 0)
        return -1;
    else
        return UCA0RXBUF;
}

int uart_hal_deinit(uint32_t hdl, uint16_t rx_pin, uint16_t tx_pin, uint16_t rts_pin, uint16_t cts_pin) {
    // TODO PETER
    return 0;
}

/*
   WDTCTL = WDTPW + WDTHOLD; // Stop WDT
   DCOCTL = 0; // Select lowest DCOx and MODx settings
   BCSCTL1 = CALBC1_1MHZ; // Set DCO
   DCOCTL = CALDCO_1MHZ;
   P2DIR |= 0xFF; // All P2.x outputs
   P2OUT &= 0x00; // All P2.x reset
#   P1SEL |= RXD + TXD ; // P1.1 = RXD, P1.2=TXD
#   P1SEL2 |= RXD + TXD ; // P1.1 = RXD, P1.2=TXD
#   P1DIR |= RXLED + TXLED;
   P1OUT &= 0x00;
   UCA0CTL1 |= UCSSEL_2; // SMCLK
   UCA0BR0 = 0x08; // 1MHz 115200
   UCA0BR1 = 0x00; // 1MHz 115200
   UCA0MCTL = UCBRS2 + UCBRS0; // Modulation UCBRSx = 5
   UCA0CTL1 &= ~UCSWRST; // **Initialize USCI state machine**
   UC0IE |= UCA0RXIE; // Enable USCI_A0 RX interrupt
   __bis_SR_register(CPUOFF + GIE); // Enter LPM0 w/ int until Byte RXed
   while (1)
   { }
}
 
#pragma vector=USCIAB0TX_VECTOR
__interrupt void USCI0TX_ISR(void)
{
   P1OUT |= TXLED; 
     UCA0TXBUF = string[i++]; // TX next character 
    if (i == sizeof string - 1) // TX over? 
       UC0IE &= ~UCA0TXIE; // Disable USCI_A0 TX interrupt 
    P1OUT &= ~TXLED; } 
  
#pragma vector=USCIAB0RX_VECTOR 
__interrupt void USCI0RX_ISR(void) 
{ 
   P1OUT |= RXLED; 
    if (UCA0RXBUF == 'a') // 'a' received?
    { 
       i = 0; 
       UC0IE |= UCA0TXIE; // Enable USCI_A0 TX interrupt 
      UCA0TXBUF = string[i++]; 
    } 
    P1OUT &= ~RXLED;*
    */