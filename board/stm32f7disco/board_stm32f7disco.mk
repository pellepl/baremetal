ARCH ?= cortex-m
FAMILY ?= stm32f7
PROC ?= stm32f746

# corresponding to stm32f7xx_hal_conf.h for this board
CFLAGS += -DHSE_VALUE=25000000
CFLAGS += -DHSE_STARTUP_TIMEOUT=500
CFLAGS += -DHSI_VALUE=16000000
CFLAGS += -DLSI_VALUE=32000
CFLAGS += -DLSE_VALUE=32768
CFLAGS += -DLSE_STARTUP_TIMEOUT=5000
CFLAGS += -DEXTERNAL_CLOCK_VALUE=12288000
CFLAGS += -DVDD_VALUE=3300
CFLAGS += -DTICK_INT_PRIORITY=0x0F
CFLAGS += -DART_ACCLERATOR_ENABLE=1

LINKER_FILES += $(board_dir)/sdram.ld

# should the app define this maybe?
CFLAGS += -DHAL_MODULE_ENABLED
CFLAGS += -DHAL_ADC_MODULE_ENABLED
CFLAGS += -DHAL_DMA_MODULE_ENABLED
CFLAGS += -DHAL_FLASH_MODULE_ENABLED
CFLAGS += -DHAL_SRAM_MODULE_ENABLED
CFLAGS += -DHAL_SDRAM_MODULE_ENABLED
CFLAGS += -DHAL_GPIO_MODULE_ENABLED
CFLAGS += -DHAL_LTDC_MODULE_ENABLED
CFLAGS += -DHAL_PWR_MODULE_ENABLED
CFLAGS += -DHAL_RCC_MODULE_ENABLED
CFLAGS += -DHAL_SPI_MODULE_ENABLED
CFLAGS += -DHAL_TIM_MODULE_ENABLED
CFLAGS += -DHAL_CORTEX_MODULE_ENABLED
CFLAGS += -DHAL_SAI_MODULE_ENABLED
CFLAGS += -DHAL_I2C_MODULE_ENABLED
