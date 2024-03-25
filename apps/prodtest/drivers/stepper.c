#include "errors.h"
#include "targets.h"
#include "stepper.h"
#include "gpio_driver.h"
#include "minio.h"

#define MAX_PHASES 2
#define MAX_SEQUENCES 2

typedef enum
{
    FLOAT,
    HIGH,
    LOW,
    RESET
} pin_state_t;

// increasing phase means going clockwise
typedef struct
{
    uint8_t phases;
    struct
    {
        uint8_t sequences;
        struct
        {
            pin_state_t a;
            pin_state_t b;
            pin_state_t c;
            uint16_t delay_us;
        } pindef[MAX_PHASES][MAX_SEQUENCES];
    } dir[MAX_STEPPER_DIRS];
} stepper_driver_t;

static const stepper_driver_t drivers[MAX_STEPPER_TYPES] = {
    [STEPPER_TYPE_SOPROD_2PHASE] = {
        .phases = 2,
        .dir[STEPPER_DIR_CW] = {.sequences = 2, .pindef[0] = {{.a = HIGH, .b = FLOAT, .c = LOW, .delay_us = 2900}, {.a = LOW, .b = LOW, .c = LOW, .delay_us = 0}}, .pindef[1] = {{.a = LOW, .b = FLOAT, .c = HIGH, .delay_us = 2900}, {.a = LOW, .b = LOW, .c = LOW, .delay_us = 0}}},
        .dir[STEPPER_DIR_CCW] = {.sequences = 2, .pindef[0] = {{.a = FLOAT, .b = HIGH, .c = LOW, .delay_us = 2900}, {.a = LOW, .b = LOW, .c = LOW, .delay_us = 0}}, .pindef[1] = {{.a = FLOAT, .b = LOW, .c = HIGH, .delay_us = 2900}, {.a = LOW, .b = LOW, .c = LOW, .delay_us = 0}}}},
};

static uint8_t stepper_phase[MAX_STEPPER_IDS];

static NRF_GPIO_Type *port_for_pin(uint16_t pin)
{
    return pin < 32 ? NRF_P0 : NRF_P1;
}

