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
CLI_FUNCTION(cli_targets, "targets", "lists supported targets");

static void _print_pin(uint16_t pin, bool nl) {
    if (pin == BOARD_PIN_UNDEF)
        printf("<N/A>")
    else
        printf("P%d.%02d", pin >> 5, pin & 0x1f);
    if (nl) printf("\r\n")
}

static void print_pin(uint16_t pin) {
    _print_pin(pin, false);
}

static void print_pin_nl(uint16_t pin) {
    _print_pin(pin, true);
}

static void target_info_bus(const char *prefix, const bus_t *b) {
    printf("%sbus.speed: %d Hz\r\n", prefix, b->bus_speed_hz);
    if (b->bus_type == BUS_TYPE_I2C) {
        printf("%sbus.type: i2c\r\n", prefix);
        printf("%sbus.i2c.id: 0x%02x\r\n", prefix, b->i2c.id);
        printf("%sbus.i2c.pin_scl: ", prefix); print_pin_nl(b->i2c.pin_scl);
        printf("%sbus.i2c.pin_sda: ", prefix); print_pin_nl(b->i2c.pin_sda);
    }
    else if (b->bus_type == BUS_TYPE_SPI) {
        printf("%sbus.type: spi\r\n", prefix);
        printf("%sbus.spi.mode: %d\r\n", prefix, b->spi.mode);
        printf("%sbus.spi.pin_clk: ", prefix); print_pin_nl(b->spi.pin_clk);
        printf("%sbus.spi.pin_mosi: ", prefix); print_pin_nl(b->spi.pin_mosi);
        printf("%sbus.spi.pin_miso: ", prefix); print_pin_nl(b->spi.pin_miso);
        printf("%sbus.spi.pin_cs: ", prefix); print_pin(b->spi.pin_cs);
        printf("   active: %s\r\n", b->spi.pin_cs_active_high ? "HI" : "LO");
    }
}

static void target_info_button(int id, const button_t *button) {
    if (button->routed) {
        printf("button.%d.pin: ", id);
        print_pin(button->pin);
        printf("   active: %s\r\n",button->pin_active_high ? "HI" : "LO");
    }
}

static void target_info_stepper(int id, const stepper_motor_t *stepper) {
    if (stepper->routed) {
        printf("stepper.%d.pin_a: ", id); print_pin_nl(stepper->pin_a);
        printf("stepper.%d.pin_b: ", id); print_pin_nl(stepper->pin_b);
        printf("stepper.%d.pin_c: ", id); print_pin_nl(stepper->pin_c);
        printf("stepper.%d.type: %d\r\n", id, stepper->stepper_type);
        printf("stepper.%d.steps_per_rev: %d\r\n", id, stepper->steps_per_rev);
        printf("stepper.%d.delay_us: %d\r\n", id, stepper->delay_us);
    }
}

static void target_info_battery(const target_t *t) {
    if (t->battery.routed) {
        printf("battery.pin_load: "); print_pin(t->battery.pin_load);
        printf("   active: %s\r\n",t->battery.pin_load_active_high ? "HI" : "LO");
        printf("battery.load_delay_ms: %d\r\n", t->battery.load_delay_ms);
        printf("battery.pin_measure: "); print_pin_nl(t->battery.pin_measure);
        printf("battery.pin_measure_enable: "); print_pin(t->battery.pin_measure_enable);
        printf("   active: %s\r\n",t->battery.pin_measure_enable_active_high ? "HI" : "LO");
    }
}

static void target_info_flip_flop_enabler(const target_t *t) {
    if (t->battery_flipflop_enabler.routed) {
        printf("battery_flipflop_enabler.pin_clk: "); print_pin_nl(t->battery_flipflop_enabler.clk);
        printf("battery_flipflop_enabler.pin_data: "); print_pin_nl(t->battery_flipflop_enabler.data);
    }
}

static void target_info_charger(const target_t *t) {
    if (t->charger.routed) {
    }
}

static void target_info_vibrator(const target_t *t) {
    if (t->vibrator.routed) {
        printf("vibrator.pin: "); print_pin(t->vibrator.pin);
        printf("   active: %s\r\n", t->vibrator.pin_active_high ? "HI" : "LO");
    }
}

