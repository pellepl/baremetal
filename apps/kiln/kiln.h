#ifndef _KILN_H_
#define _KILN_H_

#include "controller.h"
#include "timer.h"

#define PORTA(x)                (0*16+x)
#define PORTB(x)                (1*16+x)
#define PORTC(x)                (2*16+x)
#define PORTD(x)                (3*16+x)
#define PORTE(x)                (4*16+x)

//#define MAX6675
#define MAX31855

#define UART_BAUDRATE           UART_BAUDRATE_921600

#define THERMOCOUPLE_SAMPLE_PERIOD_MS           100
#define THERMOCOUPLE_MAX_NBR_CONSEQ_ERRS        10

#define PIN_ELEMENT_1           PORTB(12)
#define PIN_ELEMENT_2           PORTB(13)
#define PIN_ELEMENT_3           PORTB(14)
#define PIN_ELEMENT_4           PORTB(15)

#define PIN_THERMOC_CLK         PORTE(6)
#define PIN_THERMOC_CS          PORTA(1)
#define PIN_THERMOC_MISO        PORTC(3)

#define PIN_TOUCH_CLK           PORTA(5)
#define PIN_TOUCH_MISO          PORTA(6)
#define PIN_TOUCH_MOSI          PORTA(7)
#define PIN_TOUCH_CS            PORTB(7)
#define PIN_TOUCH_IRQ           PORTB(6)

#define PIN_UART_TX             PORTA(2)
#define PIN_UART_RX             PORTA(3)
#define PIN_UART_TX_IPC         PORTA(9)
#define PIN_UART_RX_IPC         PORTA(10)

#define PIN_SPEAKER_PWM         PORTB(0) // TIM3 CH3

typedef enum {
    ATT_MAX_TEMP,
    ATT_THERMO_ERR,
    ATT_MAX_POW_TIME,
    ATT_MAX_ON_TIME,
    ATT_PANIC
} kiln_attention_t;

void gui_init(void);
void gui_loop(void);
void gui_status_report_ctrl(control_state_t ctrl, int temp, int target_temp, int power);
void gui_silence(void);
void gui_beep(uint16_t hz, tick_t time);
void gui_melody(const uint16_t *hz, const uint16_t *time, uint8_t len);
void gui_set_attention(kiln_attention_t att);


uint32_t __get_used_stack(void);

#endif
