#include "targets.h"
#include "minio.h"
#include "gpio_driver.h"

extern void *__target_entries_start, *__target_entries_end;

static const target_t *current_target = NULL;

const target_t *target_set_by_uicr(void)
{
    const target_t *def = NULL;
    const target_t *targets = target_get_all();
    uint32_t ficr_proc = NRF_FICR->INFO.PART;
    uint32_t uicr_80 = NRF_UICR->CUSTOMER[0];
    uint32_t uicr_84 = NRF_UICR->CUSTOMER[1];
    for (int i = 0; i < target_count(); i++)
    {
        const target_t *t = targets;
        if (strcmp("PLUTO", t->name) == 0) def = t;
        targets++;
        if (ficr_proc != t->id.ficr_proc)
            continue;
        if ((t->id.uicr.raw._80 & t->id.uicr_mask.raw._80) != (uicr_80 & t->id.uicr_mask.raw._80))
            continue;
        if ((t->id.uicr.raw._84 & t->id.uicr_mask.raw._84) != (uicr_84 & t->id.uicr_mask.raw._84))
            continue;
        current_target = t;
        break;
    }

    if (current_target == NULL) current_target = def;

    // set default pulls
    for (int i = 0; current_target->pins_pulled_up != NULL && i < current_target->pins_pulled_up_count; i++)
    {
        gpio_disconnect(current_target->pins_pulled_up[i], GPIO_PULL_UP);
    }
    for (int i = 0; current_target->pins_pulled_down != NULL && i < current_target->pins_pulled_down_count; i++)
    {
        gpio_disconnect(current_target->pins_pulled_down[i], GPIO_PULL_DOWN);
    }

    return current_target;
}

int target_count(void)
{
    return ((intptr_t)&__target_entries_end - (intptr_t)&__target_entries_start) / sizeof(target_t);
}

const target_t *target_get_all(void)
{
    return (const target_t *)&__target_entries_start;
}

const target_t *target_get(void)
{
    return current_target;
}

void target_reset_pin(uint16_t pin)
{
    gpio_disconnect(pin, target_default_pull_for_pin(pin));
}

gpio_pull_t target_default_pull_for_pin(uint16_t pin)
{
    gpio_pull_t pull = GPIO_PULL_NONE;
    for (int i = 0; current_target->pins_pulled_up != NULL && i < current_target->pins_pulled_up_count; i++)
    {
        if (current_target->pins_pulled_up[i] == pin)
        {
            pull = GPIO_PULL_UP;
        }
    }
    for (int i = 0; current_target->pins_pulled_down != NULL && i < current_target->pins_pulled_down_count; i++)
    {
        if (current_target->pins_pulled_down[i] == pin)
        {
            pull = GPIO_PULL_DOWN;
        }
    }
    return pull;
}

#if NO_EXTRA_CLI == 0

#include "cli.h"
#include "minio.h"

static void mask_hex_print(uint32_t val, uint32_t mask, int bits) {
    char masked_hex[10];
    char *buf = masked_hex;
    while (bits > 0) {
        bits -= 4;
        uint32_t nibble_mask = 0xf<<bits;
        if ((mask & nibble_mask) == nibble_mask) {
            *buf++ = "0123456789ABCDEF"[(val & nibble_mask) >> bits];
        } else if ((mask & nibble_mask) == 0) {
            *buf++ = 'x';
        } else {
            *buf++ = '?';
        }
        if (buf >= &masked_hex[sizeof(masked_hex)-2]) break;
    }
    *buf++ = ' ';
    *buf = 0;
    printf("%s", masked_hex);
}

static int cli_targets(int argc, const char **argv)
{
    const target_t *targets = target_get_all();
    for (int i = 0; i < target_count(); i++)
    {
        const target_t *t = targets;
        targets++;
        
        printf("%s\tnrf%x\tUICR:", t->name, t->id.ficr_proc);
        mask_hex_print(t->id.uicr.raw._80, t->id.uicr_mask.raw._80, 32);
        mask_hex_print(t->id.uicr.pronto.rev, t->id.uicr_mask.pronto.rev, 8);
        mask_hex_print(t->id.uicr.pronto.subrev, t->id.uicr_mask.pronto.subrev, 8);
        printf("\r\n");
    }

    printf("current target:%s\r\n", current_target ? current_target->name : "NONE");

    return 0;
}
CLI_FUNCTION(cli_targets, "targets", "lists supported targets")

#endif