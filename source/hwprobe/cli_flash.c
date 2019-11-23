#if 1//CONFIG_FLASH==1

#include "flash_driver.h"
#include "cli.h"
#include "minio.h"
#include "cpu.h"

static int cli_flash_list(int argc, const char **argv) {
    uint32_t sector;
    uint32_t num_sectors;
    for (flash_type_t t = 0; t < _FLASH_TYPE_CNT; t++) {
        int res = flash_get_sectors_for_type(t, &sector, &num_sectors);
        if (res) continue;
        uint32_t size = 0;
        for (uint32_t i = 0; i < num_sectors; i++) {
            res = flash_get_sector_size(sector);
            if (res > 0) size += res;
        }
        printf("%d\tstart sector:%08x\t%d sectors\t%d bytes total\n", t, sector, num_sectors, size);
    }
    return 0;
}
CLI_FUNCTION(cli_flash_list, "flash_list");

static int cli_flash_read(int argc, const char **argv) {
    if (argc != 3) {
        printf("[<sector>,<offset>,<length>]\n");
        return ERR_CLI_EINVAL;
    }
    uint32_t sector = atoi(argv[0]);
    uint32_t offset = atoi(argv[1]);
    uint32_t length = atoi(argv[2]);

    uint8_t buf[length];

    int res = flash_read(sector, offset, buf, length);

    if (res >= 0) {
        fprint_mem(UART_STD, buf, res);
        printf("read %d bytes\n", res);
        res = 0;
    }
    return res;
}
CLI_FUNCTION(cli_flash_read, "flash_read");

static int cli_flash_write(int argc, const char **argv) {
    if (argc < 3) {
        printf("[<sector>,<offset>,...]\n");
        return ERR_CLI_EINVAL;
    }
    uint32_t sector = atoi(argv[0]);
    uint32_t offset = atoi(argv[1]);
    uint32_t length = argc-2;

    uint8_t buf[length];

    for (int i = 2; i < argc; i++) {
        buf[i-2] = strtol(argv[i], 0, 16);
    }

    int res = flash_write(sector, offset, buf, length);

    if (res >= 0) {
        printf("wrote %d bytes\n", res);
        res = 0;
    }

    return res;
}
CLI_FUNCTION(cli_flash_write, "flash_write");

static int cli_flash_erase(int argc, const char **argv) {
    if (argc != 1) {
        printf("[<sector>]\n");
        return ERR_CLI_EINVAL;
    }
    uint32_t sector = atoi(argv[0]);

    int res = flash_erase(sector);

    return res;
}
CLI_FUNCTION(cli_flash_erase, "flash_erase");

#endif