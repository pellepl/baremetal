MEMORY
{
RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 128K
RAM2 (xrw)      : ORIGIN = 0x20020000, LENGTH = 64K
FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 512K
}

INCLUDE "arch/cortex-m/stm32u3/stm32u3.ld"
