#include "bmtypes.h"
#include "controller.h"
#include "settings.h"
#include "rtc.h"
#include "timer.h"
#include "dbg_kiln_model.h"
#include "thermo.h"
#include "minio.h"
#include "board.h"
#include "gpio_driver.h"
#include "settings.h"
#include "kiln.h"
#include "gpio_driver.h"
#include "ui/ui.h"
#include "assert.h"

typedef struct
{
    float set, act, output, output_max, output_min;
    float error_epsilon;
    float error_prev;
    float integral;
    float k_p, k_i, k_d;
} pid_t;

static struct
{
    volatile int panic;
    volatile control_state_t state;
    float kiln_temp_manual;
    uint32_t now;
    uint32_t kiln_start_time;
    volatile uint32_t kiln_full_power_time;
    uint8_t kiln_power_manual;
    uint8_t kiln_cycle_ix;
    uint8_t kiln_power_active_cycles;
    timer_t kiln_timer;
    pid_t pid;
} ctrl;

static ui_event_t kiln_event_on = {
    .type = EVENT_KILN_ON,
    .is_static = 1,
};
static ui_event_t kiln_event_off = {
    .type = EVENT_KILN_OFF,
    .is_static = 1,
};

static float ctrl_pid_regulate(pid_t *pid, float act, float dt)
{
    float error = pid->set - act;
    pid->act = act;
    if (error > pid->error_epsilon || error < -pid->error_epsilon)
    {
        pid->integral += error * dt;
    }
    float deriv = (error - pid->error_prev) / dt;

    pid->output =
        pid->k_p * error +
        pid->k_i * pid->integral +
        pid->k_d * deriv;

    if (pid->output > pid->output_max)
        pid->output = pid->output_max;
    if (pid->output < pid->output_min)
        pid->output = pid->output_min;

    pid->error_prev = error;

    return pid->output;
}

static void ctrl_kiln_timer_cb(timer_t *t)
{
    static uint8_t therm_err_cnt = 0;
    thermo_result_t therm_res;
    uint32_t dt_ms = (uint32_t)settings_get_val_for_id(SETTING_KILN_MIN_SWITCH_TIME);
    float dt = dt_ms / 1000.0f;
    thermoc_temp(&therm_res);
    if (therm_res.error)
    {
        gui_set_attention(ATT_THERMO_ERR);
        if (therm_res.is_new)
        {
            therm_err_cnt++;
            if (therm_err_cnt > THERMOCOUPLE_MAX_NBR_CONSEQ_ERRS)
            {
                ctrl_panic();
            }
        }
    }
    else
    {
        therm_err_cnt = 0;
    }
    char max_temp = therm_res.temperature > (int32_t)settings_get_val_for_id(SETTING_KILN_MAX_TEMP);
    if (max_temp)
        gui_set_attention(ATT_MAX_TEMP);

    if (!max_temp && !ctrl.panic && ctrl.state)
    {
        uint8_t active_cycles = (uint8_t)(ctrl.pid.output * CTRL_CYCLES);
        if (ctrl.state == CONTROL_MANUAL)
        {
            uint8_t max_active_cycles =
                (uint8_t)((ctrl.kiln_power_manual * CTRL_CYCLES) / 100);
            if (active_cycles > max_active_cycles)
                active_cycles = max_active_cycles;
        }
        ctrl.kiln_power_active_cycles = active_cycles;
        if (active_cycles >= CTRL_CYCLES)
        {
            if (ctrl.kiln_full_power_time == 0)
            {
                ctrl.kiln_full_power_time = ctrl.now;
            }
        }
        else
        {
            ctrl.kiln_full_power_time = 0;
        }
        // cycle backwards, so we start with kiln off =>
        // if value is changed, off state will be preferred
        int enable_power = (active_cycles > (CTRL_CYCLES - ctrl.kiln_cycle_ix - 1));
        if (settings_get_val_for_id(SETTING_KILN_SIMULATION))
        {
            model_tick(enable_power ? 12.0f : 0, dt);
        }
        else
        {
            // turn on relay acc to enable_power
            gpio_set(PIN_ELEMENT_1, enable_power ? 1 : 0);
            gpio_set(PIN_ELEMENT_2, enable_power ? 1 : 0);
            gpio_set(PIN_ELEMENT_3, enable_power ? 1 : 0);
            gpio_set(PIN_ELEMENT_4, enable_power ? 1 : 0);
        }
        gpio_set(BOARD_LED_GPIO_PIN[1], enable_power ? BOARD_LED_GPIO_ACTIVE[1] : !BOARD_LED_GPIO_ACTIVE[1]);
        ctrl_pid_regulate(&ctrl.pid, therm_res.temperature, dt);
        ctrl.kiln_cycle_ix++;
        if (ctrl.kiln_cycle_ix >= CTRL_CYCLES)
            ctrl.kiln_cycle_ix = 0;
    }
    else
    {
        // turn off relays
        ctrl_elements_off();
        if (settings_get_val_for_id(SETTING_KILN_SIMULATION))
        {
            model_tick(0, dt);
        }
        gpio_set(BOARD_LED_GPIO_PIN[1], !BOARD_LED_GPIO_ACTIVE[1]);
    }
    // get switch setting each callback if it changes
    timer_start(&ctrl.kiln_timer, ctrl_kiln_timer_cb,
                TIMER_MS_TO_TICKS(dt_ms),
                TIMER_ONESHOT);
    gui_status_report_ctrl(ctrl.state, therm_res.temperature, (int)ctrl.pid.set, (int)(ctrl.pid.output * 100));
}

