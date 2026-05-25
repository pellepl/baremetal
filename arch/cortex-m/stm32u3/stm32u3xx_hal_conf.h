/**
  ******************************************************************************
  * @file    stm32u3xx_hal_conf.h
  * @brief   Minimal HAL configuration for LL-driver builds in baremetal.
  ******************************************************************************
  */

#ifndef __STM32U3xx_HAL_CONF_H
#define __STM32U3xx_HAL_CONF_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(HSE_VALUE)
#define HSE_VALUE (32000000U)
#endif

#if !defined(HSE_STARTUP_TIMEOUT)
#define HSE_STARTUP_TIMEOUT (100U)
#endif

#if !defined(HSI_VALUE)
#define HSI_VALUE (16000000U)
#endif

#if !defined(HSI48_VALUE)
#define HSI48_VALUE (48000000U)
#endif

#if !defined(LSI_VALUE)
#define LSI_VALUE (32000U)
#endif

#if !defined(LSI_STARTUP_TIMEOUT)
#define LSI_STARTUP_TIMEOUT (130U)
#endif

#if !defined(LSE_VALUE)
#define LSE_VALUE (32768U)
#endif

#if !defined(LSE_STARTUP_TIMEOUT)
#define LSE_STARTUP_TIMEOUT (5000U)
#endif

#if !defined(MSIRC0_VALUE)
#define MSIRC0_VALUE (96000000U)
#endif

#if !defined(MSIRC1_VALUE)
#define MSIRC1_VALUE (24000000U)
#endif

#if !defined(EXTERNAL_SAI1_CLOCK_VALUE)
#define EXTERNAL_SAI1_CLOCK_VALUE (48000U)
#endif

#define VDD_VALUE         (3300U)
#define TICK_INT_PRIORITY ((uint32_t)((1U << __NVIC_PRIO_BITS) - 1U))
#define USE_RTOS          0U
#define PREFETCH_ENABLE   1U
#define USE_SPI_CRC       1U
#define USE_SD_TRANSCEIVER 0U

#define USE_HAL_ADC_REGISTER_CALLBACKS       0U
#define USE_HAL_CCB_REGISTER_CALLBACKS       0U
#define USE_HAL_COMP_REGISTER_CALLBACKS      0U
#define USE_HAL_CORTEX_REGISTER_CALLBACKS    0U
#define USE_HAL_CRC_REGISTER_CALLBACKS       0U
#define USE_HAL_CRYP_REGISTER_CALLBACKS      0U
#define USE_HAL_DAC_REGISTER_CALLBACKS       0U
#define USE_HAL_DMA_REGISTER_CALLBACKS       0U
#define USE_HAL_EXTI_REGISTER_CALLBACKS      0U
#define USE_HAL_FDCAN_REGISTER_CALLBACKS     0U
#define USE_HAL_FLASH_REGISTER_CALLBACKS     0U
#define USE_HAL_GPIO_REGISTER_CALLBACKS      0U
#define USE_HAL_GTZC_REGISTER_CALLBACKS      0U
#define USE_HAL_HASH_REGISTER_CALLBACKS      0U
#define USE_HAL_HCD_REGISTER_CALLBACKS       0U
#define USE_HAL_HSP_REGISTER_CALLBACKS       0U
#define USE_HAL_I2C_REGISTER_CALLBACKS       0U
#define USE_HAL_I3C_REGISTER_CALLBACKS       0U
#define USE_HAL_ICACHE_REGISTER_CALLBACKS    0U
#define USE_HAL_IRDA_REGISTER_CALLBACKS      0U
#define USE_HAL_IWDG_REGISTER_CALLBACKS      0U
#define USE_HAL_LPTIM_REGISTER_CALLBACKS     0U
#define USE_HAL_MDF_REGISTER_CALLBACKS       0U
#define USE_HAL_MMC_REGISTER_CALLBACKS       0U
#define USE_HAL_OPAMP_REGISTER_CALLBACKS     0U
#define USE_HAL_PCD_REGISTER_CALLBACKS       0U
#define USE_HAL_PKA_REGISTER_CALLBACKS       0U
#define USE_HAL_PWR_REGISTER_CALLBACKS       0U
#define USE_HAL_RAMCFG_REGISTER_CALLBACKS    0U
#define USE_HAL_RCC_REGISTER_CALLBACKS       0U
#define USE_HAL_RNG_REGISTER_CALLBACKS       0U
#define USE_HAL_RTC_REGISTER_CALLBACKS       0U
#define USE_HAL_SAI_REGISTER_CALLBACKS       0U
#define USE_HAL_SD_REGISTER_CALLBACKS        0U
#define USE_HAL_SMARTCARD_REGISTER_CALLBACKS 0U
#define USE_HAL_SMBUS_REGISTER_CALLBACKS     0U
#define USE_HAL_SPI_REGISTER_CALLBACKS       0U
#define USE_HAL_TIM_REGISTER_CALLBACKS       0U
#define USE_HAL_TSC_REGISTER_CALLBACKS       0U
#define USE_HAL_UART_REGISTER_CALLBACKS      0U
#define USE_HAL_USART_REGISTER_CALLBACKS     0U
#define USE_HAL_WWDG_REGISTER_CALLBACKS      0U
#define USE_HAL_XSPI_REGISTER_CALLBACKS      0U

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
