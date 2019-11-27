proc_dir := $(family_dir)/$(PROC)
mdk_dir := $(family_dir)/mdk/include

ifeq "$(PROC)" "msp430g2553"
CFLAGS += -D__MSP430G2553__
CFLAGS += -mmcu=msp430X -mlarge
LDFLAGS += -mmsp430X 

#CFLAGS += -mmcu=MSP430G2553 #$(PROC)

else
$(error PROC is not defined correctly for arch $(ARCH), family $(FAMILY))
endif

INCLUDE += $(mdk_dir)
LDFLAGS += --library-path=$(mdk_dir)
LIBS += -lgcc
LIBS += -lc
LIBS += -lnosys


LINKER_FILE = $(mdk)/$(PROC).ld

NO_CRT0 := 1
