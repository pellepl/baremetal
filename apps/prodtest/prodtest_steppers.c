#include "prodtest.h"
#include "stepper.h"
#include "gpio_driver.h"
#include "cli.h"
#include "minio.h"

static void mast_inputify(const uint16_t *pins, uint8_t num_pins)
{
    for (uint8_t i = 0; i < num_pins; i++)
    {
        if (pins[i] == BOARD_PIN_UNDEF)
            continue;
        target_reset_pin(pins[i]);
    }
}

static int mast_test_short_ref(const uint16_t *pins, uint8_t num_pins, gpio_pull_t pull, bool output)
{
    int err = ERROR_OK;
    for (uint8_t i = 0; i < num_pins; i++)
    {
        uint16_t pin = pins[i];
        if (pin == BOARD_PIN_UNDEF)
            continue;
        NRF_GPIO_Type *port = prodtest_port_for_pin(pin);
        port->PIN_CNF[pin & 0x1f] =
            (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
            (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) |
            ((pull == GPIO_PULL_NONE ? GPIO_PIN_CNF_PULL_Disabled : (pull == GPIO_PULL_DOWN ? GPIO_PIN_CNF_PULL_Pulldown : GPIO_PIN_CNF_PULL_Pullup)) << GPIO_PIN_CNF_PULL_Pos) |
            (GPIO_PIN_CNF_DRIVE_D0S1 << GPIO_PIN_CNF_DRIVE_Pos) |
            (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);
    }
    cpu_halt(10);
    const uint8_t expected_output = (pull == GPIO_PULL_DOWN) ? 0 : 1;
    for (uint8_t i = 0; i < num_pins; i++)
    {
        if (pins[i] == BOARD_PIN_UNDEF)
            continue;
        if (gpio_read(pins[i]) != expected_output)
        {
            err = (expected_output == 0) ? ESTEPPERMOTOR_SHORT_HIGH : ESTEPPERMOTOR_SHORT_LOW;
            if (output)
            {
                printf("ERROR pin P%1d.%02d shorted to %s\r\n", pins[i] >> 5, pins[i] & 0x1f,
                       (pull == GPIO_PULL_DOWN) ? "VCC" : "GND");
            }
        }
    }
    return err;
}

static int mast_test_connection(const uint16_t *pins, uint8_t num_pins, bool connected, bool output)
{
    int err = ERROR_OK;
    const uint8_t expected_output = connected ? 1 : 0;
    for (uint8_t i = 0; i < num_pins; i++)
    {
        uint16_t pin = pins[i];
        if (pin == BOARD_PIN_UNDEF)
            continue;
        NRF_GPIO_Type *port = prodtest_port_for_pin(pin);
        port->PIN_CNF[pin & 0x1f] =
            (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
            (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) |
            (GPIO_PIN_CNF_PULL_Pulldown << GPIO_PIN_CNF_PULL_Pos) |
            (GPIO_PIN_CNF_DRIVE_D0S1 << GPIO_PIN_CNF_DRIVE_Pos) |
            (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);
    }
    cpu_halt(10);

    for (uint8_t i = 0; i < num_pins; i++)
    {
        if (pins[i] == BOARD_PIN_UNDEF)
            continue;
        uint8_t output_pin = pins[i];
        gpio_set(output_pin, 0);
        NRF_GPIO_Type *port = prodtest_port_for_pin(output_pin);
        port->PIN_CNF[output_pin & 0x1f] =
            (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
            (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos) |
            (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos) |
            (GPIO_PIN_CNF_DRIVE_S0H1 << GPIO_PIN_CNF_DRIVE_Pos) |
            (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);
        gpio_set(output_pin, 1);
        cpu_halt(10);

        for (uint8_t j = i + 1; j < num_pins; j++)
        {
            if (pins[j] == BOARD_PIN_UNDEF)
                continue;
            uint8_t connected_pin = pins[j];
            if (gpio_read(connected_pin) != expected_output)
            {
                err = ESTEPPERMOTOR_COIL_NC;
                if (output)
                {
                    printf("ERROR pins P%1d.%02d and P%1d.%02d are%s connected\r\n", output_pin >> 5,
                           output_pin & 0x1f, connected_pin >> 5, connected_pin & 0x1f, connected ? " not" : "");
                }
            }
        }

        port = prodtest_port_for_pin(output_pin);
        port->PIN_CNF[output_pin & 0x1f] =
            (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
            (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) |
            (GPIO_PIN_CNF_PULL_Pulldown << GPIO_PIN_CNF_PULL_Pos) |
            (GPIO_PIN_CNF_DRIVE_D0S1 << GPIO_PIN_CNF_DRIVE_Pos) |
            (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);
    }
    return err;
}

static int mast_test_stepper(const stepper_motor_t *stepper, bool connected, bool output)
{
    int err = ERROR_OK;
    uint16_t pins[3];
    pins[0] = stepper->pin_a;
    pins[1] = stepper->pin_b;
    pins[2] = stepper->pin_c;
    if (output)
    {
        printf("testing stepper pins ");
        for (uint8_t i = 0; i < 3; i++)
        {
            if (pins[i] == BOARD_PIN_UNDEF)
                continue;
            printf("P%1d.%02d ", pins[i] >> 5, pins[i] & 0x1f);
        }
        printf("- %s\r\n", connected ? "mounted" : "not mounted");
    }
    int err_gnd = mast_test_short_ref(pins, 3, GPIO_PULL_UP, output);
    if (err == ERROR_OK)
    {
        err = err_gnd;
    }
    int err_vcc = mast_test_short_ref(pins, 3, GPIO_PULL_DOWN, output);
    if (err == ERROR_OK)
    {
        err = err_vcc;
    }
    int err_con = mast_test_connection(pins, 3, connected, output);
    if (err == ERROR_OK)
    {
        err = err_con;
    }
    mast_inputify(pins, 3);
    if (output && err == ERROR_OK)
    {
        printf("pins ok\r\n");
    }

    return err;
}

static int cli_mast(int argc, const char **argv)
{
    const bool output = argc >= 1 && argv[0][0] == '2';
    int err = ERROR_OK;

    const target_t *target = target_get();
    for (stepper_id_t id = 0; id < MAX_STEPPER_IDS; id++)
    {
        if (target->steppers[id].routed)
        {
            bool mounted = true;
            if (target->stepper_mounted != NULL) {
                mounted = target->stepper_mounted(id);
            }
            int err_stepper = mast_test_stepper(&target->steppers[id], mounted, output);
            if (err == ERROR_OK)
            {
                err = err_stepper;
            }
        }
    }

    printf("MAST result : %s;\r\n", err == ERROR_OK ? "PASSED" : "FAILED");

    return ERROR_OK;
}
CLI_FUNCTION(cli_mast, "MAST", "Test all stepper motors, MAST=2; gives detailed output");

static int cli_step(int argc, const char **argv) {
    if (argc != 3 && argc != 4) {
        return -EINVAL;
    }
    const target_t *target = target_get();
    stepper_id_t id = strtol(argv[0], NULL, 0);
    uint32_t steps = strtol(argv[1], NULL, 0);
    stepper_dir_t dir = strtol(argv[2], NULL, 0) == 0 ? STEPPER_DIR_CW : STEPPER_DIR_CCW;
    uint32_t delay_ms = argc == 4 ? strtol(argv[3], NULL, 0) : 0;
    if (id >= MAX_STEPPER_IDS) {
        return -EINVAL;
    }
    if (!target->steppers[id].routed) {
        return -ENOTSUP;
    }
    stepper_init();
    while (steps--) {
        int err = stepper_move_singlestep(id, dir);
        if (err) {
            return err;
        }
        if (delay_ms) cpu_halt(delay_ms);

    }
    stepper_deinit();
    
    return ERROR_OK;
}
CLI_FUNCTION(cli_step, "STEP", "Step motor control. STEP=Hand,Steps,Direction[,Delay]; 0=Hour hand 1, steps 1 to 720, direction 0 = clockwise, 1 = counter, delay in ms (optional)")
