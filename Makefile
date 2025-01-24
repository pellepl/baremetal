APP ?= kiln
BOARD ?= hy-stm32
include apps/$(APP)/$(APP).mk
include main.mk
