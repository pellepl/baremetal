#include "minio.h"
#include "board.h"
#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_exti.h"
#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_tim.h"
#include "input.h"
#include "events.h"
#include "sys.h"

static struct
{
    uint32_t button_mask;
    uint32_t button_press_timestamp[_INPUT_BUTTON_COUNT];
    event_t ev_hw_button;
    event_t ev_button;
    event_t ev_scroll;
    int16_t rot_prev;
    int acc_rot;
} me;

static void rotary_init(void)
{
    /* Enable Clocks for TIM2 and GPIOA */
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2);
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOA);

    /* Configure PA0 (TIM2_CH1) and PA1 (TIM2_CH2) as input floating */
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_0, LL_GPIO_MODE_FLOATING);
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_1, LL_GPIO_MODE_FLOATING);

    /* Configure TIM2 for Encoder Mode */
    LL_TIM_SetEncoderMode(TIM2, LL_TIM_ENCODERMODE_X2_TI1); // X4 mode (counts all edges)
    LL_TIM_IC_SetPolarity(TIM2, LL_TIM_CHANNEL_CH1, LL_TIM_IC_POLARITY_RISING);
    LL_TIM_IC_SetPolarity(TIM2, LL_TIM_CHANNEL_CH2, LL_TIM_IC_POLARITY_RISING);
    LL_TIM_IC_SetFilter(TIM2, LL_TIM_CHANNEL_CH1, LL_TIM_IC_FILTER_FDIV16_N8);
    LL_TIM_IC_SetFilter(TIM2, LL_TIM_CHANNEL_CH2, LL_TIM_IC_FILTER_FDIV16_N8);

    /* Set Auto-Reload to max (16-bit counter) */
    LL_TIM_SetAutoReload(TIM2, 0xFFFF);

    /* Enable TIM2 Counter */
    LL_TIM_EnableCounter(TIM2);

    /* Enable Input Capture Interrupts */
    LL_TIM_EnableIT_CC1(TIM2); // Channel 1 interrupt
    LL_TIM_EnableIT_CC2(TIM2); // Channel 2 interrupt

    /* Enable TIM2 IRQ */
    NVIC_EnableIRQ(TIM2_IRQn);
    NVIC_SetPriority(TIM2_IRQn, 1);

    /* Enable TIM2 Counter */
    LL_TIM_EnableCounter(TIM2);
}

static void buttons_init(void)
{
    /* Enable clock for GPIOA */
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOA);

    /* Set PA4 as input with pull-down (or pull-up if needed) */
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_4, LL_GPIO_MODE_FLOATING);

    /* Enable EXTI4 */
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_AFIO);
    LL_GPIO_AF_SetEXTISource(LL_GPIO_AF_EXTI_PORTA, LL_GPIO_AF_EXTI_LINE4);

    /* Configure EXTI line 4 to trigger on rising & falling edge */
    LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_4);
    LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_4);
    LL_EXTI_EnableFallingTrig_0_31(LL_EXTI_LINE_4);

    /* Enable EXTI4 IRQ */
    NVIC_EnableIRQ(EXTI4_IRQn);
    NVIC_SetPriority(EXTI4_IRQn, 2);
}

void input_init(void)
{
    rotary_init();
    buttons_init();
}

static int16_t input_rot_read(void)
{
    return (int16_t)LL_TIM_GetCounter(TIM2);
}

void input_handle_rotary(void)
{
    int16_t rot = input_rot_read();
    me.acc_rot += rot - me.rot_prev;
    me.rot_prev = rot;
    if (me.acc_rot / INPUT_ROTARY_DIVISOR != 0)
    {
        event_add(&me.ev_scroll, EVENT_UI_SCRL, (void *)(int)(me.acc_rot / INPUT_ROTARY_DIVISOR));
        me.acc_rot -= INPUT_ROTARY_DIVISOR * (me.acc_rot / INPUT_ROTARY_DIVISOR);
    }
}

static void input_event(uint32_t type, void *arg)
{
    switch (type)
    {
    case EVENT_BUTTON_PRESS:
    {
        input_button_t but = (input_button_t)arg;
        if (but >= _INPUT_BUTTON_COUNT)
            break;
        me.button_mask |= 1 << but;
        me.button_press_timestamp[but] = sys_get_up_second();
        break;
    }
    case EVENT_BUTTON_RELEASE:
    {
        input_button_t but = (input_button_t)arg;
        if (but >= _INPUT_BUTTON_COUNT)
            break;
        if (me.button_mask & (1 << but))
        {
            me.button_mask &= ~(1 << but);
            event_add(&me.ev_button, EVENT_UI_PRESS, (void *)but);
        }
    }
    case EVENT_SECOND_TICK:
    {
        uint32_t now_s = (uint32_t)arg;
        if (me.button_mask == 0)
            break;
        for (int i = 0; i < _INPUT_BUTTON_COUNT; i++)
        {
            if (me.button_mask & (1 << i) && now_s - me.button_press_timestamp[i] > INPUT_LONG_PRESS_SEC)
            {
                me.button_mask &= ~(1 << i);
                event_add(&me.ev_button, EVENT_UI_PRESSHOLD, i);
            }
        }
        break;
    }
    default:
        break;
    }
}
EVENT_HANDLER(input_event);

void TIM2_IRQHandler(void);
void TIM2_IRQHandler(void)
{
    // rotary interrupt
    if (LL_TIM_IsActiveFlag_CC1(TIM2))
    {
        LL_TIM_ClearFlag_CC1(TIM2);
    }

    if (LL_TIM_IsActiveFlag_CC2(TIM2))
    {
        LL_TIM_ClearFlag_CC2(TIM2);
    }
}

void EXTI4_IRQHandler(void);
void EXTI4_IRQHandler(void)
{
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_4))
    {
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_4);
        if (LL_GPIO_IsInputPinSet(GPIOA, LL_GPIO_PIN_4))
        {
            event_add_specific(&me.ev_hw_button, EVENT_BUTTON_RELEASE, INPUT_BUTTON_ROTARY, input_event);
        }
        else
        {
            event_add_specific(&me.ev_hw_button, EVENT_BUTTON_PRESS, INPUT_BUTTON_ROTARY, input_event);
        }
    }
}