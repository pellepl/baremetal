/* Linker script to configure memory regions. */

SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x20000000, LENGTH = 0x20000
  RAM (rwx) : ORIGIN = 0x20020000, LENGTH = 0x20000
}


INCLUDE "arch/cortex-m/nrf53/mdk/nrf_common.ld"
