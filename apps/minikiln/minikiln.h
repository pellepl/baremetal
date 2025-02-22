#pragma once
#include "timer.h"

#define PORTA(x) (0 * 16 + x)
#define PORTB(x) (1 * 16 + x)
#define PORTC(x) (2 * 16 + x)
#define PORTD(x) (3 * 16 + x)
#define PORTE(x) (4 * 16 + x)

// #define MAX6675
#define MAX31855

#define UART_BAUDRATE UART_BAUDRATE_921600

#define THERMOCOUPLE_SAMPLE_PERIOD_MS 100
#define THERMOCOUPLE_MAX_NBR_CONSEQ_ERRS 10

#define PIN_ELEMENT_1 PORTB(12)
#define PIN_ELEMENT_2 PORTB(13)
#define PIN_ELEMENT_3 PORTB(14)
#define PIN_ELEMENT_4 PORTB(15)

#define PIN_THERMOC_CLK PORTE(6)
#define PIN_THERMOC_CS PORTA(1)
#define PIN_THERMOC_MISO PORTC(3)


#define PIN_UART_TX PORTA(2)
#define PIN_UART_RX PORTA(3)

typedef enum
{
    ATT_MAX_TEMP,
    ATT_THERMO_ERR,
    ATT_MAX_POW_TIME,
    ATT_MAX_ON_TIME,
    ATT_PANIC
} kiln_attention_t;

uint32_t __get_used_stack(void);

