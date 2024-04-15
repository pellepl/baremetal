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
        .dir[STEPPER_DIR_CW] = {.sequences = 2, .pindef[0] = {{.a = FLOAT, .b = HIGH, .c = LOW, .delay_us = 2900}, {.a = LOW, .b = LOW, .c = LOW, .delay_us = 0}}, .pindef[1] = {{.a = FLOAT, .b = LOW, .c = HIGH, .delay_us = 2900}, {.a = LOW, .b = LOW, .c = LOW, .delay_us = 0}}},
        .dir[STEPPER_DIR_CCW] = {.sequences = 2, .pindef[0] = {{.a = HIGH, .b = FLOAT, .c = LOW, .delay_us = 2900}, {.a = LOW, .b = LOW, .c = LOW, .delay_us = 0}}, .pindef[1] = {{.a = LOW, .b = FLOAT, .c = HIGH, .delay_us = 2900}, {.a = LOW, .b = LOW, .c = LOW, .delay_us = 0}}}},
};

static struct
{
    uint32_t step_delay_us;
    uint32_t sequence_delay_us;
} stepper_delay_overrides[MAX_STEPPER_IDS];

static struct stepper_state
{
    uint16_t current_step;
    uint8_t phase;
} states[MAX_STEPPER_IDS];

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
    uint32_t total_already_delayed_us = 0;

    for (int i = 0; i < steppers; i++) {
        stepper_id_t id = ids[i];
        stepper_dir_t dir = dirs[i];
        const stepper_motor_t *stepper = &target->steppers[id];
        const stepper_driver_t *driver = &drivers[stepper->stepper_type];
        struct stepper_state *state = &states[id];
        uint32_t delay_step_us = stepper->delay_us;
        if (stepper_delay_overrides[id].step_delay_us != 0)
        {
            delay_step_us = stepper_delay_overrides[id].step_delay_us;
        }

        uint8_t phase = state->phase;
        phase = (dir == STEPPER_DIR_CW) ? (phase + 1) : (phase - 1);
        phase %= driver->phases;
        state->phase = phase;

        uint32_t instance_already_delayed_us = 0;
        for (int seq = 0; seq < driver->dir[dir].sequences; seq++)
        {
            uint32_t delay_seq_us = driver->dir[dir].pindef[phase][seq].delay_us;
            if (stepper_delay_overrides[id].sequence_delay_us != 0)
            {
                delay_seq_us = stepper_delay_overrides[id].sequence_delay_us;
            }
            pin_def(stepper->pin_a, driver->dir[dir].pindef[phase][seq].a);
            pin_def(stepper->pin_b, driver->dir[dir].pindef[phase][seq].b);
            pin_def(stepper->pin_c, driver->dir[dir].pindef[phase][seq].c);
            cpu_halt_us(delay_seq_us);
            total_already_delayed_us += delay_seq_us;
            instance_already_delayed_us += delay_seq_us;
        }
        if (delay_step_us > max_delay_us)
        {
            max_delay_us = delay_step_us + instance_already_delayed_us;
        }
        if (dir == STEPPER_DIR_CCW)
        {
            if (state->current_step == 0)
            {
                state->current_step = stepper->steps_per_rev;
            }
            state->current_step--;
        }
        else
        {
            state->current_step++;
            if (state->current_step >= stepper->steps_per_rev)
            {
                state->current_step = 0;
            }
        }
    }
    if (total_already_delayed_us < max_delay_us)
    {
        cpu_halt_us(max_delay_us - total_already_delayed_us);
    }

    return 0;
}

int stepper_hour_to_step(const stepper_id_t id, uint8_t hour)
{
    const target_t *target = target_get();
    hour %= 12;
    if (id >= MAX_STEPPER_IDS)
        return -EINVAL;
    const stepper_motor_t *stepper = &target->steppers[id];
    if (!stepper->routed)
        return -ENOTSUP;
    return stepper->steps_per_rev * hour / 12;
}

