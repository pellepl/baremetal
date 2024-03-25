#include "uicr.h"
#include "minio.h"

#define FLASH_UICR_ADDR_START (0x10001080)
#define FLASH_UICR_ADDR_END (0x10001210)

int uicr_get_active_entry_bit_ix(uint32_t word, uint8_t bitsize) {
    for (int i = 0; i < 32; i += bitsize+1) {
        if ((word & (1<<(i+bitsize))) != 0) {
            return i;
        }
    }
    return -1;
}

static uint32_t get_active_entry(uint32_t word, uint8_t bitsize) {
    int bit_ix = uicr_get_active_entry_bit_ix(word, bitsize);
    if (bit_ix < 0) {
        return (uint32_t)-1;
    }
    uint32_t mask = ((1 << bitsize) - 1) << bit_ix;
    return (word & mask) >> bit_ix;
}

void uicr_get_hw_identifier(target_hw_identifier_t *id)
{
    if (id == NULL)
        return;
    id->raw._80 = NRF_UICR->CUSTOMER[0];
    id->raw._84 = NRF_UICR->CUSTOMER[1];
}

volatile const uicr_pascal_t *uicr_get_pascal(void)
{
    return (volatile const uicr_pascal_t *)&NRF_UICR->CUSTOMER[0];
}

volatile const uicr_pluto_t *uicr_get_pluto(void)
{
    return (volatile const uicr_pluto_t *)&NRF_UICR->CUSTOMER[0];
}

void uicr_get_serial_str(char *buf, size_t buf_size)
{
    const target_t *t = target_get();
    if (strcmp(TARGET_PASCAL, t->name) == 0) {
        volatile const uicr_pascal_t *uicr = uicr_get_pascal();
        snprintf(buf, buf_size, "%02d%02d%02X%02X%02X", uicr->serial[0] % 100, uicr->serial[1] % 100,
                        uicr->serial[2], uicr->serial[3], uicr->serial[4]);
    } else {
        // defaults to pluto
        volatile const uicr_pluto_t *uicr = uicr_get_pluto();
        snprintf(buf, buf_size, "%02d%02d%02X%02X%02X", uicr->serial[0] % 100, uicr->serial[1] % 100,
                        uicr->serial[2], uicr->serial[3], uicr->serial[4]);

    } 
}

void  uicr_get_item_id_str(char *buf, size_t buf_size)
{
    const target_t *t = target_get();
    if (strcmp(TARGET_PASCAL, t->name) == 0) {
        volatile const uicr_pascal_t *uicr = (volatile const uicr_pascal_t *)&NRF_UICR->CUSTOMER[0];
        volatile const uint8_t *itemid = (volatile const uint8_t *)&uicr->itemid;
        snprintf(buf, buf_size, "A%02u%02u-%02u%02u", itemid[0] % 100, itemid[1] % 100,
                 itemid[2] % 100, itemid[3] % 100);
    } else {
        // defaults to pluto
        volatile const uicr_pluto_t *uicr = (volatile const uicr_pluto_t *)&NRF_UICR->CUSTOMER[0];
	    snprintf(buf, buf_size, "A%02u%02u-%02u%02u", uicr->itemid[0] % 100, uicr->itemid[1] % 100,
		    			uicr->itemid[2] % 100, uicr->itemid[3] % 100);
    }
}

bool uicr_pba_motor_rev_is_set(volatile const uicr_pascal_t *u)
{
    return (u->pba_motor_rev) != 0xffff;
}

bool uicr_pba_hr_rev_is_set(volatile const uicr_pascal_t *u)
{
    return (u->pba_hr_rev) != 0xffff;
}

bool uicr_pba_display_rev_is_set(volatile const uicr_pascal_t *u)
{
    return (u->pba_display_rev) != 0xffff;
}

