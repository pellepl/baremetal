/* Linker script to configure memory regions. */

SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x20000000, LENGTH = 0x4000
  RAM (rwx) :  ORIGIN = 0x20004000, LENGTH = 0x4000
}


INCLUDE "arch/cortex-m/nrf52/mdk/nrf52_common.ld"