int stepper_position_multiple_hour(const stepper_id_t *ids, const uint8_t *hours, int steppers)
{
    const target_t *target = target_get();
    uint16_t steps[steppers];
    stepper_dir_t dirs[steppers];
    for (int i = 0; i < steppers; i++)
    {
        stepper_id_t id = ids[i];
        if (id >= MAX_STEPPER_IDS)
            return -EINVAL;
        const stepper_motor_t *stepper = &target->steppers[id];
        if (!stepper->routed)
            return -ENOTSUP;

        uint16_t target_step = stepper_hour_to_step(id, hours[i]);
        if (target_step < states[id].current_step)
        {
            target_step += stepper->steps_per_rev;
        }
        uint16_t delta_step = target_step - states[id].current_step;
        if (delta_step > stepper->steps_per_rev / 2)
        {
            // ccw
            dirs[i] = STEPPER_DIR_CCW;
            steps[i] = stepper->steps_per_rev - delta_step;
        }
        else
        {
            // cw
            dirs[i] = STEPPER_DIR_CW;
            steps[i] = delta_step;
        }
    }

    return stepper_move_multistep_multiple(ids, steps, dirs, steppers);
}

int stepper_position_all_hour(uint8_t hour)
{
    const target_t *target = target_get();
    stepper_id_t ids[MAX_STEPPER_IDS];
    uint8_t hours[MAX_STEPPER_IDS];
    int steppers = 0;
    for (stepper_id_t id = 0; id < MAX_STEPPER_IDS; id++)
    {
        const stepper_motor_t *stepper = &target->steppers[id];
        if (!stepper->routed)
            continue;
        ids[steppers] = id;
        hours[steppers] = hour;
        steppers++;
    }
    return stepper_position_multiple_hour(ids, hours, steppers);
}

void stepper_position_reset_reference(stepper_id_t id)
{
    if (id >= MAX_STEPPER_IDS)
        return;
    states[id].current_step = 0;
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
    memset(states, 0, sizeof(states));
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
        printf("\tstep:%3d", states[i].current_step);
        printf("\tpins[a,b,c]:[%d,%d,%d]\r\n", stepper->pin_a, stepper->pin_b, stepper->pin_c);
    }
    return 0;
}
CLI_FUNCTION(cli_step_info, "step_info", "show stepper motor information");

static int cli_step_move(int argc, const char **argv)
{
    if (argc == 0 || (argc % 2) != 0)
        return ERR_CLI_EINVAL;
    int steppers = argc / 2;
    stepper_id_t ids[steppers];
    stepper_dir_t dirs[steppers];
    uint16_t steps[steppers];
    for (int i = 0; i < argc; i += 3) {
        ids[i / 2] = strtol(argv[i + 0], NULL, 0);
        int asteps = strtol(argv[i + 1], NULL, 0);
        dirs[i / 2] = asteps < 0 ? STEPPER_DIR_CCW : STEPPER_DIR_CW;
        steps[i / 2] = asteps < 0 ? -asteps : asteps;
    }
    return stepper_move_multistep_multiple(ids, steps, dirs, steppers);
}
CLI_FUNCTION(cli_step_move, "step_move", "move steppers, pos steps for CW, neg for CCW: (<id> <steps>)*");

static int cli_step_pos(int argc, const char **argv)
{
    if (argc == 1)
    {
        return stepper_position_all_hour(strtol(argv[0], NULL, 0));
    }
    if (argc == 0 || (argc % 2) != 0)
        return ERR_CLI_EINVAL;
    int steppers = argc / 2;
    stepper_id_t ids[steppers];
    uint8_t hours[steppers];
    for (int i = 0; i < argc; i += 2)
    {
        ids[i / 2] = strtol(argv[i + 0], NULL, 0);
        hours[i / 2] = strtol(argv[i + 1], NULL, 0) % 12;
    }
    return stepper_position_multiple_hour(ids, hours, steppers);
}
CLI_FUNCTION(cli_step_pos, "step_pos", "posistion steppers: (<id> <hour:0-11>)* | <hour:0-11>");