int uicr_write_word(uint32_t addr, uint32_t word)  {
    if (addr < FLASH_UICR_ADDR_START || addr >= FLASH_UICR_ADDR_END-4) {
        return -EINVAL;
    }
    NRF_NVMC->CONFIG = 1;
    __ISB();
    __DSB();
    uint32_t *dst = (uint32_t *)addr;
    *dst = word;
    while (NRF_NVMC->READY == 0);
    NRF_NVMC->CONFIG = 0;
    __ISB();
    __DSB();
    return 0;
}

static uint32_t pluto_bitsize_for_marker(volatile const uint32_t *w) {
    const target_t *t = target_get();
    uint32_t bitsize = 0;
    if (w == &uicr_get_pluto()->prodtest) {
        bitsize = t->uicr_info.pronto.marker_prodtest_bitsize;
    } else if (w == &uicr_get_pluto()->coretest) {
        bitsize = t->uicr_info.pronto.marker_coretest_bitsize;
    } else if (w == &uicr_get_pluto()->movementtest) {
        bitsize = t->uicr_info.pronto.marker_movementtest_bitsize;
    } 
    return bitsize;
}

static uint32_t pluto_get_marker(volatile const uint32_t *w) {
    return get_active_entry(*w, pluto_bitsize_for_marker(w));
}

static int pluto_set_marker(int ix, volatile const uint32_t *w) {
    int entry_ix = uicr_get_active_entry_bit_ix(*w, pluto_bitsize_for_marker(w));
    if (entry_ix < 0) {
        return -ENOMEM;
    }
    return uicr_write_word((uint32_t)w, *w & ~(1<<(entry_ix + ix)));

}

static int pluto_clear_marker(volatile const uint32_t *w) {
    uint32_t bitsize = pluto_bitsize_for_marker(w);
    int entry_ix = uicr_get_active_entry_bit_ix(*w, bitsize);
    if (entry_ix < 0 || entry_ix > 32 - ((int)bitsize + 1) * 2)  {
        return -ENOMEM;
    }
    return uicr_write_word((uint32_t)w, *w & ~(1<<(entry_ix + bitsize)));
}

uint32_t uicr_get_prodtest_marker(void) {
    const target_t *t = target_get();
    if (strcmp(TARGET_PASCAL, t->name) == 0)
    {
        return uicr_get_pascal()->prodtest[0]; // TODO PETER
    }
    // defaults to pluto
    return pluto_get_marker(&uicr_get_pluto()->prodtest);
}

int uicr_set_prodtest_marker(int ix) {
    const target_t *t = target_get();
    if (ix < 0 || ix >= t->uicr_info.prodtest_flags) {
        return -EINVAL;
    }
    if (strcmp(TARGET_PASCAL, t->name) == 0) {
        return -ENOSYS; // TODO PETER
    }

    // defaults to pluto
    return pluto_set_marker(ix, &uicr_get_pluto()->prodtest);
}

int uicr_clear_prodtest_marker(void) {
    const target_t *t = target_get();
    if (strcmp(TARGET_PASCAL, t->name) == 0) {
        return -ENOSYS; // TODO PETER
    }
    // defaults to pluto
    return pluto_clear_marker(&uicr_get_pluto()->prodtest);
}

uint32_t uicr_get_coretest_marker(void) {
    const target_t *t = target_get();
    if (strcmp(TARGET_PASCAL, t->name) == 0)
    {
        return uicr_get_pascal()->coretest[0].flag; // TODO PETER
    }
    // defaults to pluto
    return pluto_get_marker(&uicr_get_pluto()->coretest);
}

int uicr_set_coretest_marker(int ix) {
    const target_t *t = target_get();
    if (strcmp(TARGET_PASCAL, t->name) == 0) {
        return -ENOSYS; // TODO PETER
    }
    // defaults to pluto
    return pluto_set_marker(ix, &uicr_get_pluto()->coretest);
}

int uicr_clear_coretest_marker(void) {
    const target_t *t = target_get();
    if (strcmp(TARGET_PASCAL, t->name) == 0) {
        return -ENOSYS; // TODO PETER
    }
    // defaults to pluto
    return pluto_clear_marker(&uicr_get_pluto()->coretest);
}

