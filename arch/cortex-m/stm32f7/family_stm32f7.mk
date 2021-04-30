# Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com)
# MIT License (see ./LICENSE)

proc_dir := $(family_dir)/$(PROC)
mdk_dir := $(family_dir)/mdk

CFILES += $(mdk_dir)/STM32F7xx/Source/Templates/system_stm32f7xx.c
INCLUDE += $(mdk_dir)/STM32F7xx/Include

ifeq "$(PROC)" "stm32f746"
CFLAGS += -DSTM32F746xx
else
$(error PROC is not defined correctly for arch $(ARCH), family $(FAMILY))
endif

proc-cc-flags += -mcpu=cortex-m7 -mthumb -mabi=aapcs -mfloat-abi=hard -mfpu=fpv5-d16

CFLAGS += $(proc-cc-flags)
ASMFLAGS += $(proc-cc-flags)
LIBS += -lgcc -lc -lnosys

ifndef NO_STM32_LL_DRIVER
STM_CFILES_LL = $(wildcard $(family_dir)/STM32F7xx_HAL_Driver/Src/*.c)
CFILES += $(filter-out $(wildcard $(family_dir)/STM32F7xx_HAL_Driver/Src/*template*), $(STM_CFILES_LL))
INCLUDE += $(family_dir)/STM32F7xx_HAL_Driver/Inc
ifndef NO_STM32_LL_DRIVER_FULL
CFLAGS += -DUSE_FULL_LL_DRIVER
endif
ifndef NO_STM32_HAL_DRIVER
STM_CFILES_HAL = $(wildcard $(family_dir)/STM32F7xx_HAL_Driver/Src/HAL/*.c)
CFILES += $(filter-out $(wildcard $(family_dir)/STM32F7xx_HAL_Driver/Src/HAL/*template*), $(STM_CFILES_HAL))
INCLUDE += $(family_dir)/STM32F7xx_HAL_Driver/Inc/HAL
endif

ifndef NO_SQUELCH_OF_HAL_WARNINGS
CFLAGS += \
	-Wno-inline \
	-Wno-cast-qual \
	-Wno-cast-align \
	-Wno-redundant-decls \
#	-Wno-expansion-to-defined \
	-Wno-missing-prototypes \
	-Wno-redundant-decls \
	-Wno-strict-prototypes \
	-Wno-discarded-qualifiers \
	-Wno-implicit-fallthrough \
	-Wno-missing-field-initializers
endif

endif

-include $(family_dir)/stm-flash-f7.mk
