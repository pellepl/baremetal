#ifndef _STEPPER_H_
#define _STEPPER_H_

#include <stdint.h>
#include "targets.h"

typedef enum
{
    STEPPER_DIR_CW = 0,
    STEPPER_DIR_CCW,
    MAX_STEPPER_DIRS
} stepper_dir_t;

int stepper_move(stepper_id_t id, stepper_dir_t dir);
void stepper_init(void);
void stepper_deinit(void);

#endif // _STEPPER_H_