# Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com)
# MIT License (see ./LICENSE)

-include ../local.mk

BOARD ?= pca10040

board_dir := board/$(BOARD)
-include $(board_dir)/board_$(BOARD).mk

ifeq ($(OS),Windows_NT)
ECHO:=echo -e
exe:=.exe
else
ECHO:=echo
exe:=
endif

-include ../toolchain.mk

TARGET_DIR ?= build/$(TARGETNAME)-$(BOARD)-$(PROC)
XCC ?= $(toolchain)gcc$(exe)
XAS ?= $(XCC)
XLD ?= $(toolchain)ld$(exe)
XOBJCOPY ?= $(toolchain)objcopy$(exe)
XOBJDUMP ?= $(toolchain)objdump$(exe)
MKDIR ?= mkdir -p
RM ?= rm -f
CC_OPTIMISATION ?= s
VERBOSE ?= @

v = $(VERBOSE)
toolchain = $(TOOLCHAIN_DIR)/bin/$(CROSS_COMPILE)
target = $(TARGET_DIR)/$(TARGETNAME)

all:: $(target).hex

# sanity check environment

arch_dir := arch/$(ARCH)
ifndef NO_SANITY_CHECK

make_v := $(subst ., ,$(MAKE_VERSION))
ifeq ($(shell test $(word 1, $(make_v)) -lt 4; echo $$?),0)
$(warning ############################################################)
$(warning Your make reports version $(MAKE_VERSION))
$(warning Required version is at least 4.1)
$(warning MinGWs default make is normally too old.)
$(warning ############################################################)
$(error Old make)
endif

ifeq "$(wildcard $(XCC))" ""
$(warning ############################################################)
$(warning Cannot find a compiler. Make sure TOOLCHAIN_DIR and)
$(warning CROSS_COMPILE are set. See the README.)
$(warning "$(XCC)")
$(warning ############################################################)
NO_BUILD_INFO_COMPILER := 1
endif

ifeq "$(wildcard $(board_dir)/board.h)" ""
$(warning ############################################################)
$(warning BOARD seems to be set incorrectly, cannot find board)
$(warning definition file $(board_dir)/board.h)
$(warning ############################################################)
$(error Cannot find board.h for board $(BOARD))
endif

ifeq "$(TARGETNAME)" ""
$(warning ############################################################)
$(warning TARGETNAME must be defined. Is APP set correctly ($(APP))?)
$(warning ############################################################)
$(error TARGETNAME undefined)
endif

ifeq "$(wildcard $(arch_dir)/arch_$(ARCH).mk)" ""
$(warning ############################################################)
$(warning ARCH seems to be set incorrectly, cannot find arch make file)
$(warning $(arch_dir)/arch_$(ARCH).mk)
$(warning ############################################################)
$(error Cannot find makefile for arch $(ARCH))
endif
endif

ifndef NO_WARNINGS
CFLAGS += -Wall -Wno-format-y2k -W -Wstrict-prototypes -Wmissing-prototypes \
-Wpointer-arith -Wreturn-type -Wcast-qual -Wwrite-strings -Wswitch \
-Wshadow -Wcast-align -Wchar-subscripts -Winline -Wnested-externs \
-Wredundant-decls -Wno-unused-parameter
endif

# include arch and modules
include $(arch_dir)/arch_$(ARCH).mk
include arch/arch.mk
modules_dir := modules
include modules/modules.mk

GCC_AS_LD ?= 1

ifeq "$(GCC_AS_LD)" "1"
  XLD := $(XCC)
else
  ifeq "$(XLD)" "$(XCC)"
    GCC_AS_LD := 1
  else
    GCC_AS_LD := 0
  endif
endif

ifeq "$(GCC_AS_LD)" "0"
  # see if we find libc and libgcc
  libgcc_dir := $(dir $(shell find $(TOOLCHAIN_DIR) -name "libgcc.a" | head -n 1))
  libc_dir := $(dir $(shell find $(TOOLCHAIN_DIR) -name "libc.a" | head -n 1))

  ifneq "$(libgcc_dir)" ""
    LIBS += -L $(libgcc_dir)
  endif
  ifneq "$(libc_dir)" ""
    LIBS += -L $(libc_dir)
  endif
endif

# set default linker file
LINKER_FILE ?= $(proc_dir)/$(PROC).ld
# general include directories in specific-first, general-last order
INCLUDE += $(board_dir) board/ $(proc_dir) $(family_dir) $(arch_dir) arch/
# board compile files
CFILES += board/board_common.c
ifneq "$(wildcard $(board_dir)/board_$(BOARD).c)" ""
CFILES += $(board_dir)/board_$(BOARD).c
endif

# pass build info to compilation entities
ifeq "$(wildcard .git)" ""
NO_BUILD_INFO_GIT := 1
endif
ifndef NO_BUILD_INFO_GIT
build_info_git_commit := "$(shell git rev-parse --short HEAD)"
build_info_git_branch := "$(shell git rev-parse --abbrev-ref HEAD)$(shell git diff-index --quiet HEAD -- || echo -dirty)"
build_info_git_tag := "$(shell git decribe 2> /dev/null || echo '(tagless)')"
CFLAGS += -DBUILD_INFO_GIT_COMMIT=$(build_info_git_commit)
CFLAGS += -DBUILD_INFO_GIT_BRANCH=$(build_info_git_branch)
CFLAGS += -DBUILD_INFO_GIT_TAG=$(build_info_git_tag)
endif
ifndef NO_BUILD_INFO_HOST
host_info_name := "$(shell hostname)"
host_info_who := "$(shell whoami)"
host_info_when_date := "$(shell date +%Y%m%d)"
host_info_when_time := "$(shell date +%H%M%S)"
host_info_when_epoch := "$(shell date +%s)"
CFLAGS += -DBUILD_INFO_HOST_NAME=$(host_info_name)
CFLAGS += -DBUILD_INFO_HOST_WHO=$(host_info_who)
CFLAGS += -DBUILD_INFO_HOST_WHEN_DATE=$(host_info_when_date)
CFLAGS += -DBUILD_INFO_HOST_WHEN_TIME=$(host_info_when_time)
CFLAGS += -DBUILD_INFO_HOST_WHEN_EPOCH=$(host_info_when_epoch)
endif
ifndef NO_BUILD_INFO_TARGET
CFLAGS += -DBUILD_INFO_TARGET_NAME="$(TARGETNAME)"
CFLAGS += -DBUILD_INFO_TARGET_ARCH="$(ARCH)"
CFLAGS += -DBUILD_INFO_TARGET_FAMILY="$(FAMILY)"
CFLAGS += -DBUILD_INFO_TARGET_PROC="$(PROC)"
CFLAGS += -DBUILD_INFO_TARGET_BOARD="$(BOARD)"
endif
ifndef NO_BUILD_INFO_COMPILER
cc_info_machine := "$(shell $(XCC) -dumpmachine)"
cc_info_version := "$(shell $(XCC) -dumpversion)"
CFLAGS += -DBUILD_INFO_CC_MACHINE=$(cc_info_machine)
CFLAGS += -DBUILD_INFO_CC_VERSION=$(cc_info_version)
endif

# compile recipes
objfiles := $(CFILES:%.c=$(TARGET_DIR)/%.o)
asobjfiles := $(ASFILES:%.S=$(TARGET_DIR)/%.o)
depfiles := $(CFILES:%.c=$(TARGET_DIR)/%.d)
iflags := $(foreach dir,$(INCLUDE),-I$(dir))
ifndef NO_DEBUG_SYMBOLS
CFLAGS += -g
LDFLAGS += -g
endif
CFLAGS += -O$(CC_OPTIMISATION)
ifeq (,$(findstring $(MAKECMDGOALS),clean info makeinfo))
-include $(depfiles)
endif

CFLAGS += $(CFLAGS_LATE)

$(target).hex:: $(target).elf
	$(v)$(XOBJCOPY) $(XTRA_OBJCOPY_PARAMS) -O ihex $< $@
	$(v)$(XOBJCOPY) $(XTRA_OBJCOPY_PARAMS) --gap-fill 0xff -I ihex $@ -O binary $(target).bin


asm: $(target).elf
	@$(ECHO) "ASM\t$(target)_disasm.S"
	$(v)$(XOBJDUMP) -hd -j .text -j .data -d -S $< > $(target)_disasm.S

LDFLAGS += -Map=$(target).map

ifneq "$(LINKER_FILE)" ""
LDFLAGS += --script=$(LINKER_FILE)
endif

ifeq "$(GCC_AS_LD)" "1"
_ldflags:=$(CFLAGS) $(LDFLAGS:-%=-Wl,-%)
_ldflags_late:=$(LDFLAGS_LATE:-%=-Wl,-%)
else
_ldflags:=$(LDFLAGS)
_ldflags_late:=$(LDFLAGS_LATE)
endif

$(target).elf: $(objfiles) $(asobjfiles)
	@$(ECHO) "LD\t$@"
	$(v)$(XLD) $(_ldflags) -o $@ $(OBJFILES_EXTRA_FIRST) $(objfiles) $(asobjfiles) $(OBJFILES_EXTRA_LAST) $(LIBS) $(_ldflags_late)
ifneq ($(OS),Windows_NT)
	$(v)size $@
endif
	$(v)$(ECHO) "BUILD.commit: $(build_info_git_commit)" > $(target).build
	$(v)$(ECHO) "BUILD.branch: $(build_info_git_branch)" >> $(target).build
	$(v)$(ECHO) 'BUILD.tag: $(build_info_git_tag)' >> $(target).build
	$(v)$(ECHO) "HOST.name: $(host_info_name)" >> $(target).build
	$(v)$(ECHO) "HOST.who: $(host_info_who)" >> $(target).build
	$(v)$(ECHO) "HOST.when: $(host_info_when_date) $(host_info_when_time)" >> $(target).build
	$(v)$(ECHO) "TARGET.name: $(TARGETNAME)" >> $(target).build
	$(v)$(ECHO) "TARGET.arch: $(ARCH)" >> $(target).build
	$(v)$(ECHO) "TARGET.family: $(FAMILY)" >> $(target).build
	$(v)$(ECHO) "TARGET.proc: $(PROC)" >> $(target).build
	$(v)$(ECHO) "TARGET.board: $(BOARD)" >> $(target).build

$(objfiles): $(TARGET_DIR)/%.o:%.c
	@$(ECHO) "CC\t$@"
	$(v)$(MKDIR) $(@D)
	$(v)$(XCC) $(CFLAGS) $(iflags) -c -o $@ $< $(LIBS)

$(asobjfiles): $(TARGET_DIR)/%.o:%.S
	@$(ECHO) "AS\t$@"
	$(v)$(MKDIR) $(@D)
	$(v)$(XAS) $(CFLAGS) $(iflags) -c -o $@ $< $(LIBS)

$(depfiles): $(TARGET_DIR)/%.d:%.c
	@$(ECHO) "DEP\t$@"
	$(v)$(RM) $@; \
	$(MKDIR) $(@D); \
	$(XCC) $(CFLAGS) $(iflags) -MM -MG -MF $@ $<; \
	sed -i 's,$(notdir $(basename $*)).o[ :]*, $(TARGET_DIR)/$(basename $^).o: ,g' $@

clean:
	$(v)$(RM) -r $(TARGET_DIR)

-include apps/$(APP)/$(APP)-late.mk

define var_set
$(if $1,✓,)
endef

ifeq "$(GCC_AS_LD)" "1"
_gcc_as_ld := 1
endif

makeinfo:
	$(info NO_SANITY_CHECK        $(call var_set,$(NO_SANITY_CHECK)))
	$(info NO_BUILD_INFO_GIT      $(call var_set,$(NO_BUILD_INFO_GIT)))
	$(info NO_BUILD_INFO_HOST     $(call var_set,$(NO_BUILD_INFO_HOST)))
	$(info NO_BUILD_INFO_TARGET   $(call var_set,$(NO_BUILD_INFO_TARGET)))
	$(info NO_BUILD_INFO_COMPILER $(call var_set,$(NO_BUILD_INFO_COMPILER)))
	$(info NO_DEBUG_SYMBOLS       $(call var_set,$(NO_DEBUG_SYMBOLS)))
	$(info NO_WARNINGS            $(call var_set,$(NO_WARNINGS)))
	$(info NO_CRT0                $(call var_set,$(NO_CRT0)))
	$(info CC_OPTIMISATION        $(CC_OPTIMISATION))
	$(info GCC_AS_LD              $(call var_set,$(_gcc_as_ld)))

info:
	$(info TARGET      $(TARGETNAME))
	$(info BOARD       $(BOARD))
	$(info ARCH        $(ARCH))
	$(info FAMILY      $(FAMILY))
	$(info PROC        $(PROC))
	$(info CFILES)
	$(foreach c,$(CFILES),$(info .           $(c)))
	$(info ASMFILES)
	$(foreach c,$(ASFILES),$(info .           $(c)))
	$(info CFLAGS)
	$(foreach c,$(CFLAGS),$(info .           $(c)))
	$(info INCLUDE)
	$(foreach c,$(INCLUDE),$(info .           $(c)))
	$(info LIBS)
	$(foreach c,$(LIBS),$(info .           $(c)))
	$(info LD FILE     $(LINKER_FILE))
	$(info CC          $(XCC))
