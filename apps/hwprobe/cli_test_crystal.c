/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#if CONFIG_CRYSTAL_TEST == 1

#include "cli.h"
#include "minio.h"
#if FAMILY == nrf52 || FAMILY == stm32f1 || FAMILY == stm32l0
#include "crystal_test.h"

static int cli_xtal_test(int argc, const char **argv) {
    if (argc != 1) {
        printf("[rc|xtal|synth]\n");
        return -1;
    }
    switch (argv[0][0]) {
        case 'r':
            crystal_test(LFCLK_OSC_RC);
            break;
        case 'x':
            crystal_test(LFCLK_OSC_XTAL);
            break;
        case 's':
            crystal_test(LFCLK_OSC_SYNTH);
            break;
        default:
            printf("unknown arg\n");
            return -1;
    }
    return 0;
}
CLI_FUNCTION(cli_xtal_test, "xtal_test", "");
#endif

#endif
