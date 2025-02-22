APP ?= minikiln
BOARD ?= minikiln
DEVICE_SINGLE := 1
include apps/$(APP)/$(APP).mk
include main.mk
