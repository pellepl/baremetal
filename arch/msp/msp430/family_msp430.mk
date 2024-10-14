# Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com)
# MIT License (see ./LICENSE)

proc_dir := $(family_dir)/$(PROC)
mdk_dir := $(family_dir)/mdk/include

ifeq "$(PROC)" "msp430g2553"
CFLAGS += -D__MSP430G2553__
CFLAGS += -mmcu=$(PROC)
msp_link_type := 430
else ifeq "$(PROC)" "msp430f4132"
CFLAGS += -D__MSP430F4132__
CFLAGS += -mmcu=$(PROC)
msp_link_type := 430
else
$(error PROC is not defined correctly for arch $(ARCH), family $(FAMILY))
endif

ifneq "$(GCC_AS_LD)" "1"

# following sequence was derived by linking the MSP with GCC instead of ld, 
# along with the -v flag, a stroke of luck, and a load of angst

NO_CRT0 := 1
msp_crt0_o := $(realpath $(shell find $(TOOLCHAIN_DIR) -name "crt0.o" | grep "$(msp_link_type)/crt0.o" | head -n 1))
ifeq "$(strip $(msp_crt0_o))" ""
$(error Could not find crt0.o for msp link type "$(msp_link_type)". Make sure msp_link_type is valid for PROC $(PROC) in $(family_dir)/family_$(FAMILY).mk \
 and that your TOOLCHAIN_DIR is set correctly ($(TOOLCHAIN_DIR)))
endif
INCLUDE += $(mdk_dir)
LDFLAGS += --library-path=$(mdk_dir)

msp_lib_path := $(dir $(shell find $(TOOLCHAIN_DIR)/lib -name "libgcc.a" | grep "$(msp_link_type)/libgcc.a" | head -n 1))
#OBJFILES_EXTRA_FIRST += $(msp_crt0_o)
NO_LINK_FILE := 1

LIBS += $(foreach path,$(dir $(msp_crt0_o)) $(msp_lib_path),-L$(path))
LIBS += -lmul_none
LIBS += -lc
LIBS += -lgcc
LIBS += -lcrt
LIBS += -lnosys
LINKER_FILE = $(mdk)/$(PROC).ld
endif
