/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#if 1 // CONFIG_NVMTNV==1

#include "nvmtnv.h"
#include "cli.h"
#include "minio.h"

static int cli_format(int argc, const char **argv) {
    if (argc != 3) {
        printf("<start_sector> <sectors> <val_size>\n");
        return -1;
    }
    return nvmtnv_format(atoi(argv[0]), atoi(argv[1]), atoi(argv[2]));
}
CLI_FUNCTION(cli_format, "nvm_format");

static int cli_mount(int argc, const char **argv) {
    if (argc != 1) {
        printf("<start_sector>\n");
        return -1;
    }
    return nvmtnv_mount(atoi(argv[0]));
}
CLI_FUNCTION(cli_mount, "nvm_mount");

static int cli_write(int argc, const char **argv) {
    if (argc != 2) {
        printf("<tag> <string>\n");
        return -1;
    }
    return nvmtnv_write(atoi(argv[0]), (const uint8_t *)argv[1], strlen(argv[1]));
}
CLI_FUNCTION(cli_write, "nvm_write");

static int cli_read(int argc, const char **argv) {
    if (argc != 1) {
        printf("<tag>\n");
        return -1;
    }
    uint8_t buf[255];
    int res = nvmtnv_read(atoi(argv[0]), buf, sizeof(buf));
    if (res < 0) return res;
    fprint_mem(UART_STD, buf, res);
    return 0;
}
CLI_FUNCTION(cli_read, "nvm_read");

static int cli_delete(int argc, const char **argv) {
    if (argc != 1) {
        printf("<tag>\n");
        return -1;
    }
    return nvmtnv_delete(atoi(argv[0]));
}
CLI_FUNCTION(cli_delete, "nvm_delete");

#endif