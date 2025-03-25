OPENOCD_DEBUGGER ?= stlink
OPENOCD_TARGET_FILE = target/stm32l0.cfg
EXTRA_DEBUG_COMMANDS = -ex "mon reset_config srst_only"
include $(arch_dir)/stm-flash-common.mk
