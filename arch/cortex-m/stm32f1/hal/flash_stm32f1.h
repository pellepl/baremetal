#ifndef _FLASH_STM32F1_H_
#define _FLASH_STM32F1_H_


#if defined(LD)

// low density
#define CODE_SECTORS        32
#define CODE_SECTOR_SIZE    1024
#define SYSTEM_SECTOR_SIZE  2048
#define OPT_SECTOR_SIZE     16

#elif defined(MD) 

// medium density
#define CODE_SECTORS        128
#define CODE_SECTOR_SIZE    1024
#define SYSTEM_SECTOR_SIZE  2048
#define OPT_SECTOR_SIZE     16

#elif defined(HD) 

// high density
#define CODE_SECTORS        256
#define CODE_SECTOR_SIZE    2048
#define SYSTEM_SECTOR_SIZE  2048
#define OPT_SECTOR_SIZE     16

#elif defined(CL) 

// connectivity line
#define CODE_SECTORS        128
#define CODE_SECTOR_SIZE    2048
#define SYSTEM_SECTOR_SIZE  (18*1024)
#define OPT_SECTOR_SIZE     16

#endif



#endif // _FLASH_STM32F1_H_
