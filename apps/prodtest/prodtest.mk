TARGETNAME := $(APP)

# this app must run on an nrf528xx

ARCH := cortex-m
FAMILY := nrf52
PROC := nrf528xx

SDK_PROC = nrf52840

LIBS += -lm

CONFIG_GPIO := 1
CONFIG_GPIO_IRQ := 1
CONFIG_UART := 1
CONFIG_MINIO := 1
CONFIG_CLI := 1
CONFIG_RINGBUFFER := 1
CONFIG_EVENTQUEUE := 1
CFLAGS += -DEVENTQ_CUSTOM_INC="events.h"
CONFIG_NRF52_TEST_BLE_DTM := 1

INCLUDE += apps/$(APP)
CFILES += $(wildcard apps/$(APP)/*.c)

INCLUDE += apps/$(APP)/targets
CFILES += $(wildcard apps/$(APP)/targets/*.c)

INCLUDE += apps/$(APP)/drivers
CFILES += $(wildcard apps/$(APP)/drivers/*.c)

CFLAGS += -Wno-address-of-packed-member

LINKER_FILES += apps/$(APP)/targets/linker_targets.ld