uint32_t uicr_get_mvttest_marker(void) {
    const target_t *t = target_get();
    if (strcmp(TARGET_PASCAL, t->name) == 0)
    {
        return uicr_get_pascal()->movementtest; // TODO PETER
    }
    // defaults to pluto
    return pluto_get_marker(&uicr_get_pluto()->movementtest);
}

int uicr_set_mvttest_marker(int ix) {
    const target_t *t = target_get();
    if (strcmp(TARGET_PASCAL, t->name) == 0) {
        return -ENOSYS; // TODO PETER
    }
    // defaults to pluto
    return pluto_set_marker(ix, &uicr_get_pluto()->movementtest);
}

int uicr_clear_mvttest_marker(void) {
    const target_t *t = target_get();
    if (strcmp(TARGET_PASCAL, t->name) == 0) {
        return -ENOSYS; // TODO PETER
    }
    // defaults to pluto
    return pluto_clear_marker(&uicr_get_pluto()->movementtest);
}

#if NO_EXTRA_CLI == 0

#include "cli.h"

static uint8_t uicr_local_copy[FLASH_UICR_ADDR_END - FLASH_UICR_ADDR_START];

static int cli_dump(int argc, const char **argv)
{
    int n = 0;
    const uint8_t *addr = (const uint8_t *)FLASH_UICR_ADDR_START;
    while (addr < (const uint8_t *)FLASH_UICR_ADDR_END)
    {
        if ((n++ & 0xf) == 0)
        {
            printf("\r\n0x%08x: ", addr);
        }
        printf("%02x ", *addr++)
    }
    printf("\r\n");
    return 0;
}
CLI_FUNCTION(cli_dump, "uicr_dump", "dumps raw UICR");

static int cli_erase(int argc, const char **argv)
{
    if (argc != 1 || strcmp("iamsure", argv[0]) != 0)
        return ERR_CLI_EINVAL;
    // erase
    NRF_NVMC->CONFIG = 2;
    NRF_NVMC->ERASEUICR = 1;
    while (NRF_NVMC->READY == 0)
        ;
    NRF_NVMC->CONFIG = 0;
    return 0;
}
CLI_FUNCTION(cli_erase, "uicr_erase", "erase UICR: iamsure");

static int cli_write(int argc, const char **argv)
{
    if (argc < 2)
        return ERR_CLI_EINVAL;
    uint32_t addr = strtol(argv[0], NULL, 0);
    if (addr < FLASH_UICR_ADDR_START || addr + argc - 1 >= FLASH_UICR_ADDR_END)
        return ERR_CLI_EINVAL;
    // copy
    memcpy(uicr_local_copy, (const uint8_t *)FLASH_UICR_ADDR_START, sizeof(uicr_local_copy));

    // erase
    NRF_NVMC->CONFIG = 2;
    __ISB();
    __DSB();
    NRF_NVMC->ERASEUICR = 1;
    while (NRF_NVMC->READY == 0)
        ;
    NRF_NVMC->CONFIG = 0;
    __ISB();
    __DSB();

    for (int i = 1; i < argc; i++)
    {
        uicr_local_copy[addr - FLASH_UICR_ADDR_START + (i - 1)] = strtol(argv[i], NULL, 0);
    }

    // write back
    NRF_NVMC->CONFIG = 1;
    __ISB();
    __DSB();

    uint32_t *dst = (uint32_t *)FLASH_UICR_ADDR_START;
    const uint8_t *data = uicr_local_copy;
    while (dst < (uint32_t *)FLASH_UICR_ADDR_END)
    {
        uint32_t word =
            (data[0] << 0) |
            (data[1] << 8) |
            (data[2] << 16) |
            (data[3] << 24);
        *((volatile uint32_t *)dst) = word;
        while (NRF_NVMC->READY == 0)
            ;
        dst++;
        data += 4;
    }
    NRF_NVMC->CONFIG = 0;
    __ISB();
    __DSB();

    return 0;
}
CLI_FUNCTION(cli_write, "uicr_write", "writes raw UICR (no erase needed): <addr> <byte>*");

#endif