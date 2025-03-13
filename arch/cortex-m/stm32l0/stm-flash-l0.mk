OPENOCD_DEBUGGER ?= stlink
OPENOCD_TARGET_FILE = target/stm32l0.cfg
include $(arch_dir)/stm-flash-common.mk
