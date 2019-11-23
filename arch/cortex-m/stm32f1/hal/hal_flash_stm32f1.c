#include "types.h"

static uint16_t sector_size;
static uint16_t sectors;
static uint16_t banks;

int flash_init(void) {
    volatile uint32_t *DBGMCU_IDCODE = (volatile uint32_t *)0xe0042000;
    volatile uint16_t *FLASH_SIZE = (volatile uint16_t *)0x1ffff7e0; // in kB
    uint32_t code = *DBGMCU_IDCODE;
    uint32_t size = *FLASH_SIZE;
    //uint32_t rev = (code>>16) & 0xffff;
    uint32_t dev = (code & 0x0fff);

    // see https://github.com/blacksphere/blackmagic/blob/master/src/target/stm32f1.c
    switch(dev) {
	case 0x410:  // medium density
	case 0x412:  // low density
	case 0x420:  // value, low-/medium density
		sector_size = 1024;
		break;
	case 0x414:	 // high density
	case 0x418:  // connectivity
	case 0x428:	 // value, high density
        sector_size = 2048;
        break;
	case 0x438:  // STM32F303x6/8 and STM32F328
	case 0x422:  // STM32F30x
	case 0x446:  // STM32F303xD/E and STM32F398xE
	case 0x432:  // STM32F37x
	case 0x439:  // STM32F302C8
        sector_size = 2048;
        break;
    case 0x430: // xl density
        // two banks 512+256/512
        // code sector size 2048
	}

    #if 0
    volatile uint32_t *DBGMCU_IDCODE_CM0 = (volatile uint32_t *)0x40015800;
    code = *DBGMCU_IDCODE_CM0;
    uint32_t dev = (code & 0x0fff);
	switch(dev) {
	case 0x444:  /* STM32F03 RM0091 Rev.7, STM32F030x[4|6] RM0360 Rev. 4*/
		t->driver = "STM32F03";
		flash_size = 0x8000;
		break;
	case 0x445:  /* STM32F04 RM0091 Rev.7, STM32F070x6 RM0360 Rev. 4*/
		t->driver = "STM32F04/F070x6";
		flash_size = 0x8000;
		break;
	case 0x440:  /* STM32F05 RM0091 Rev.7, STM32F030x8 RM0360 Rev. 4*/
		t->driver = "STM32F05/F030x8";
		flash_size = 0x10000;
		break;
	case 0x448:  /* STM32F07 RM0091 Rev.7, STM32F070xB RM0360 Rev. 4*/
		t->driver = "STM32F07";
		flash_size = 0x20000;
		block_size = 0x800;
		break;
	case 0x442:  /* STM32F09 RM0091 Rev.7, STM32F030xC RM0360 Rev. 4*/
		t->driver = "STM32F09/F030xC";
		flash_size = 0x40000;
		block_size = 0x800;
		break;
	default:     /* NONE */
		return false;
	}
#endif

    return 0;
}

int flash_get_sectors_for_type(flash_type_t type, uint32_t *sector, uint32_t *num_sectors) {
}


static void get_flash_info(void) {
    volatile uint32_t *DBGMCU_IDCODE = (volatile uint32_t *)0xe0042000;
    volatile uint16_t *FLASH_SIZE = (volatile uint16_t *)0x1ffff7e0; // in kB
    uint32_t code = *DBGMCU_IDCODE;
    uint32_t rev = (code>>16) & 0xffff;
    uint32_t dev = (code & 0x0fff);

    switch (dev) {
        case 0x412: // low density
        // code sector size 1024
        // opt size         16
        break;
        case 0x410: // medium density
        // code sector size 1024
        // opt size         16
        break;
        case 0x414: // high density
        // code sector size 2048
        // opt size         16
        break;
        case 0x430: // xl density
        // two banks 512+256/512
        // code sector size 2048
        break;
        case 0x418: // connectivity
        // code sector size 2048
        // opt size         16
        break;
        default:
        break;
    }

}
