OPENOCD_DEBUGGER ?= stlink-v2
OPENOCD_TARGET_FILE = target/stm32f1x.cfg
include $(arch_dir)/stm-flash-common.mk
