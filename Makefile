APP ?= hwprobe
BOARD ?= pca10100
include apps/$(APP)/$(APP).mk
include main.mk
