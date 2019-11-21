-include ../local.mk

BOARD ?= pca10040

board_dir := board/$(BOARD)
-include $(board_dir)/board_$(BOARD).mk

TARGET_DIR ?= build/$(TARGETNAME)-$(BOARD)-$(PROC)
XCC ?= $(toolchain)gcc
XAS ?= $(XCC)
XLD ?= $(toolchain)ld
XOBJCOPY ?= $(toolchain)objcopy
MKDIR ?= mkdir -p
RM ?= rm -f
CC_OPTIMISATION ?= s
VERBOSE ?= @

v = $(VERBOSE)
toolchain = $(TOOLCHAIN_DIR)/bin/$(CROSS_COMPILE)
target = $(TARGET_DIR)/$(TARGETNAME)

all: $(target).hex

ifeq "$(wildcard $(board_dir)/board.h)" ""
$(error BOARD seems to be set incorrectly, cannot find board definition file $(board_dir)/board.h)
endif

ifeq "$(TARGETNAME)" ""
$(error TARGETNAME must be defined)
endif

arch_dir := arch/$(ARCH)
ifeq "$(wildcard $(arch_dir)/arch_$(ARCH).mk)" ""
$(error ARCH seems to be set incorrectly, cannot find arch make file $(arch_dir)/arch_$(ARCH).mk))
endif
include $(arch_dir)/arch_$(ARCH).mk
include arch/arch.mk
modules_dir := modules
include modules/modules.mk

libgcc_dir := $(dir $(shell find $(TOOLCHAIN_DIR) -name "libgcc.a" | head -n 1))
libc_dir := $(dir $(shell find $(TOOLCHAIN_DIR) -name "libc.a" | head -n 1))

ifneq "$(libgcc_dir)" ""
LIBS += -L $(libgcc_dir)
endif
ifneq "$(libc_dir)" ""
LIBS += -L $(libc_dir)
endif

LINKER_FILE ?= $(proc_dir)/$(PROC).ld
INCLUDE += $(board_dir) board/ $(proc_dir) $(family_dir) $(arch_dir) arch/
CFILES += board/board_common.c
ifneq "$(wildcard $(board_dir)/board_$(BOARD).c)" ""
CFILES += $(board_dir)/board_$(BOARD).c
endif

objfiles := $(CFILES:%.c=$(TARGET_DIR)/%.o)
asobjfiles := $(ASFILES:%.S=$(TARGET_DIR)/%.o)
depfiles := $(CFILES:%.c=$(TARGET_DIR)/%.d)
iflags := $(foreach dir,$(INCLUDE),-I$(dir))
ifndef NO_DEBUG_SYMBOLS
CFLAGS += -g
LDFLAGS += -g
endif
ifndef NO_WARNINGS
CFLAGS += -Wall -Wno-format-y2k -W -Wstrict-prototypes -Wmissing-prototypes \
-Wpointer-arith -Wreturn-type -Wcast-qual -Wwrite-strings -Wswitch \
-Wshadow -Wcast-align -Wchar-subscripts -Winline -Wnested-externs \
-Wredundant-decls -Wno-unused-parameter
endif
CFLAGS += -O$(CC_OPTIMISATION)

ifndef NO_BUILD_INFO_GIT
CFLAGS += -DBUILD_INFO_GIT_COMMIT="$(shell git rev-parse --short HEAD)"
CFLAGS += -DBUILD_INFO_GIT_BRANCH="$(shell git rev-parse --abbrev-ref HEAD)$(shell git diff-index --quiet HEAD -- || echo -dirty)"
CFLAGS += -DBUILD_INFO_GIT_TAG="$(shell git decribe 2> /dev/null || echo '(tagless)')"
endif
ifndef NO_BUILD_INFO_HOST
CFLAGS += -DBUILD_INFO_HOST_NAME="$(shell hostname)"
CFLAGS += -DBUILD_INFO_HOST_WHO="$(shell whoami)"
CFLAGS += -DBUILD_INFO_HOST_WHEN_DATE="$(shell date +%Y%m%d)"
CFLAGS += -DBUILD_INFO_HOST_WHEN_TIME="$(shell date +%H%M%S)"
CFLAGS += -DBUILD_INFO_HOST_WHEN_EPOCH="$(shell date +%s)"
endif
ifndef NO_BUILD_INFO_TARGET
CFLAGS += -DBUILD_INFO_TARGET_NAME="$(TARGETNAME)"
CFLAGS += -DBUILD_INFO_TARGET_ARCH="$(ARCH)"
CFLAGS += -DBUILD_INFO_TARGET_FAMILY="$(FAMILY)"
CFLAGS += -DBUILD_INFO_TARGET_PROC="$(PROC)"
CFLAGS += -DBUILD_INFO_TARGET_BOARD="$(BOARD)"
endif
ifndef NO_BUILD_INFO_COMPILER
CFLAGS += -DBUILD_INFO_CC_MACHINE="$(shell $(XCC) -dumpmachine)"
CFLAGS += -DBUILD_INFO_CC_VERSION="$(shell $(XCC) -dumpversion)"
endif

ifeq (,$(findstring $(MAKECMDGOALS),clean info))
-include $(depfiles)
endif

$(target).hex: $(target).elf
	$(v)$(XOBJCOPY) -O ihex $< $@
	$(v)$(XOBJCOPY) -O binary $< $(target).bin

$(target).elf: $(objfiles) $(asobjfiles)
	@echo "LD\t$@"
	$(v)$(XLD) $(LDFLAGS) -T $(LINKER_FILE) -Map $(target).map -o $@ $(objfiles) $(asobjfiles) $(LIBS)
	$(v)size $@

$(objfiles): $(TARGET_DIR)/%.o:%.c
	@echo "CC\t$@"
	$(v)$(MKDIR) $(@D)
	$(v)$(XCC) $(CFLAGS) $(iflags) -c -o $@ $< $(LIBS)

$(asobjfiles): $(TARGET_DIR)/%.o:%.S
	@echo "AS\t$@"
	$(v)$(MKDIR) $(@D)
	$(v)$(XAS) $(CFLAGS) $(iflags) -c -o $@ $< $(LIBS)

$(depfiles): $(TARGET_DIR)/%.d:%.c
	@echo "DEP\t$@"
	$(v)$(RM) $@; \
	$(MKDIR) $(@D); \
	$(XCC) $(CFLAGS) $(iflags) -MM -MG -MF $@ $<; \
	sed -i 's,$(notdir $(basename $*)).o[ :]*, $(TARGET_DIR)/$(basename $^).o: ,g' $@

clean:
	$(v)$(RM) -r $(TARGET_DIR)

info:
	$(info TARGET      $(TARGETNAME))
	$(info BOARD       $(BOARD))
	$(info ARCH        $(ARCH))
	$(info FAMILY      $(FAMILY))
	$(info PROC        $(PROC))
	$(info CFILES)
	$(foreach c,$(CFILES),$(info .           $(c)))
	$(info CFLAGS)
	$(foreach c,$(CFLAGS),$(info .           $(c)))
	$(info INCLUDE)
	$(foreach c,$(INCLUDE),$(info .           $(c)))
	$(info LD FILE     $(LINKER_FILE))
	$(info CC          $(XCC))
