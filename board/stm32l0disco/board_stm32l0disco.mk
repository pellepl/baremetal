ARCH ?= cortex-m
FAMILY ?= stm32l0
PROC ?= stm32l053
# The main crystal is 8MHz
CFLAGS += -DHSE_VALUE=8000000
CFLAGS += -DDEFAULT_UART_BAUDRATE=UART_BAUDRATE_9600
