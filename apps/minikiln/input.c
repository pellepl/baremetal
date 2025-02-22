#include "minio.h"
#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_exti.h"
#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_tim.h"
#include "input.h"

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

int16_t input_rot_read(void)
{
    return (int16_t)LL_TIM_GetCounter(TIM2);
}

void input_rot_reset(void)
{
    LL_TIM_SetCounter(TIM2, 0);
}

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
void EXTI4_IRQHandler(void) {
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_4)) {
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_4);
        if (LL_GPIO_IsInputPinSet(GPIOA, LL_GPIO_PIN_4)) {
            // Rising edge detected
            printf("release\n");
        } else {
            // Falling edge detected
            printf("press\n");
        }
    }
}