# makefile
#
# Targets:
# all:        builds test
# test:       builds and runs all tests, recommended to have GCOV=y
#
# GCOV=y to enable coverage analysis
# DBG=y to enable loads of debug output
# 

.DEFAULT_GOAL := all

srcdir := ..
testdir := test
builddir = build
gcov-report = $(builddir)/gcov-report
binary = nvmtnvj
MKDIR = mkdir -p
FLAGS ?=

GCOV ?= y
ADDR-SANI ?= y
V ?= @
DBG ?=

CFILES_BASE := $(wildcard $(srcdir)/*.c)
CFILES_TEST = $(wildcard $(srcdir)/$(testdir)/*.c)

CFLAGS += \
	-I$(srcdir) \
	-I$(srcdir)/$(testdir) \
	-I../../flash \

CFLAGS += -DNVMTNVJ_TEST

CFILES_FS = $(CFILES_BASE)

# default to the test binary
CFILES := $(CFILES_FS) $(CFILES_TEST)
binary := $(binary)-test
targetdir := $(builddir)/test

OBJFILES = $(CFILES:%.c=$(targetdir)/%.o)
OBJFSFILES = $(CFILES_FS:%.c=$(targetdir)/%.o)
DEPFILES = $(CFILES:%.c=$(targetdir)/%.d)

CFLAGS += \
-Wall -Wno-format-y2k -W -Wstrict-prototypes -Wmissing-prototypes \
-Wpointer-arith -Wreturn-type -Wcast-qual -Wwrite-strings -Wswitch \
-Wshadow -Wcast-align -Wchar-subscripts -Winline -Wno-nested-externs \
-Wredundant-decls

# todo remove when done
CFLAGS += -Wno-unused

# enable address sanitization
ifeq ($(ADDR-SANI),y)
CFLAGS += -fsanitize=address
LDFLAGS += -fsanitize=address -fno-omit-frame-pointer -O
LIBS += -lasan
endif

# enable coverage test
GCOV_FLAGS = -fprofile-arcs -ftest-coverage
ifeq ($(GCOV),y)
CFLAGS_GCOV = $(GCOV_FLAGS)
LDFLAGS_GCOV = $(GCOV_FLAGS)
GCOV_SWITCHES = -H #-n
RUN_GCOV = y
endif

LDFLAGS += $(LDFLAGS_GCOV) -lm
CFLAGS += $(FLAGS)

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),info)
-include $(DEPFILES)
endif
endif

# coverage for fs files only
$(OBJFSFILES): CFLAGS += $(CFLAGS_GCOV)

$(builddir)/$(binary): $(OBJFILES)
	$(V)@echo "LN\t$@"
	$(V)$(CC) $(LDFLAGS) -o $@ $(OBJFILES) $(LIBS)

$(OBJFILES) : $(targetdir)/%.o:%.c
	$(V)echo "CC\t$@"
	$(V)$(MKDIR) $(@D);
	$(V)$(CC) $(CFLAGS) -g -c -o $@ $<

$(DEPFILES) : $(targetdir)/%.d:%.c
	$(V)rm -f $@; \
	$(MKDIR) $(@D); \
	$(CC) -M $< > $@.$$$$ 2> /dev/null; \
	sed 's,\($*\)\.o[ :]*, $(targetdir)/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

.PHONY: all test clean

.mkdirs:
	-$(V)$(MKDIR) $(builddir) $(targetdir)

ALL = $(builddir)/$(binary)

# make all binaries
all:	$(ALL)

# build and run test suites
test: $(builddir)/$(binary)
	$(V)./$(builddir)/$(binary)
ifeq ($(RUN_GCOV),y)
	$(V)rm -f $(gcov-report)
	$(V)for cfile in $(CFILES_FS); do \
		gcov $(GCOV_SWITCHES) -o $(targetdir)/$(srcdir) $(targetdir)/$$cfile >> $(gcov-report); \
	done
endif

test-buildonly: $(builddir)/$(binary)

clean:
	$(V)echo "CLEAN"
	$(V)rm -rf $(builddir)
	$(V)rm -f *.gcov
	$(V)rm -f _tests_ok _tests_fail

info:
	$(V)echo CFILES
	$(V)echo $(CFILES)
	$(V)echo CFLAGS
	$(V)echo $(CFLAGS)
	$(V)echo LDFLAGS
	$(V)echo $(LDFLAGS)
	$(V)echo DEPFILES
	$(V)echo $(DEPFILES)