static int cli_step_conf(int argc, const char **argv)
{
    const target_t *target = target_get();
    if (argc == 0)
    {
        for (stepper_id_t i = 0; i < MAX_STEPPER_IDS; i++)
        {
            const stepper_motor_t *stepper = &target->steppers[i];
            if (!stepper->routed)
                continue;

            printf("id: %d", i);
            printf("\tstep delay: ");
            if (stepper_delay_overrides[i].step_delay_us != 0)
            {
                printf("%d us", stepper_delay_overrides[i].step_delay_us);
            }
            else
            {
                printf("DRIVER");
            }
            printf("\tsequence delay: ");
            if (stepper_delay_overrides[i].sequence_delay_us != 0)
            {
                printf("%d us", stepper_delay_overrides[i].sequence_delay_us);
            }
            else
            {
                printf("DRIVER");
            }
            printf("\n");
        }
        return ERROR_OK;
    }
    if (argc != 3)
        return ERR_CLI_EINVAL;
    stepper_id_t id = strtol(argv[0], NULL, 0);
    if (id >= MAX_STEPPER_IDS)
        return -EINVAL;
    const stepper_motor_t *stepper = &target->steppers[id];
    if (!stepper->routed)
        return -ENOTSUP;
    stepper_delay_overrides[id].step_delay_us = strtol(argv[1], NULL, 0);
    stepper_delay_overrides[id].sequence_delay_us = strtol(argv[2], NULL, 0);
    return ERROR_OK;
}
CLI_FUNCTION(cli_step_conf, "step_conf", "override driver config times: <id> <step delay us> <sequence delay us>");

static int cli_step_calibrate(int argc, const char **argv)
{
    if (argc == 0)
        return ERR_CLI_EINVAL;
    stepper_id_t id = strtol(argv[0], NULL, 0);
    const target_t *target = target_get();
    if (id >= MAX_STEPPER_IDS)
        return -EINVAL;
    const stepper_motor_t *stepper = &target->steppers[id];
    if (!stepper->routed)
        return -ENOTSUP;

    button_id_t butts[3] = {BUTTON_ID_PUSHER_DOWN, BUTTON_ID_PUSHER_MIDDLE, BUTTON_ID_PUSHER_UP};
    uint16_t pins[3];
    for (int i = 0; i < 3; i++)
    {
        if (!target->buttons[butts[i]].routed)
            return -ENOTSUP;
        pins[i] = target->buttons[butts[i]].pin;
        gpio_config(pins[i], GPIO_DIRECTION_INPUT, target_default_pull_for_pin(pins[i]));
    }
    printf("calibrate stepper %d\nuse upper button to move ccw\nuse lower button to move cw\npress middle button to exit\n", id);
    stepper_init();
    uint32_t halt = 300;
    while (gpio_read(pins[1]) != target->buttons[BUTTON_ID_PUSHER_MIDDLE].pin_active_high ? 1 : 0)
    {
        if (gpio_read(pins[0]) == target->buttons[BUTTON_ID_PUSHER_DOWN].pin_active_high ? 1 : 0)
        {
            stepper_move_singlestep(id, STEPPER_DIR_CW);
            cpu_halt(halt);
            halt /= 2;
        }
        else if (gpio_read(pins[2]) == target->buttons[BUTTON_ID_PUSHER_UP].pin_active_high ? 1 : 0)
        {
            stepper_move_singlestep(id, STEPPER_DIR_CCW);
            cpu_halt(halt);
            halt /= 2;
        }
        else
        {
            halt = 300;
        }
    }
    stepper_position_reset_reference(id);
    stepper_deinit();
    for (int i = 0; i < 3; i++)
    {
        target_reset_pin(pins[i]);
    }
    return ERROR_OK;
}
CLI_FUNCTION(cli_step_calibrate, "step_calib", "calibrate stepper: <id>");

#endif