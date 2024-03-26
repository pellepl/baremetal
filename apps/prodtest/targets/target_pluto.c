#include "targets.h"
#include "uicr.h"
#include "gpio_driver.h"

static bool pluto_stepper_mounted(stepper_id_t id) {
    target_hw_identifier_t hwid;
    uicr_get_hw_identifier(&hwid);
    switch (hwid.pronto.subrev) {
        default:
        case 1:
            switch (id) {
                case STEPPER_ID_HOUR: return true;
                case STEPPER_ID_MINUTE: return true;
                default: return false;
            }
        break;
        case 2:
            switch (id) {
                case STEPPER_ID_HOUR: return true;
                case STEPPER_ID_MINUTE: return true;
                case STEPPER_ID_SUB2: return true;
                default: return false;
            }
        break;
        case 3:
            switch (id) {
                case STEPPER_ID_HOUR: return true;
                case STEPPER_ID_MINUTE: return true;
                case STEPPER_ID_SUB0: return true;
                case STEPPER_ID_SUB1: return true;
                default: return false;
            }
        break;
    }
}

TARGET_SPEC pluto =

    /////////////////////////////////////////////////
    // BT07, formerly known as BT004, a.k.a. Pluto //
    /////////////////////////////////////////////////
    {
        .name = TARGET_PLUTO,

        .id.ficr_proc = FICR_PROC_NRF52832,
        .id.uicr.pronto.pba[0] = 0x14,
        .id.uicr.pronto.pba[1] = 0x00,
        .id.uicr.pronto.pba[2] = 0x0b,
        .id.uicr.pronto.pba[3] = 0x03,
        .id.uicr.pronto.rev = 0x03,
        .id.uicr_mask.raw._80 = 0xffffffff,
        .id.uicr_mask.raw._84 = 0x00ff,

        .uart.pin_rx = P(0, 6),
        .uart.pin_tx = P(0, 8),

        .buttons[BUTTON_ID_PUSHER_UP].routed = true,
        .buttons[BUTTON_ID_PUSHER_UP].pin = P(0, 19),
        .buttons[BUTTON_ID_PUSHER_UP].pin_active_high = false,

        .buttons[BUTTON_ID_PUSHER_MIDDLE].routed = true,
        .buttons[BUTTON_ID_PUSHER_MIDDLE].pin = P(0, 11),
        .buttons[BUTTON_ID_PUSHER_MIDDLE].pin_active_high = false,

        .buttons[BUTTON_ID_PUSHER_DOWN].routed = true,
        .buttons[BUTTON_ID_PUSHER_DOWN].pin = P(0, 25),
        .buttons[BUTTON_ID_PUSHER_DOWN].pin_active_high = false,

        .buttons[BUTTON_ID_PRODTEST].routed = true,
        .buttons[BUTTON_ID_PRODTEST].pin = P(0, 4),
        .buttons[BUTTON_ID_PRODTEST].pin_active_high = false,

        .buttons[BUTTON_ID_BTTEST].routed = true,
        .buttons[BUTTON_ID_BTTEST].pin = P(0, 28),
        .buttons[BUTTON_ID_BTTEST].pin_active_high = false,

        .buttons[BUTTON_ID_MVTTEST].routed = true,
        .buttons[BUTTON_ID_MVTTEST].pin = P(0, 2),
        .buttons[BUTTON_ID_MVTTEST].pin_active_high = false,

        .battery.routed = true,
        .battery.pin_load = P(0, 20),
        .battery.pin_load_active_high = true,
        .battery.pin_measure_enable = BOARD_PIN_UNDEF,
        .battery.pin_measure = BOARD_PIN_UNDEF,

        .accelerometer.routed = true,
        .accelerometer.type = ACC_TYPE_BMA400,
        .accelerometer.pin_vdd = P(0, 24),
        .accelerometer.pin_vdd_active_high = true,
        .accelerometer.pin_int = P(0, 10),
        .accelerometer.pin_int_secondary = BOARD_PIN_UNDEF,
        .accelerometer.bus.bus_type = BUS_TYPE_SPI,
        .accelerometer.bus.bus_speed_hz = 8000000,
        .accelerometer.bus.spi.mode = 0,
        .accelerometer.bus.spi.pin_clk = P(0, 23),
        .accelerometer.bus.spi.pin_miso = P(0, 13),
        .accelerometer.bus.spi.pin_mosi = P(0, 17),
        .accelerometer.bus.spi.pin_cs = P(0, 22),
        .accelerometer.bus.spi.pin_cs_active_high = false,

        .vibrator.routed = true,
        .vibrator.pin = P(0, 26),
        .vibrator.pin_active_high = true,

        .steppers[STEPPER_ID_HOUR].stepper_type = STEPPER_TYPE_SOPROD_2PHASE,
        .steppers[STEPPER_ID_HOUR].routed = true,
        .steppers[STEPPER_ID_HOUR].steps_per_rev = 120,
        .steppers[STEPPER_ID_HOUR].delay_us = 15000,
        .steppers[STEPPER_ID_HOUR].pin_a = P(0, 31),
        .steppers[STEPPER_ID_HOUR].pin_b = P(0, 2),
        .steppers[STEPPER_ID_HOUR].pin_c = P(0, 27),

        .steppers[STEPPER_ID_MINUTE].stepper_type = STEPPER_TYPE_SOPROD_2PHASE,
        .steppers[STEPPER_ID_MINUTE].routed = true,
        .steppers[STEPPER_ID_MINUTE].steps_per_rev = 120,
        .steppers[STEPPER_ID_MINUTE].delay_us = 15000,
        .steppers[STEPPER_ID_MINUTE].pin_a = P(0, 30),
        .steppers[STEPPER_ID_MINUTE].pin_b = P(0, 28),
        .steppers[STEPPER_ID_MINUTE].pin_c = P(0, 29),

        .steppers[STEPPER_ID_SUB0].stepper_type = STEPPER_TYPE_SOPROD_2PHASE,
        .steppers[STEPPER_ID_SUB0].routed = true,
        .steppers[STEPPER_ID_SUB0].steps_per_rev = 180,
        .steppers[STEPPER_ID_SUB0].delay_us = 15000,
        .steppers[STEPPER_ID_SUB0].pin_a = P(0, 4),
        .steppers[STEPPER_ID_SUB0].pin_b = P(0, 7),
        .steppers[STEPPER_ID_SUB0].pin_c = P(0, 3),

        .steppers[STEPPER_ID_SUB1].stepper_type = STEPPER_TYPE_SOPROD_2PHASE,
        .steppers[STEPPER_ID_SUB1].routed = true,
        .steppers[STEPPER_ID_SUB1].steps_per_rev = 180,
        .steppers[STEPPER_ID_SUB1].delay_us = 15000,
        .steppers[STEPPER_ID_SUB1].pin_a = P(0, 16),
        .steppers[STEPPER_ID_SUB1].pin_b = P(0, 14),
        .steppers[STEPPER_ID_SUB1].pin_c = P(0, 15),

        .steppers[STEPPER_ID_SUB2].stepper_type = STEPPER_TYPE_SOPROD_2PHASE,
        .steppers[STEPPER_ID_SUB2].routed = true,
        .steppers[STEPPER_ID_SUB2].steps_per_rev = 180,
        .steppers[STEPPER_ID_SUB2].delay_us = 15000,
        .steppers[STEPPER_ID_SUB2].pin_a = P(0, 5),
        .steppers[STEPPER_ID_SUB2].pin_b = P(0, 12),
        .steppers[STEPPER_ID_SUB2].pin_c = P(0, 18),

        .pin_test_clk = P(0, 27),

        .pins_pulled_down = (const uint16_t[]){
            P(0, 20), // battery load
            P(0, 26), // vibrator
            P(0, 24), // acc vdd
            P(0, 4),  // prodtest
        },
        .pins_pulled_down_count = 4,
        .pins_pulled_up = (const uint16_t[]){
            P(0, 19), // pusher up
            P(0, 11), // pusher mid
            P(0, 25), // pusher down
        },
        .pins_pulled_up_count = 3,

        .stepper_mounted = pluto_stepper_mounted,

        .uicr_info.pronto.marker_prodtest_bitsize = 4,
        .uicr_info.pronto.marker_coretest_bitsize = 1,
        .uicr_info.pronto.marker_movementtest_bitsize = 1,
        .uicr_info.prodtest_flags = 4,
};
