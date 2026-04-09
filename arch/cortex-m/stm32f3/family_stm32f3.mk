# Copyright (c) 2026 Peter Andersson (pelleplutt1976<at>gmail.com)
# MIT License (see ./LICENSE)

proc_dir := $(family_dir)/$(PROC)
mdk_dir := $(family_dir)/mdk
cc-flags-fpu = -mfloat-abi=hard -mfpu=fpv4-sp-d16

ifndef NO_STM32_TEMPLATE_SYSTEM_FILE
CFILES += $(mdk_dir)/STM32F3xx/Source/Templates/system_stm32f3xx.c
endif
INCLUDE += $(mdk_dir)/STM32F3xx/Include

ifeq "$(PROC)" "stm32f303x8"
CFLAGS += -DSTM32F303x8
proc-cc-flags += $(cc-flags-fpu)
else ifeq "$(PROC)" "stm32f334x8"
CFLAGS += -DSTM32F334x8
proc-cc-flags += $(cc-flags-fpu)
else
$(error PROC is not defined correctly for arch $(ARCH), family $(FAMILY))
endif

proc-cc-flags += -mcpu=cortex-m4 -mthumb -mabi=aapcs

CFLAGS += $(proc-cc-flags)
ASMFLAGS += $(proc-cc-flags)
LIBS += -lgcc -lc -lnosys

ifndef NO_STM32_LL_DRIVER
CFILES += \
	$(family_dir)/STM32F3xx_HAL_Driver/Src/stm32f3xx_ll_gpio.c \
	$(family_dir)/STM32F3xx_HAL_Driver/Src/stm32f3xx_ll_rcc.c \
	$(family_dir)/STM32F3xx_HAL_Driver/Src/stm32f3xx_ll_usart.c \
	$(family_dir)/STM32F3xx_HAL_Driver/Src/stm32f3xx_ll_utils.c
INCLUDE += $(family_dir)/STM32F3xx_HAL_Driver/Inc
ifndef NO_STM32_LL_DRIVER_FULL
CFLAGS += -DUSE_FULL_LL_DRIVER
endif
endif

ifndef NO_SQUELCH_OF_HAL_WARNINGS
CFLAGS += \
	-Wno-inline \
	-Wno-cast-align \
	-Wno-redundant-decls \
	-Wno-expansion-to-defined \
	-Wno-missing-prototypes \
	-Wno-strict-prototypes \
	-Wno-discarded-qualifiers \
	-Wno-implicit-fallthrough \
	-Wno-missing-field-initializers
endif

-include $(family_dir)/stm-flash-f3.mk
