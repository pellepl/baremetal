# Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com)
# MIT License (see ./LICENSE)

proc_dir := $(family_dir)/$(PROC)
mdk_dir := $(family_dir)/mdk
cc-flags-nofpu = -mfloat-abi=soft
cc-flags-fpu = -mfloat-abi=hard

ifndef NO_STM32_TEMPLATE_SYSTEM_FILE
CFILES += $(mdk_dir)/STM32F1xx/Source/Templates/system_stm32f1xx.c
endif
INCLUDE += $(mdk_dir)/STM32F1xx/Include

ifeq "$(PROC)" "stm32f103x4"
CFLAGS += -DSTM32F103x4
proc-cc-flags += $(cc-flags-nofpu)
else ifeq "$(PROC)" "stm32f103x6"
CFLAGS += -DSTM32F103x6
proc-cc-flags += $(cc-flags-nofpu)
else ifeq "$(PROC)" "stm32f103x8"
CFLAGS += -DSTM32F103x6
proc-cc-flags += $(cc-flags-nofpu)
else ifeq "$(PROC)" "stm32f103xB"
CFLAGS += -DSTM32F103xB
proc-cc-flags += $(cc-flags-nofpu)
else ifeq "$(PROC)" "stm32f103xC"
CFLAGS += -DSTM32F103xC
proc-cc-flags += $(cc-flags-nofpu)
else ifeq "$(PROC)" "stm32f103xD"
CFLAGS += -DSTM32F103xD
proc-cc-flags += $(cc-flags-nofpu)
else ifeq "$(PROC)" "stm32f103xE"
CFLAGS += -DSTM32F103xE
proc-cc-flags += $(cc-flags-nofpu) -Wno-inline
else ifeq "$(PROC)" "stm32f103xF"
CFLAGS += -DSTM32F103xF
proc-cc-flags += $(cc-flags-nofpu)
else ifeq "$(PROC)" "stm32f103xG"
CFLAGS += -DSTM32F103xG
proc-cc-flags += $(cc-flags-nofpu)
else
$(error PROC is not defined correctly for arch $(ARCH), family $(FAMILY))
endif

proc-cc-flags += -mcpu=cortex-m3 -mthumb -mabi=aapcs

CFLAGS += $(proc-cc-flags)
ASMFLAGS += $(proc-cc-flags)
LIBS += -lgcc -lc -lnosys

ifndef NO_STM32_LL_DRIVER
CFILES += $(wildcard $(family_dir)/STM32F1xx_HAL_Driver/Src/*.c)
INCLUDE += $(family_dir)/STM32F1xx_HAL_Driver/Inc
ifndef NO_STM32_LL_DRIVER_FULL
CFLAGS += -DUSE_FULL_LL_DRIVER
endif
endif

-include $(family_dir)/stm-flash-f1.mk

