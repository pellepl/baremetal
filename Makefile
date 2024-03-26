APP ?= prodtest
BOARD ?= nrf52
include apps/$(APP)/$(APP).mk
include main.mk
