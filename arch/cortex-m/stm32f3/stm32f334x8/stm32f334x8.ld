MEMORY
{
FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 64K
RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 12K
}

INCLUDE "arch/cortex-m/stm32f3/stm32f3.ld"
