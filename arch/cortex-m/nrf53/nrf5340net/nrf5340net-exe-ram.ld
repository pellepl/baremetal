/* Linker script to configure memory regions. */

SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x21000000, LENGTH = 0x8000
  RAM (rwx) : ORIGIN = 0x21008000, LENGTH = 0x7ff0
}


INCLUDE "arch/cortex-m/nrf53/mdk/nrf_common.ld"
