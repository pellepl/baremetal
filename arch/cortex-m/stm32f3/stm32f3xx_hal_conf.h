/**
  ******************************************************************************
  * @file    stm32f3xx_hal_conf.h
  * @brief   Minimal HAL configuration for LL-driver builds in baremetal.
  ******************************************************************************
  */

#ifndef __STM32F3xx_HAL_CONF_H
#define __STM32F3xx_HAL_CONF_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(HSE_VALUE)
#define HSE_VALUE (8000000U)
#endif

#if !defined(HSE_STARTUP_TIMEOUT)
#define HSE_STARTUP_TIMEOUT (100U)
#endif

#if !defined(HSI_VALUE)
#define HSI_VALUE (8000000U)
#endif

#if !defined(HSI_STARTUP_TIMEOUT)
#define HSI_STARTUP_TIMEOUT (5000U)
#endif

#if !defined(LSI_VALUE)
#define LSI_VALUE (40000U)
#endif

#if !defined(LSE_VALUE)
#define LSE_VALUE (32768U)
#endif

#if !defined(LSE_STARTUP_TIMEOUT)
#define LSE_STARTUP_TIMEOUT (5000U)
#endif

#if !defined(EXTERNAL_CLOCK_VALUE)
#define EXTERNAL_CLOCK_VALUE (8000000U)
#endif

#define VDD_VALUE                (3300U)
#define TICK_INT_PRIORITY        ((uint32_t)((1U << __NVIC_PRIO_BITS) - 1U))
#define USE_RTOS                 0U
#define PREFETCH_ENABLE          1U
#define INSTRUCTION_CACHE_ENABLE 0U
#define DATA_CACHE_ENABLE        0U
#define USE_SPI_CRC              1U

#define USE_HAL_ADC_REGISTER_CALLBACKS       0U
#define USE_HAL_CAN_REGISTER_CALLBACKS       0U
#define USE_HAL_COMP_REGISTER_CALLBACKS      0U
#define USE_HAL_CEC_REGISTER_CALLBACKS       0U
#define USE_HAL_DAC_REGISTER_CALLBACKS       0U
#define USE_HAL_SRAM_REGISTER_CALLBACKS      0U
#define USE_HAL_SMBUS_REGISTER_CALLBACKS     0U
#define USE_HAL_SDADC_REGISTER_CALLBACKS     0U
#define USE_HAL_NAND_REGISTER_CALLBACKS      0U
#define USE_HAL_NOR_REGISTER_CALLBACKS       0U
#define USE_HAL_PCCARD_REGISTER_CALLBACKS    0U
#define USE_HAL_HRTIM_REGISTER_CALLBACKS     0U
#define USE_HAL_I2C_REGISTER_CALLBACKS       0U
#define USE_HAL_UART_REGISTER_CALLBACKS      0U
#define USE_HAL_USART_REGISTER_CALLBACKS     0U
#define USE_HAL_IRDA_REGISTER_CALLBACKS      0U
#define USE_HAL_SMARTCARD_REGISTER_CALLBACKS 0U
#define USE_HAL_WWDG_REGISTER_CALLBACKS      0U
#define USE_HAL_OPAMP_REGISTER_CALLBACKS     0U
#define USE_HAL_RTC_REGISTER_CALLBACKS       0U
#define USE_HAL_SPI_REGISTER_CALLBACKS       0U
#define USE_HAL_I2S_REGISTER_CALLBACKS       0U
#define USE_HAL_TIM_REGISTER_CALLBACKS       0U
#define USE_HAL_TSC_REGISTER_CALLBACKS       0U
#define USE_HAL_PCD_REGISTER_CALLBACKS       0U

#ifdef USE_FULL_ASSERT
#define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
void assert_failed(uint8_t *file, uint32_t line);
#else
#define assert_param(expr) ((void)0U)
#endif

#ifdef __cplusplus
}
#endif

#endif
