CFILES += $(modules_dir)/tick_timer/tick_timer.c
INCLUDE += $(modules_dir)/tick_timer

$(eval $(call include_hal_implementation,tick_timer))
