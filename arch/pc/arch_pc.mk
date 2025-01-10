# Copyright (c) 2023 Peter Andersson (pelleplutt1976<at>gmail.com)
# MIT License (see ./LICENSE)

family_dir := $(arch_dir)/$(FAMILY)
ifeq "$(FAMILY)" "sandbox"
else
$(error FAMILY is not defined correctly for arch $(ARCH))
endif
include $(family_dir)/family_$(FAMILY).mk

ifndef NO_LD_GC_SECTIONS
# keep every function in a separate section, this allows linker to discard unused ones
CFLAGS += -ffunction-sections -fdata-sections
# let linker dump unused sections
LDFLAGS += --gc-sections
endif

CFLAGS += -I/usr/include

CFLAGS += -DARCH_PC=1

-include $(arch_dir)/pc-flash.mk
