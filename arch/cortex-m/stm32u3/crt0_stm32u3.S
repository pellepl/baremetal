/* Copyright (c) 2026 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#if defined(STM32U356xx)
#include "mdk/STM32U3xx/Source/Templates/gcc/startup_stm32u356xx.s"
#elif defined(STM32U366xx)
#include "mdk/STM32U3xx/Source/Templates/gcc/startup_stm32u366xx.s"
#elif defined(STM32U375xx)
#include "mdk/STM32U3xx/Source/Templates/gcc/startup_stm32u375xx.s"
#elif defined(STM32U385xx)
#include "mdk/STM32U3xx/Source/Templates/gcc/startup_stm32u385xx.s"
#elif defined(STM32U3B5xx)
#include "mdk/STM32U3xx/Source/Templates/gcc/startup_stm32u3b5xx.s"
#elif defined(STM32U3C5xx)
#include "mdk/STM32U3xx/Source/Templates/gcc/startup_stm32u3c5xx.s"
#else
#error Unsupported STM32U3 device for crt0_stm32u3.S
#endif
