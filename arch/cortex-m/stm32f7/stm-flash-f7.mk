OPENOCD_DEBUGGER ?= stlink-v2-1
OPENOCD_TARGET_FILE = target/stm32f7x.cfg
include $(arch_dir)/stm-flash-common.mk
