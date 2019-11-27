proc_dir := $(family_dir)/$(PROC)
mdk_dir := $(family_dir)/mdk/include

ifeq "$(PROC)" "msp430g2553"
CFLAGS += -D__MSP430G2553__
CFLAGS += -mmcu=$(PROC)
else
$(error PROC is not defined correctly for arch $(ARCH), family $(FAMILY))
endif

INCLUDE += $(mdk_dir)
LDFLAGS += --library-path=$(mdk_dir)

# default to using gcc as linker, otherwise you're on your own with TIs toolchain
GCC_AS_LD ?= 1

ifeq "$(GCC_AS_LD)" "1"
  NO_CRT0 := 1
  LINKER_FILE :=
else
  LINKER_FILE = $(mdk)/$(PROC).ld
endif