static void pin_def(uint16_t pin, pin_state_t state)
{
    if (pin == BOARD_PIN_UNDEF)
        return;
    NRF_GPIO_Type *port = port_for_pin(pin);
    switch (state)
    {
    case LOW:
        gpio_set(pin, 0);
        port->PIN_CNF[pin & 0x1f] =
            (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
            (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos) |
            (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos) |
            (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos) |
            (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);

        break;
    case HIGH:
        gpio_set(pin, 1);
        port->PIN_CNF[pin & 0x1f] =
            (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
            (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos) |
            (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos) |
            (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos) |
            (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);
        break;
    case FLOAT:
        port->PIN_CNF[pin & 0x1f] =
            (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
            (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos) |
            (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos) |
            (GPIO_PIN_CNF_DRIVE_D0S1 << GPIO_PIN_CNF_DRIVE_Pos) |
            (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);
        break;
    case RESET:
        target_reset_pin(pin);
        break;
    }
}

static void config_all_stepper_pin(pin_state_t state)
{
    const target_t *target = target_get();
    for (stepper_id_t id = 0; id < MAX_STEPPER_IDS; id++)
    {
        const stepper_motor_t *stepper = &target->steppers[id];
        if (!stepper->routed)
            continue;
        pin_def(stepper->pin_a, state);
        pin_def(stepper->pin_b, state);
        pin_def(stepper->pin_c, state);
    }
}

int stepper_move_multistep_multiple(const stepper_id_t *ids, const uint16_t *steps, const stepper_dir_t *dirs, int steppers) 
{
    uint16_t steps_left[steppers];
    memcpy(steps_left, steps, sizeof(steps_left));
    uint32_t steps_left_sum = 0;
    for (int i = 0; i < steppers; i++) {
        steps_left_sum += steps_left[i];
    }
    int err = ERROR_OK;
    while (steps_left_sum > 0 && err == ERROR_OK) {
        stepper_id_t ids_to_move[steppers];
        stepper_dir_t dirs_to_move[steppers];
        int to_move_count = 0;
        for (int i = 0; i < steppers; i++) {
            if (steps_left[i] > 0) {
                ids_to_move[to_move_count] = ids[i];
                dirs_to_move[to_move_count] = dirs[i];
                to_move_count++;
                steps_left[i]--;
                steps_left_sum--;
            }
        }
        err = stepper_move_singlestep_multiple(ids_to_move, dirs_to_move, to_move_count);
    }

    return err;
}


int stepper_move_singlestep_multiple(const stepper_id_t *ids, const stepper_dir_t *dirs, int steppers)
{
    const target_t *target = target_get();
    for (int i = 0; i < steppers; i++) {
        if (ids[i] >= MAX_STEPPER_IDS || dirs[i] > MAX_STEPPER_DIRS)
            return -EINVAL;
        const stepper_motor_t *stepper = &target->steppers[ids[i]];
            if (!stepper->routed)
                return -ENOTSUP;
    }

    uint32_t max_delay_us = 0;

    for (int i = 0; i < steppers; i++) {
        stepper_id_t id = ids[i];
        stepper_dir_t dir = dirs[i];
        const stepper_motor_t *stepper = &target->steppers[id];
        const stepper_driver_t *driver = &drivers[stepper->stepper_type];

        uint8_t phase = stepper_phase[id];
        phase = (dir == STEPPER_DIR_CW) ? (phase + 1) : (phase - 1);
        phase %= driver->phases;
        stepper_phase[id] = phase;

        for (int seq = 0; seq < driver->dir[dir].sequences; seq++)
        {
            pin_def(stepper->pin_a, driver->dir[dir].pindef[phase][seq].a);
            pin_def(stepper->pin_b, driver->dir[dir].pindef[phase][seq].b);
            pin_def(stepper->pin_c, driver->dir[dir].pindef[phase][seq].c);
            cpu_halt_us(driver->dir[dir].pindef[phase][seq].delay_us);
        }
        if (stepper->delay_us > max_delay_us) {
            max_delay_us = stepper->delay_us;
        }
    }

    cpu_halt_us(max_delay_us);

    return 0;
}

int stepper_move_singlestep(stepper_id_t id, stepper_dir_t dir)
{
    stepper_id_t ids[1] = {id};
    stepper_dir_t dirs[1] = {dir};
    return stepper_move_singlestep_multiple(ids, dirs, 1);
}

void stepper_init(void)
{
    config_all_stepper_pin(LOW);
    memset(stepper_phase, 0, sizeof(stepper_phase));
}

void stepper_deinit(void)
{
    config_all_stepper_pin(RESET);
}

#if NO_EXTRA_CLI == 0

#include "cli.h"

static int cli_step_info(int argc, const char **argv)
{
    const target_t *target = target_get();
    printf("stepper info\r\n");
    for (stepper_id_t i = 0; i < MAX_STEPPER_IDS; i++)
    {
        const stepper_motor_t *stepper = &target->steppers[i];
        printf("id: %d", i);
        bool mounted = true;
        if (!stepper->routed)
        {
            printf("\tnot routed\r\n");
            continue;
        }
        if (target->stepper_mounted != NULL) {
            mounted = target->stepper_mounted(i);
        }
        printf("\t%smount", mounted ? "" : "un");
        printf("\ttype:%d", stepper->stepper_type);
        printf("\tsteps/rev:%d", stepper->steps_per_rev);
        printf("\tpins[a,b,c]:[%d,%d,%d]\r\n", stepper->pin_a, stepper->pin_b, stepper->pin_c);
    }
    return 0;
}
CLI_FUNCTION(cli_step_info, "step_info", "show stepper motor information");

static int cli_step_move(int argc, const char **argv)
{
    if (argc == 0 || (argc % 3) != 0) return ERR_CLI_EINVAL;
    int steppers = argc / 3;
    stepper_id_t ids[steppers];
    stepper_dir_t dirs[steppers];
    uint16_t steps[steppers];
    for (int i = 0; i < argc; i += 3) {
        ids[i/3] = strtol(argv[i+0], NULL, 0);
        dirs[i/3] = strcmp(argv[i+1], "ccw" ) == 0 ? STEPPER_DIR_CCW : STEPPER_DIR_CW;
        steps[i/3] = strtol(argv[i+2], NULL, 0);
    }
    return stepper_move_multistep_multiple(ids, steps, dirs, steppers);
}
CLI_FUNCTION(cli_step_move, "step_move", "move steppers: (<id> <cw|ccw> <steps>)*");

#endif