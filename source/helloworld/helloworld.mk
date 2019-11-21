TARGETNAME := $(APP)
CONFIG_GPIO := 1
CONFIG_UART := 1
CONFIG_MINIO := 1

CFILES += $(wildcard source/$(APP)/*.c)
