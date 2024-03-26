#include "targets.h"

TARGET_SPEC pca10040 =

    ////////////////////////////////////////////////
    // Unknown target - UICR unwritten - PCA10040 //
    ////////////////////////////////////////////////
    {
        .name = "PCA10040",

        .id.ficr_proc = FICR_PROC_NRF52832,
        .id.uicr.pronto.pba[0] = 0xff,
        .id.uicr.pronto.pba[1] = 0xff,
        .id.uicr.pronto.pba[2] = 0xff,
        .id.uicr.pronto.pba[3] = 0xff,
        .id.uicr.pronto.rev = 0xff,
        .id.uicr.pronto.subrev = 0xff,
        .id.uicr_mask.raw._80 = 0xffffffff,
        .id.uicr_mask.raw._84 = 0xffff,

        .uart.pin_rx = P(0, 8),
        .uart.pin_tx = P(0, 6),

        .buttons[BUTTON_ID_PUSHER_UP].routed = true,
        .buttons[BUTTON_ID_PUSHER_UP].pin = P(0, 13),
        .buttons[BUTTON_ID_PUSHER_UP].pin_active_high = false,

        .buttons[BUTTON_ID_PUSHER_MIDDLE].routed = true,
        .buttons[BUTTON_ID_PUSHER_MIDDLE].pin = P(0, 14),
        .buttons[BUTTON_ID_PUSHER_MIDDLE].pin_active_high = false,

        .buttons[BUTTON_ID_PUSHER_DOWN].routed = true,
        .buttons[BUTTON_ID_PUSHER_DOWN].pin = P(0, 15),
        .buttons[BUTTON_ID_PUSHER_DOWN].pin_active_high = false,

        .buttons[BUTTON_ID_PRODTEST].routed = true,
        .buttons[BUTTON_ID_PRODTEST].pin = P(0, 16),
        .buttons[BUTTON_ID_PRODTEST].pin_active_high = false,

        .vibrator.routed = true,
        .vibrator.pin = P(0, 17),
        .vibrator.pin_active_high = false,

        .pin_test_clk = P(0, 18),

        .accelerometer.routed = true,
        .accelerometer.type = ACC_TYPE_BMA400,
        .accelerometer.pin_vdd = P(0, 24),
        .accelerometer.pin_int = P(0, 27),
        .accelerometer.pin_int_secondary = BOARD_PIN_UNDEF,
        .accelerometer.bus.bus_type = BUS_TYPE_SPI,
        .accelerometer.bus.bus_speed_hz = 8000,
        .accelerometer.bus.spi.mode = 0,
        .accelerometer.bus.spi.pin_clk = P(0, 17),
        .accelerometer.bus.spi.pin_miso = P(0, 13),
        .accelerometer.bus.spi.pin_mosi = P(0, 18),
        .accelerometer.bus.spi.pin_cs = P(0, 19),
        .accelerometer.bus.spi.pin_cs_active_high = false,
};
