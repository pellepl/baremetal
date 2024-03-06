#ifndef _TARGETS_H_
#define _TARGETS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <errno.h>
#include "nrf.h"
#include "board_common.h"
#include "errors.h"

#define TARGET_PLUTO    "PLUTO"
#define TARGET_PASCAL   "PASCAL"

#define TARGET_SPEC __attribute__((used, section(".target_entries"))) static const target_t

#define FICR_PROC_NRF52832 0x52832
#define FICR_PROC_NRF52840 0x52840

typedef enum
{
    BUTTON_ID_PUSHER_UP,
    BUTTON_ID_PUSHER_MIDDLE,
    BUTTON_ID_PUSHER_DOWN,
    BUTTON_ID_PRODTEST,
    BUTTON_ID_BTTEST,
    BUTTON_ID_MVTTEST,
    MAX_BUTTON_IDS
} button_id_t;

typedef enum
{
    STEPPER_ID_HOUR,
    STEPPER_ID_MINUTE,
    STEPPER_ID_SUB0,
    STEPPER_ID_SUB1,
    STEPPER_ID_SUB2,
    MAX_STEPPER_IDS
} stepper_id_t;

typedef struct
{
    bool routed;
    uint16_t pin;
    bool pin_active_high;
} button_t;

typedef struct {
    bool routed;
    enum
    {
        STEPPER_TYPE_SOPROD_2PHASE,
        MAX_STEPPER_TYPES
    } stepper_type;
    uint16_t pin_a;
    uint16_t pin_b;
    uint16_t pin_c;
    uint16_t steps_per_rev;
    uint32_t delay_us;
} stepper_motor_t;

typedef struct
{
    uint16_t pin_miso;
    uint16_t pin_mosi;
    uint16_t pin_clk;
    uint16_t pin_cs;
    bool pin_cs_active_high;
    uint8_t mode : 2;
} bus_spi_t;

typedef struct {
    uint8_t id;
    uint16_t pin_sda;
    uint16_t pin_scl;
} bus_i2c_t;

typedef struct
{
    enum
    {
        BUS_TYPE_SPI,
        BUS_TYPE_I2C,
    } bus_type;
    uint32_t bus_speed_hz;
    union
    {
        bus_spi_t spi;
        bus_i2c_t i2c;
    };
} bus_t;

// logical representation of hardware ID, residing
// physically on UICR addresses 0x0x10001080 and 0x10001084
typedef struct
{
    union
    {
        struct
        {
            uint32_t _80;
            uint16_t _84;
        } __attribute__(( packed )) raw;

        struct
        {
            uint8_t pba[4];
            uint8_t rev;
            uint8_t subrev;
        } __attribute__(( packed ))pronto;

        struct
        {
            uint8_t pba[4];
            union
            {
                uint16_t pba_rev;
                struct
                {
                    uint16_t pba_rev_reduced : 4;
                    uint16_t __reserved_1 : 12;
                };
            };
        } __attribute__(( packed )) pascal;
    };
} __attribute__(( packed )) target_hw_identifier_t;

//
// This struct is meant to represent the PCB with all mounting options
//
typedef struct target
{
    char name[16];
    struct {
        uint32_t ficr_proc;
        target_hw_identifier_t uicr;
        // only bits set to 1 are considered when matching ids
        target_hw_identifier_t uicr_mask;
    } id;

    struct {
        uint16_t pin_tx;
        uint16_t pin_rx;
    } uart;

    button_t buttons[MAX_BUTTON_IDS];

    stepper_motor_t steppers[MAX_STEPPER_IDS];

    struct
    {
        bool routed;
        uint16_t pin_load;
        bool pin_load_active_high;
        uint32_t load_delay_ms;
        uint16_t pin_measure_enable;
        bool pin_measure_enable_active_high;
        uint16_t pin_measure;
    } battery;

    struct
    {
        bool routed;
        uint16_t clk;
        uint16_t data;
    } battery_flipflop_enabler;

    struct
    {
        bool routed;
    } charger;

    struct
    {
        bool routed;
        uint16_t pin;
        bool pin_active_high;
    } vibrator;

    struct {
        bool routed;
        enum
        {
            ACC_TYPE_BMA400,
        } type;
        uint16_t pin_vdd;
        bool pin_vdd_active_high;
        uint16_t pin_int;
        uint16_t pin_int_secondary;
        bus_t bus;
    } accelerometer;

    struct
    {
        bool routed;
        uint16_t pin_int;
        bus_t bus;
    } pat9125;

    // pin to output clock on, -1 if not routed
    uint16_t pin_test_clk;

    // specific target init function, NULL if none
    void (*target_init)(const struct target *);
    // specific target mounted query, if NULL then routed indicates also mounted
    bool (*stepper_mounted)(stepper_id_t id);

    const uint16_t *pins_pulled_up;
    uint8_t pins_pulled_up_count;
    const uint16_t *pins_pulled_down;
    uint8_t pins_pulled_down_count;

    struct {
        union {
            struct {
                uint8_t marker_prodtest_bitsize;
                uint8_t marker_coretest_bitsize;
                uint8_t marker_movementtest_bitsize;
            } pronto;
            struct {
            } pascal;
        };
        uint8_t prodtest_flags;
    } uicr_info;
} target_t;

// convenience macro for defining nrf pins in form port/pin
#define P(port, pin) ((port) * 32 + (pin))

const target_t *target_set_by_uicr(void);
int target_count(void);
const target_t *target_get_all(void);
const target_t *target_get(void);
void target_reset_pin(uint16_t pin);

#endif // _TARGETS_H_