static void ctrl_start(control_state_t c)
{
    ASSERT(ctrl.state == CONTROL_OFF);
    if (ctrl.state == CONTROL_OFF)
    {
        ctrl.state = c;
        ctrl.kiln_cycle_ix = 0;
        ctrl.pid.error_prev = 0;
        ctrl.pid.integral = 0;
        ctrl.pid.output = 0;
        kiln_event_on.value = CONTROL_MANUAL;
        ctrl.kiln_start_time = ctrl.now;
        ui_post_event(&kiln_event_on);
        printf("ctrl kiln started temp:%d pow:%d\n", (uint32_t)ctrl.pid.set, ctrl.kiln_power_manual);
    }
}

void ctrl_second_callback(uint32_t second)
{
    ctrl.now = second;
    if (ctrl.state != CONTROL_OFF)
    {
        if (second - ctrl.kiln_start_time > (uint32_t)(settings_get_val_for_id(SETTING_KILN_MAX_TIME)) * 3600)
        {
            ctrl_stop();
            gui_set_attention(ATT_MAX_ON_TIME);
        }

        if (ctrl.kiln_full_power_time &&
            second - ctrl.kiln_full_power_time > (uint32_t)(settings_get_val_for_id(SETTING_KILN_MAX_TIME_FULL_PWR)) * 60)
        {
            ctrl_stop();
            gui_set_attention(ATT_MAX_POW_TIME);
        }
    }
}

void ctrl_stop(void)
{
    ctrl_elements_off();
    if (ctrl.state != CONTROL_OFF)
    {
        ctrl.state = CONTROL_OFF;
        ui_post_event(&kiln_event_off);
    }
    printf("ctrl kiln stopped\n");
}

void ctrl_manual_set_temp(float temp)
{
    printf("ctrl manual temp %d\n", (uint32_t)temp);
    ctrl.kiln_temp_manual = temp;
    if (ctrl.state == CONTROL_MANUAL)
    { // TODO handle state transitions
        ctrl.pid.set = temp;
    }
}

void ctrl_manual_set_power(uint8_t power)
{
    printf("ctrl manual pow %d\n", power);
    ctrl.kiln_power_manual = power;
    if (power == 0)
    {
        ctrl_stop();
    }
    else
    {
        ctrl.pid.set = ctrl.kiln_temp_manual;
        if (ctrl.state != CONTROL_MANUAL)
        {
            ctrl_stop();
            ctrl_start(CONTROL_MANUAL);
        }
    }
}

void ctrl_pid_set_epsilon(uint32_t div_by_10000)
{
    ctrl.pid.error_epsilon = div_by_10000 / 10000.0f;
}
void ctrl_pid_set_k_p(uint32_t div_by_10000)
{
    ctrl.pid.k_p = div_by_10000 / 10000.0f;
}
void ctrl_pid_set_k_i(uint32_t div_by_10000)
{
    ctrl.pid.k_i = div_by_10000 / 10000.0f;
}
void ctrl_pid_set_k_d(uint32_t div_by_10000)
{
    ctrl.pid.k_d = div_by_10000 / 10000.0f;
}
void ctrl_elements_off(void)
{
    gpio_set(PIN_ELEMENT_1, 0);
    gpio_set(PIN_ELEMENT_2, 0);
    gpio_set(PIN_ELEMENT_3, 0);
    gpio_set(PIN_ELEMENT_4, 0);
}

void ctrl_init(void)
{
    ctrl_elements_off();
    gpio_config(PIN_ELEMENT_1, GPIO_DIRECTION_OUTPUT, GPIO_PULL_DOWN);
    gpio_config(PIN_ELEMENT_2, GPIO_DIRECTION_OUTPUT, GPIO_PULL_DOWN);
    gpio_config(PIN_ELEMENT_3, GPIO_DIRECTION_OUTPUT, GPIO_PULL_DOWN);
    gpio_config(PIN_ELEMENT_4, GPIO_DIRECTION_OUTPUT, GPIO_PULL_DOWN);
    ctrl_elements_off();

    ctrl.panic = 0;
    ctrl.state = CONTROL_OFF;
    ctrl.pid.set = 20;
    ctrl.pid.output_max = 1.0;
    ctrl.pid.output_min = 0.0;
    ctrl.pid.k_p = (float)((uint32_t)settings_get_val_for_id(SETTING_PID_K_P) / 10000.0f);
    ctrl.pid.k_i = (float)((uint32_t)settings_get_val_for_id(SETTING_PID_K_I) / 10000.0f);
    ctrl.pid.k_d = (float)((uint32_t)settings_get_val_for_id(SETTING_PID_K_D) / 10000.0f);
    ctrl.pid.error_epsilon = (float)((uint32_t)settings_get_val_for_id(SETTING_PID_EPSILON) / 10000.0f);
    uint32_t dt_ms = (uint32_t)settings_get_val_for_id(SETTING_KILN_MIN_SWITCH_TIME);
    timer_start(&ctrl.kiln_timer, ctrl_kiln_timer_cb,
                TIMER_MS_TO_TICKS(dt_ms),
                TIMER_ONESHOT);
}

void ctrl_panic(void)
{
    if (!ctrl.panic)
    {
        ctrl.panic = 1;
        gui_set_attention(ATT_PANIC);
        ui_post_event(ui_new_event(EVENT_PANIC));
    }
}

int ctrl_is_panicking(void)
{
    return ctrl.panic;
}

int ctrl_is_enabled(void)
{
    return ctrl.state != CONTROL_OFF;
}
