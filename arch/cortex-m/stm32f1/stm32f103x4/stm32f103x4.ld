MEMORY
{
FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 16K
RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 6K
}

INCLUDE "arch/cortex-m/stm32f1/stm32f1.ld"