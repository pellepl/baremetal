TARGETNAME := $(APP)
CONFIG_GPIO := 1

CFILES += $(wildcard apps/$(APP)/*.c)
