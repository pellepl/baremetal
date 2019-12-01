# Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com)
# MIT License (see ./LICENSE)

proc_dir := $(family_dir)/$(PROC)
mdk_dir := $(family_dir)/mdk
cc-flags-nofpu = -mfloat-abi=soft
cc-flags-fpu = -mfloat-abi=hard -mfpu=fpv4-sp-d16

ifeq "$(PROC)" "nrf52810"
CFLAGS += -DNRF52810_XXAA
CFILES += $(mdk_dir)/system_nrf52810.c
nrf52-cc-flags += $(cc-flags-nofpu)
else ifeq "$(PROC)" "nrf52811"
CFLAGS += -DNRF52811_XXAA
CFILES += $(mdk_dir)/system_nrf52811.c
nrf52-cc-flags += $(cc-flags-nofpu)
else ifeq "$(PROC)" "nrf52832"
CFLAGS += -DNRF52832_XXAA
CFILES += $(mdk_dir)/system_nrf52.c
nrf52-cc-flags += $(cc-flags-fpu)
else ifeq "$(PROC)" "nrf52833"
CFLAGS += -DNRF52833_XXAA
CFILES += $(mdk_dir)/system_nrf52833.c
nrf52-cc-flags += $(cc-flags-fpu)
else ifeq "$(PROC)" "nrf52840"
CFLAGS += -DNRF52840_XXAA
CFILES += $(mdk_dir)/system_nrf52840.c
nrf52-cc-flags += $(cc-flags-fpu)
else ifeq "$(PROC)" "nrf528xx"
CFLAGS += -DNRF52840_XXAA
CFILES += $(family_dir)/nrf528xx/system_nrf528xx.c
nrf52-cc-flags += $(cc-flags-fpu)
else
$(error PROC is not defined correctly for arch $(ARCH), family $(FAMILY))
endif

INCLUDE += $(mdk_dir)

nrf52-cc-flags += -mcpu=cortex-m4 -mthumb -mabi=aapcs

CFLAGS += $(nrf52-cc-flags)
ASMFLAGS += $(nrf52-cc-flags)
LIBS += -lgcc -lc -lnosys

ifdef EXECUTE_IN_RAM
LINKER_FILE = $(proc_dir)/$(PROC)-exe-ram.ld
endif

ifeq "$(CONFIG_NRF52_TEST_RADIO)" "1"
CFLAGS += -DCONFIG_NRF52_TEST_RADIO=1
CFILES += $(wildcard $(family_dir)/test-radio/*.c)
INCLUDE += $(family_dir)/test-radio
endif

ifeq "$(CONFIG_NRF52_TEST_BLE_DTM)" "1"
CFLAGS += -DCONFIG_NRF52_TEST_BLE_DTM=1
CFILES += $(wildcard $(family_dir)/test-ble-dtm/*.c)
INCLUDE += $(family_dir)/test-ble-dtm
endif

-include $(family_dir)/nrf-flash.mk

