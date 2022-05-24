# Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com)
# MIT License (see ./LICENSE)

proc_dir := $(family_dir)/$(PROC)
mdk_dir := $(family_dir)/mdk
cc-flags-nofpu = -mfloat-abi=soft
cc-flags-fpu = -mfloat-abi=hard -mfpu=fpv4-sp-d16

PROC-POSTFIX ?= _xxaa

ifeq "$(PROC)" "nrf52810"
SDK_PROC = nrf52810
CFLAGS += -DNRF52810_XXAA
CFILES-system = $(mdk_dir)/system_nrf52810.c
nrf52-cc-flags += $(cc-flags-nofpu)
else ifeq "$(PROC)" "nrf52811"
SDK_PROC = nrf52811
CFLAGS += -DNRF52811_XXAA
CFILES-system = $(mdk_dir)/system_nrf52811.c
nrf52-cc-flags += $(cc-flags-nofpu)
else ifeq "$(PROC)" "nrf52832"
SDK_PROC = nrf52832
CFLAGS += -DNRF52832_XXAA
CFILES-system = $(mdk_dir)/system_nrf52.c
nrf52-cc-flags += $(cc-flags-fpu)
else ifeq "$(PROC)" "nrf52833"
SDK_PROC = nrf52833
CFLAGS += -DNRF52833_XXAA
CFILES-system = $(mdk_dir)/system_nrf52833.c
nrf52-cc-flags += $(cc-flags-fpu)
else ifeq "$(PROC)" "nrf52840"
SDK_PROC = nrf52840
CFLAGS += -DNRF52840_XXAA
CFILES-system = $(mdk_dir)/system_nrf52840.c
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

ifeq "$(CONFIG_NRF_FORCE_INCLUDE_SYSTEM_STARTUP)" "1"
CFILES += $(CFILES-system)
else
  ifeq "$(CONFIG_NRF_SDK)" ""
  # if there is an SDK involved, do not use our own system initializer
  CFILES += $(CFILES-system)
  endif
endif

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

ifneq "$(CONFIG_NRF_SDK)" ""
NRF_SDK_PATH := $(family_dir)/vendor/sdk/$(CONFIG_NRF_SDK)
ifeq "$(wildcard $(NRF_SDK_PATH)/*)" ""
$(error NRF SDK path seems wrong, "$(NRF_SDK_PATH)". Please check CONFIG_NRF_SDK)
endif
CFLAGS += -DCONFIG_NRF_SDK="$(CONFIG_NRF_SDK)"
INCLUDE += $(NRF_SDK_PATH)/config/$(SDK_PROC)/config
LDFLAGS += -L$(NRF_SDK_PATH)/modules/nrfx/mdk -Lapps/$(APP)
LINKER_FILE ?= $(NRF_SDK_PATH)/config/$(SDK_PROC)/armgcc/generic_gcc_nrf52.ld
#LINKER_FILE ?= $(NRF_SDK_PATH)/modules/nrfx/mdk/$(SDK_PROC)$(SDK_PROC-POSTFIX).ld
endif

ifneq "$(CONFIG_NRF_SOFTDEVICE)" ""
NRF_SOFTDEVICE_PATH := $(family_dir)/vendor/softdevice/$(CONFIG_NRF_SOFTDEVICE)
NRF_SOFTDEVICE_HEX_PATH := $(wildcard $(NRF_SOFTDEVICE_PATH)/*.hex)
ifeq "$(NRF_SOFTDEVICE_HEX_PATH)" ""
$(error No softdevice hex file found under "$(NRF_SOFTDEVICE_PATH)". Please check CONFIG_NRF_SOFTDEVICE)
endif
CFLAGS += -DCONFIG_NRF_SOFTDEVICE="$(CONFIG_NRF_SOFTDEVICE)"
endif

INCLUDE += $(family_dir)/hal

-include $(family_dir)/../nrf-flash.mk

