CFILES += $(modules_dir)/cli/cli.c
INCLUDE += $(modules_dir)/cli

# make sure this is put last
ifeq "$(ARCH)" "pc"
LINKER_FILES += $(modules_dir)/cli/cli_pc.ld
else
ifeq "$(FAMILY)" "msp430"
LINKER_FILES += $(modules_dir)/cli/cli_msp430.ld
else
ifeq "$(FAMILY)" "stm32f1"
LINKER_FILES += $(modules_dir)/cli/cli_stm.ld
else
LINKER_FILES += $(modules_dir)/cli/cli.ld
endif
endif
endif
