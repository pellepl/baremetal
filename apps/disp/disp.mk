TARGETNAME := $(APP)
CONFIG_GPIO := 1
CONFIG_UART := 1
CONFIG_MINIO := 1
CFLAGS += -DMINIO_FORMAT_FLOAT
CONFIG_CLI := 1

CFILES += $(wildcard apps/$(APP)/*.c)
