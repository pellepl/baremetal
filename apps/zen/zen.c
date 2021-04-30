/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "board.h"
int main(void) {
    cpu_init();
    board_init();
    while (1);
}
