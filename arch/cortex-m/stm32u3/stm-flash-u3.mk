OPENOCD_DEBUGGER ?= stlink
OPENOCD_TARGET_FILE = target/stm32u3x.cfg
include $(arch_dir)/stm-flash-common.mk
