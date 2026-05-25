MEMORY
{
RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 192K
RAM2 (xrw)      : ORIGIN = 0x20030000, LENGTH = 64K
FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 1024K
}

INCLUDE "arch/cortex-m/stm32u3/stm32u3.ld"
