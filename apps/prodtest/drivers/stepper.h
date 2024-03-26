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

int stepper_move_singlestep(stepper_id_t id, stepper_dir_t dir);
int stepper_move_singlestep_multiple(const stepper_id_t *ids, const stepper_dir_t *dir, int steppers);
int stepper_move_multistep_multiple(const stepper_id_t *ids, const uint16_t *steps, const stepper_dir_t *dirs, int steppers);
int stepper_hour_to_step(const stepper_id_t id, uint8_t hour);
int stepper_position_multiple_hour(const stepper_id_t *ids, const uint8_t *hours, int steppers);
int stepper_position_all_hour(uint8_t hour);
void stepper_position_reset_reference(stepper_id_t id);
void stepper_init(void);
void stepper_deinit(void);

#endif // _STEPPER_H_