static void target_info_accelerometer(const target_t *t) {
    if (t->accelerometer.routed) {
        printf("accelerometer.type: %d\r\n", t->accelerometer.type);
        printf("accelerometer.pin_vdd: "); print_pin(t->accelerometer.pin_vdd);
        printf("   active: %s\r\n", t->accelerometer.pin_vdd_active_high ? "HI" : "LO");
        printf("accelerometer.pin_int: "); print_pin_nl(t->accelerometer.pin_int);
        printf("accelerometer.pin_int_secondary: "); print_pin_nl(t->accelerometer.pin_int_secondary);
        target_info_bus("accelerometer.", &t->accelerometer.bus);
    }
}

static void target_info_ppg(const target_t *t) {
    if (t->ppg.routed) {
        printf("ppg.type: %d\r\n", t->ppg.type);
        printf("ppg.pin_vdd: "); print_pin(t->ppg.pin_vdd);
        printf("   active: %s\r\n", t->ppg.pin_vdd_active_high ? "HI" : "LO");
        printf("ppg.pin_reset: "); print_pin(t->ppg.pin_reset);
        printf("   active: %s\r\n", t->ppg.pin_reset_active_high ? "HI" : "LO");
        printf("ppg.pin_ifc_on: "); print_pin(t->ppg.pin_interface_on);
        printf("   active: %s\r\n", t->ppg.pin_interface_on_active_high ? "HI" : "LO");
        printf("ppg.pin_adc_rdy: "); print_pin_nl(t->ppg.pin_adc_ready);
        target_info_bus("ppg.", &t->ppg.bus);
    }
}

static void target_info_disp(const target_t *t)
{
    if (t->display.routed)
    {
        printf("display.type: %d\r\n", t->display.type);
        printf("display.pin_vdd: ");
        print_pin(t->display.pin_vdd);
        printf("   active: %s\r\n", t->display.pin_vdd_active_high ? "HI" : "LO");
        printf("display.pin_reset: ");
        print_pin(t->display.pin_reset);
        printf("   display: %s\r\n", t->display.pin_reset_active_high ? "HI" : "LO");
        printf("display.pin_dc: ");
        print_pin_nl(t->display.pin_dc);
        printf("display.pin_tearing: ");
        print_pin_nl(t->display.pin_tearing);
        target_info_bus("display.", &t->display.bus);
    }
}

static void target_info_pat9125(const target_t *t) {
    if (t->pat9125.routed) {
        printf("pat9125.pin_int: "); print_pin_nl(t->pat9125.pin_int);
        target_info_bus("pat9125.", &t->pat9125.bus);
    }
}

static int cli_target_info(int argc, const char **argv)
{
    const target_t *t = target_get();
    printf("name: %s\r\n", t->name);
    printf("id.proc: %x\r\n", t->id.ficr_proc);
    printf("id.uicr.raw:  %08x %04x\r\n", t->id.uicr.raw._80, t->id.uicr.raw._84);
    printf("id.uicr.mask: %08x %04x\r\n", t->id.uicr_mask.raw._80, t->id.uicr_mask.raw._84);

    printf("uart.pin_tx: "); print_pin_nl(t->uart.pin_tx);
    printf("uart.pin_rx: "); print_pin_nl(t->uart.pin_rx);
    for (int i = 0; i < MAX_BUTTON_IDS; i++) target_info_button(i, &t->buttons[i]);
    for (int i = 0; i < MAX_STEPPER_IDS; i++) target_info_stepper(i, &t->steppers[i]);
    target_info_battery(t);
    target_info_flip_flop_enabler(t);
    target_info_charger(t);
    target_info_vibrator(t);
    target_info_accelerometer(t);
    target_info_ppg(t);
    target_info_disp(t);
    target_info_pat9125(t);
    printf("pin_test_clk: "); print_pin_nl(t->pin_test_clk);
    printf("pins_pulled_up: ");
    for (int i = 0; i < t->pins_pulled_up_count; i++) {
        print_pin(t->pins_pulled_up[i]);
        printf(" ");
    }
    printf("\r\n");
    printf("pins_pulled_down: ");
    for (int i = 0; i < t->pins_pulled_down_count; i++) {
        print_pin(t->pins_pulled_down[i]);
        printf(" ");
    }
    printf("\r\n");

    return 0;
}
CLI_FUNCTION(cli_target_info, "target_info", "list current target info");

#endif
