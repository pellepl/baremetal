ARCH ?= cortex-m
FAMILY ?= stm32f1
PROC ?= stm32f103x8
# The main crystal is 8MHz
CFLAGS += -DHSE_VALUE=8000000
