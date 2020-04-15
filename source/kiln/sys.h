#ifndef _SYS_H_
#define _SYS_H_

#include "types.h"

typedef enum {
    RR_LOW_POWER = 0,
    RR_WWDG,
    RR_IWDG,
    RR_SW,
    RR_PORPDR,
    RR_PIN
} reset_reason_t;

reset_reason_t sys_reset_reason(uint32_t *bootcnt, uint32_t *unexpected);
void sys_init(void);
void sys_watchdog_start(uint32_t ms);
void sys_watchdog_feed(void);
void sys_force_dump(void);
void sys_reset(void);

#endif