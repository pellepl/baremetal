MEMORY
{
RAM (xrw)      : ORIGIN = 0x20000000, LENGTH = 8K
FLASH (rx)      : ORIGIN = 0x8000000, LENGTH = 64K
}

INCLUDE "arch/cortex-m/stm32l0/stm32l0.ld"
