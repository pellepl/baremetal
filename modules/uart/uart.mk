CFILES += $(modules_dir)/uart/uart_driver.c
INCLUDE += $(modules_dir)/uart

$(eval $(call include_hal_implementation,uart))
