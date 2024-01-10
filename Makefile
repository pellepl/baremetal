-include ~/local.mk
APP ?= disp
BOARD ?= pca10056
include apps/$(APP)/$(APP).mk
include main.mk
