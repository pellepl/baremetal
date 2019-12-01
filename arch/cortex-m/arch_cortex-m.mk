# Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com)
# MIT License (see ./LICENSE)

# For arch cortex-m, C-flag _CORTEX_CORE_HEADER must be defined and define
# path to the a header including proper CMSIS header, core_cmX.h, thus defining
# cortex-m variant.
family_dir := $(arch_dir)/$(FAMILY)
ifeq "$(FAMILY)" "nrf52"
CFLAGS += -D_CORTEX_CORE_HEADER="\"nrf.h\""
else ifeq "$(FAMILY)" "stm32f1"
CFLAGS += -D_CORTEX_CORE_HEADER="\"stm32f1xx.h\""
else
$(error FAMILY is not defined correctly for arch $(ARCH))
endif
include $(family_dir)/family_$(FAMILY).mk

# CMSIS config
CFLAGS += -D__PROGRAM_START
INCLUDE += $(arch_dir)/cmsis/CMSIS/Core/Include
INCLUDE += $(arch_dir)/cmsis/CMSIS/DSP/Include

CFLAGS += -fno-builtin -fshort-enums -fno-strict-aliasing
ifndef NO_LD_GC_SECTIONS
# keep every function in a separate section, this allows linker to discard unused ones
CFLAGS += -ffunction-sections -fdata-sections
# let linker dump unused sections
LDFLAGS += --gc-sections
endif
ifndef NO_NEWLIB_NANO
# use newlib in nano version
CFLAGS += --specs=nano.specs
endif
