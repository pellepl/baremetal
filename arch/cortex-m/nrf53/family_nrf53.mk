# Copyright (c) 2020 Peter Andersson (pelleplutt1976<at>gmail.com)
# MIT License (see ./LICENSE)

proc_dir := $(family_dir)/$(PROC)
mdk_dir := $(family_dir)/mdk
ifeq "$(PROC)" "nrf5340app"
CFLAGS += -DNRF5340_XXAA_APPLICATION
CFILES += $(mdk_dir)/system_nrf5340_application.c
else ifeq "$(PROC)" "nrf5340net"
CFLAGS += -DNRF5340_XXAA_NETWORK
CFILES += $(mdk_dir)/system_nrf5340_network.c
else
$(error PROC is not defined correctly for arch $(ARCH), family $(FAMILY))
endif

INCLUDE += $(mdk_dir)

nrf53-cc-flags += -mcpu=cortex-m33 -mthumb -mabi=aapcs -ffreestanding -fno-common

nrf53_start_func ?= main
ASMFLAGS += -D__START=$(nrf53_start_func)
CFLAGS += -D__START=$(nrf53_start_func)
CFLAGS += $(nrf53-cc-flags)
ASMFLAGS += $(nrf53-cc-flags)
LIBS += -lgcc -lc -lnosys

INCLUDE += $(family_dir)/hal

-include $(family_dir)/../nrf-flash.mk

