/* Linker script to configure memory regions. */

SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x1000000, LENGTH = 0x40000
  EXTFLASH (rx) : ORIGIN = 0x10000000, LENGTH = 0x8000000
  RAM (rwx) : ORIGIN = 0x21000000, LENGTH = 0x10000
}


INCLUDE "arch/cortex-m/nrf53/mdk/nrf_common.ld"
