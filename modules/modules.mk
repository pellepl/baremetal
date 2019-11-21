ifeq "$(ARCH)" ""
$(error No ARCH defined)
endif
ifeq "$(FAMILY)" ""
$(error No FAMILY defined)
endif
ifeq "$(PROC)" ""
$(error No PROC defined)
endif

# Convenience macro
# Tries to find $(proc_dir)/hal/hal_<arg>_$(PROC).c
# and $(family_dir)/hal/hal_<arg>_$(FAMILY).c
#
# E.g. for bluepill board where ARCH=cortex-m, FAMILY=stm32f1, PROC=stm32f103x6,
# and arg "foo", the macro checks for files
#    "arch/cortex-m/stm32f1/stm32f103x6/hal/hal_foo_stm32f103x6.c", and
#    "arch/cortex-m/stm32f1/hal/hal_foo_stm32f1.c"
define include_hal_implementation
$(eval _$(1)_proc_hal_path := $(proc_dir)/hal/hal_$(1)_$(PROC).c)
$(eval _$(1)_family_hal_path := $(family_dir)/hal/hal_$(1)_$(FAMILY).c)
# first, check if there is a hal file for the specific processor
ifneq "$(wildcard $(_$(1)_proc_hal_path))" ""
  CFILES += $(_$(1)_proc_hal_path)
# else, check if there is a hal file for processor family
else ifneq "$(wildcard $(_$(1)_family_hal_path))" ""
  CFILES += $(_$(1)_family_hal_path)
else
  $$(error Cannot find "$$(_$(1)_proc_hal_path)" or "$$(_$(1)_family_hal_path)")
endif
endef

# march thru all directories under $(modules_dir), and if 
# CONFIG_<module>=1 then $(modules_dir)/<module>/<module>.mk will be included,
# and -DCONFIG_<module>=1 is added to CFLAGS
modules_dir_list := $(sort $(dir $(wildcard $(modules_dir)/*/*)))
modules_list_lc := $(modules_dir_list:$(modules_dir)/%/=%)

$(foreach module_name, $(modules_list_lc), \
  $(eval module_name_uc := $(shell echo $(module_name) | tr a-z A-Z)  ) \
  $(eval $(if $(filter 1,$(CONFIG_$(strip $(module_name_uc)))),$(eval include $(modules_dir)/$(module_name)/$(module_name).mk)  )) \
  $(eval $(if $(filter 1,$(CONFIG_$(strip $(module_name_uc)))),$(eval CFLAGS += -DCONFIG_$(strip $(module_name_uc))=1)  )) \
)

# debug
ifdef dump-module-info
$(info MODULES:: $(modules_list_uc))
$(foreach module_name, $(modules_list_lc), \
  $(eval module_name_uc := $(shell echo $(module_name) | tr a-z A-Z)  ) \
  $(eval $(info CONFIG_$(module_name_uc))  ) \
  $(eval $(if $(filter 1,$(CONFIG_$(strip $(module_name_uc)))),$(info enabled),$(info disabled))  ) \
)
endif

