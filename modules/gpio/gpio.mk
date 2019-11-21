CFILES += $(modules_dir)/gpio/gpio_driver.c
INCLUDE += $(modules_dir)/gpio
$(eval $(call include_hal_implementation,gpio))
