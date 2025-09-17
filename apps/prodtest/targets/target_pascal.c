#include "targets.h"
#include "gpio_driver.h"
#include "cpu.h"

/////////////////////////////////////////
// Pascal, FKS934, full display, Rango //
/////////////////////////////////////////

static void pascal_target_init(const target_t *t)
{
    // connect battery by flipping flip flop
    gpio_config(t->battery_flipflop_enabler.clk, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
    gpio_config(t->battery_flipflop_enabler.data, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
    gpio_set(t->battery_flipflop_enabler.data, 1);
    for (int i = 0; i < 3; i++)
    {
        cpu_halt(1);
        gpio_set(t->battery_flipflop_enabler.clk, 1);
        cpu_halt(1);
        gpio_set(t->battery_flipflop_enabler.clk, 0);
    }
    gpio_config(t->battery_flipflop_enabler.clk, GPIO_DIRECTION_INPUT, GPIO_PULL_NONE);
    gpio_config(t->battery_flipflop_enabler.data, GPIO_DIRECTION_INPUT, GPIO_PULL_NONE);
}

TARGET_SPEC pascal = {
    .name = TARGET_PASCAL,

    .id.ficr_proc = FICR_PROC_NRF52840,
    .id.uicr.raw._80 = 0x2f013706,
    .id.uicr_mask.raw._80 = 0xffffffff,
    .id.uicr_mask.raw._84 = 0x0000,

    .uart.pin_rx = P(0, 29),
    .uart.pin_tx = P(1, 15),

    .buttons[BUTTON_ID_PUSHER_UP].routed = true,
    .buttons[BUTTON_ID_PUSHER_UP].pin = P(1, 12),
    .buttons[BUTTON_ID_PUSHER_UP].pin_active_high = false,

    .buttons[BUTTON_ID_PUSHER_MIDDLE].routed = true,
    .buttons[BUTTON_ID_PUSHER_MIDDLE].pin = P(0, 28),
    .buttons[BUTTON_ID_PUSHER_MIDDLE].pin_active_high = false,

    .buttons[BUTTON_ID_PUSHER_DOWN].routed = true,
    .buttons[BUTTON_ID_PUSHER_DOWN].pin = P(0, 2),
    .buttons[BUTTON_ID_PUSHER_DOWN].pin_active_high = false,

    .buttons[BUTTON_ID_PRODTEST].routed = true,
    .buttons[BUTTON_ID_PRODTEST].pin = P(0, 26),
    .buttons[BUTTON_ID_PRODTEST].pin_active_high = true,

    .buttons[BUTTON_ID_BTTEST].routed = true,
    .buttons[BUTTON_ID_BTTEST].pin = P(0, 11),
    .buttons[BUTTON_ID_BTTEST].pin_active_high = true,

    .buttons[BUTTON_ID_MVTTEST].routed = true,
    .buttons[BUTTON_ID_MVTTEST].pin = P(1, 0),
    .buttons[BUTTON_ID_MVTTEST].pin_active_high = true,

    .steppers[STEPPER_ID_HOUR].stepper_type = STEPPER_TYPE_SOPROD_2PHASE,
    .steppers[STEPPER_ID_HOUR].routed = true,
    .steppers[STEPPER_ID_HOUR].steps_per_rev = 120,
    .steppers[STEPPER_ID_HOUR].delay_us = 15000,
    .steppers[STEPPER_ID_HOUR].pin_a = P(0, 11),
    .steppers[STEPPER_ID_HOUR].pin_b = P(1, 0),
    .steppers[STEPPER_ID_HOUR].pin_c = P(0, 5),

    .steppers[STEPPER_ID_MINUTE].stepper_type = STEPPER_TYPE_SOPROD_2PHASE,
    .steppers[STEPPER_ID_MINUTE].routed = true,
    .steppers[STEPPER_ID_MINUTE].steps_per_rev = 120,
    .steppers[STEPPER_ID_MINUTE].delay_us = 15000,
    .steppers[STEPPER_ID_MINUTE].pin_a = P(1, 9),
    .steppers[STEPPER_ID_MINUTE].pin_b = P(0, 26),
    .steppers[STEPPER_ID_MINUTE].pin_c = P(0, 12),

    .battery_flipflop_enabler.routed = true,
    .battery_flipflop_enabler.clk = P(1, 7),
    .battery_flipflop_enabler.data = P(1, 5),

    .pin_test_clk = P(1, 9),

    .accelerometer.routed = true,
    .accelerometer.type = ACC_TYPE_BMA400,
    .accelerometer.pin_vdd = P(0, 16),
    .accelerometer.pin_vdd_active_high = true,
    .accelerometer.pin_int = P(1, 6),
    .accelerometer.pin_int_secondary = BOARD_PIN_UNDEF,
    .accelerometer.bus.bus_type = BUS_TYPE_SPI,
    .accelerometer.bus.bus_speed_hz = 8000000,
    .accelerometer.bus.spi.mode = 0,
    .accelerometer.bus.spi.pin_clk = P(0, 14),
    .accelerometer.bus.spi.pin_miso = P(0, 15),
    .accelerometer.bus.spi.pin_mosi = P(0, 17),
    .accelerometer.bus.spi.pin_cs = P(1, 2),
    .accelerometer.bus.spi.pin_cs_active_high = false,

    .ppg.routed = true,
    .ppg.type = PPG_TYPE_AFE4410,
    .ppg.pin_vdd = P(1, 2),
    .ppg.pin_vdd_active_high = true,
    .ppg.pin_interface_on = P(1, 4),
    .ppg.pin_interface_on_active_high = true,
    .ppg.pin_reset = P(0, 30),
    .ppg.pin_reset_active_high = false,
    .ppg.pin_adc_ready = P(0, 13),
    .ppg.bus.bus_type = BUS_TYPE_SPI,
    .ppg.bus.bus_speed_hz = 1000000,
    .ppg.bus.spi.mode = 0,
    .ppg.bus.spi.pin_clk = P(0, 14),
    .ppg.bus.spi.pin_miso = P(0, 15),
    .ppg.bus.spi.pin_mosi = P(0, 17),
    .ppg.bus.spi.pin_cs = P(1, 8),
    .ppg.bus.spi.pin_cs_active_high = false,

    .vibrator.routed = true,
    .vibrator.pin = P(0, 8),
    .vibrator.pin_active_high = true,

    .target_init = pascal_target_init,
};
