NO_CRT0 := 1
# TODO: this is very much specific to a Debian 64 bit can
LINKER_FILE := /lib/x86_64-linux-gnu/ldscripts/elf_x86_64.x
LIBS += -lgcc -lc
GCC_AS_LD := 1
