#ifndef _DISP_H_
#define _DISP_H_

#include <stdint.h>
#include "board_disp.h"

typedef struct disp_cfg
{
    union
    {
        struct
        {
            uint16_t tearing, spi_csn, spi_sck, spi_mosi, dc, resetn;
        } pins;
        uint16_t pin_arr[6];
    };

} disp_cfg_t;

const disp_cfg_t *disp_get_cfg(void);

#endif // _DISP_H_