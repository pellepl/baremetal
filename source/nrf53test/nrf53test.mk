TARGETNAME := $(APP)
CONFIG_GPIO := 1
CONFIG_UART := 1
CONFIG_MINIO := 1
CONFIG_CLI := 1

CFILES += $(wildcard source/$(APP)/*.c)

FAMILY := nrf53
PROC := nrf5340app
