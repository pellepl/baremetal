MEMORY
{
FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 128K
RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 20K
}

INCLUDE "arch/cortex-m/stm32f1/stm32f1.ld"