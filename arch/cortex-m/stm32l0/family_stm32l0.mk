# Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com)
# MIT License (see ./LICENSE)

proc_dir := $(family_dir)/$(PROC)
mdk_dir := $(family_dir)/mdk
cc-flags-nofpu = -mfloat-abi=soft
cc-flags-fpu = -mfloat-abi=hard

ifndef NO_STM32_TEMPLATE_SYSTEM_FILE
CFILES += $(mdk_dir)/STM32L0xx/Source/Templates/system_stm32l0xx.c
endif
INCLUDE += $(mdk_dir)/STM32F0xx/Include

ifeq "$(PROC)" "stm32l053"
CFLAGS += -DSTM32L053
proc-cc-flags += $(cc-flags-nofpu)
else
$(error PROC is not defined correctly for arch $(ARCH), family $(FAMILY))
endif

proc-cc-flags += -mcpu=cortex-m0 -mthumb -mabi=aapcs

CFLAGS += $(proc-cc-flags)
ASMFLAGS += $(proc-cc-flags)
LIBS += -lgcc -lc -lnosys

ifndef NO_STM32_LL_DRIVER
CFILES += $(wildcard $(family_dir)/STM32L0xx_HAL_Driver/Src/*.c)
INCLUDE += $(family_dir)/STM32L0xx_HAL_Driver/Inc
ifndef NO_STM32_LL_DRIVER_FULL
CFLAGS += -DUSE_FULL_LL_DRIVER
endif
endif

-include $(family_dir)/stm-flash-l0.mk
