ARCH ?= cortex-m
FAMILY ?= stm32u3
PROC ?= stm32u385xx
# No main crystal on this devboard
#CFLAGS += -DHSE_VALUE=8000000
CFLAGS += -DDEFAULT_UART_BAUDRATE=UART_BAUDRATE_9600
