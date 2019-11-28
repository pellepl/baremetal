#include "msp430.h"
#include "board_common.h"

extern uint32_t _clockspeed;

void board_init() {
    DCOCTL = 0; // Select lowest DCOx and MODx settings
    BCSCTL1 = CALBC1_1MHZ; // Set DCO
    DCOCTL = CALDCO_1MHZ;
    _clockspeed = 1000000;
}
