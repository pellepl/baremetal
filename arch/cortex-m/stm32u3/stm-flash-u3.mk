ifndef NO_OPENOCD
OPENOCD_DEBUGGER ?= stlink-v3
OPENOCD_TARGET_FILE = target/stm32u5x.cfg
include $(arch_dir)/stm-flash-common.mk
else
include $(arch_dir)/stm-flash-cube.mk
endif
