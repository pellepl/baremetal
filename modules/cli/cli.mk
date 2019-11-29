CFILES += $(modules_dir)/cli/cli.c
INCLUDE += $(modules_dir)/cli

# make sure this is put last
ifeq "$(FAMILY)" "msp430"
LDFLAGS_LATE += --script=$(modules_dir)/cli/cli_msp430.ld
else
LDFLAGS_LATE += --script=$(modules_dir)/cli/cli.ld
endif