family_dir := $(arch_dir)/$(FAMILY)
ifeq "$(FAMILY)" "msp430"
else
$(error FAMILY is not defined correctly for arch $(ARCH))
endif
include $(family_dir)/family_$(FAMILY).mk

CFLAGS += -fno-builtin
CFLAGS += -fno-strict-aliasing
ifndef NO_LD_GC_SECTIONS
# keep every function in a separate section, this allows linker to discard unused ones
CFLAGS += -ffunction-sections -fdata-sections
# let linker dump unused sections
LDFLAGS += --gc-sections
endif
