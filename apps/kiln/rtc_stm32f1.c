#include "minio.h"
#include "bmtypes.h"
#include "stm32f1xx_ll_rtc.h"
#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_rcc.h"
#include "stm32f1xx_ll_pwr.h"
#include "rtc.h"
#include "cli.h"
#include "minio.h"

#ifndef CONFIG_RTC_CLOCK_HZ
#define CONFIG_RTC_CLOCK_HZ         32768
#endif
#ifndef CONFIG_RTC_PRESCALER
 // this gives a tick resolution of 1/1024 seconds for a 32768 Hz clock
#define CONFIG_RTC_PRESCALER        32
#endif

typedef enum {
    STM32F1_RTC_CLOCK_INTERNAL = 0,
    STM32F1_RTC_CLOCK_EXTERNAL_LOW,
    STM32F1_RTC_CLOCK_EXTERNAL_HIGH
} rtc_clock_t;

#ifndef CONFIG_RTC_CLOCK
#define CONFIG_RTC_CLOCK STM32F1_RTC_CLOCK_EXTERNAL_LOW
#endif

#ifndef CONFIG_STM32F1_BKP_REG_MAGIC    
#define CONFIG_STM32F1_BKP_REG_MAGIC               LL_RTC_BKP_DR2
#endif
#ifndef CONFIG_STM32F1_BKP_REG_RTCCYH
#define CONFIG_STM32F1_BKP_REG_RTCCYH              LL_RTC_BKP_DR3
#endif
#ifndef CONFIG_STM32F1_BKP_REG_RTCCYL
#define CONFIG_STM32F1_BKP_REG_RTCCYL              LL_RTC_BKP_DR4
#endif
#ifndef CONFIG_STM32F1_BKP_REG_RTCALH
#define CONFIG_STM32F1_BKP_REG_RTCALH              LL_RTC_BKP_DR5
#endif
#ifndef CONFIG_STM32F1_BKP_REG_RTCALL
#define CONFIG_STM32F1_BKP_REG_RTCALL              LL_RTC_BKP_DR6
#endif
#ifndef CONFIG_STM32F1_BKP_REG_RTCOFHH
#define CONFIG_STM32F1_BKP_REG_RTCOFHH             LL_RTC_BKP_DR7
#endif
#ifndef CONFIG_STM32F1_BKP_REG_RTCOFHL
#define CONFIG_STM32F1_BKP_REG_RTCOFHL             LL_RTC_BKP_DR8
#endif
#ifndef CONFIG_STM32F1_BKP_REG_RTCOFLH
#define CONFIG_STM32F1_BKP_REG_RTCOFLH             LL_RTC_BKP_DR9
#endif
#ifndef CONFIG_STM32F1_BKP_REG_RTCOFLL
#define CONFIG_STM32F1_BKP_REG_RTCOFLL             LL_RTC_BKP_DR10
#endif

#define MAGIC_ALARM_OFF        0x1e1f
#define MAGIC_ALARM_ON         0xd01f

#define bkprd(x)    LL_RTC_BKP_GetRegister(BKP, x)
#define bkpwr(x,y)  LL_RTC_BKP_SetRegister(BKP, x, y)
 
static void (*rtc_cb)(void) = 0;

static uint32_t rtc_bkp_get_rtc_cycles(void) {
    uint16_t h = bkprd(CONFIG_STM32F1_BKP_REG_RTCCYH);
    uint16_t l = bkprd(CONFIG_STM32F1_BKP_REG_RTCCYL);
    return ((uint32_t)h<<16) | (uint32_t)l;
}

static void rtc_bkp_set_rtc_cycles(uint32_t rtccy) {
    bkpwr(CONFIG_STM32F1_BKP_REG_RTCCYH, (uint16_t)(rtccy>>16));
    bkpwr(CONFIG_STM32F1_BKP_REG_RTCCYL, (uint16_t)(rtccy&0xffff));
}

static int rtc_bkp_is_alarm_enabled(void) {
    return bkprd(CONFIG_STM32F1_BKP_REG_MAGIC) == MAGIC_ALARM_ON;
}

static void rtc_bkp_set_alarm_enabled(int enabled) {
    bkpwr(CONFIG_STM32F1_BKP_REG_MAGIC, enabled ? MAGIC_ALARM_ON : MAGIC_ALARM_OFF);
}

static uint64_t rtc_bkp_get_alarm_tick(void) {
    uint16_t hh = bkprd(CONFIG_STM32F1_BKP_REG_RTCALH);
    uint16_t hl = bkprd(CONFIG_STM32F1_BKP_REG_RTCALL);
    return ((uint64_t)hh << 48) | ((uint64_t)hl << 32) | ((uint64_t)RTC->ALRH<<16) | (uint64_t)RTC->ALRL;
}

static void rtc_bkp_set_alarm_tick(uint64_t tick) {
    bkpwr(CONFIG_STM32F1_BKP_REG_RTCALH, (uint16_t)(tick>>48));
    bkpwr(CONFIG_STM32F1_BKP_REG_RTCALL, (uint16_t)(tick>>32));
}

static int64_t rtc_bkp_get_offs_tick(void) {
    uint16_t hh = bkprd(CONFIG_STM32F1_BKP_REG_RTCOFHH);
    uint16_t hl = bkprd(CONFIG_STM32F1_BKP_REG_RTCOFHL);
    uint16_t lh = bkprd(CONFIG_STM32F1_BKP_REG_RTCOFLH);
    uint16_t ll = bkprd(CONFIG_STM32F1_BKP_REG_RTCOFLL);
    return ((uint64_t)hh << 48) | ((uint64_t)hl << 32) | ((uint64_t)lh<<16) | (uint64_t)ll;
}

static void rtc_bkp_set_offs_tick(int64_t tick_offs) {
    bkpwr(CONFIG_STM32F1_BKP_REG_RTCOFHH, (uint16_t)(tick_offs>>48));
    bkpwr(CONFIG_STM32F1_BKP_REG_RTCOFHL, (uint16_t)(tick_offs>>32));
    bkpwr(CONFIG_STM32F1_BKP_REG_RTCOFLH, (uint16_t)(tick_offs>>16));
    bkpwr(CONFIG_STM32F1_BKP_REG_RTCOFLL, (uint16_t)(tick_offs>>0));
}

uint64_t rtc_get_tick(void) {
    uint32_t cycles, counter;
    counter = LL_RTC_TIME_Get(RTC);
    cycles = rtc_bkp_get_rtc_cycles();
    if (counter > LL_RTC_TIME_Get(RTC)) {
        // amidst an overflow, reread
        counter = LL_RTC_TIME_Get(RTC);
        cycles = rtc_bkp_get_rtc_cycles();
    }
    return ((uint64_t)cycles << 32) | (uint64_t)counter;
}

void rtc_set_tick(uint64_t tick) {
    rtc_bkp_set_rtc_cycles(tick >> 32);
    while (LL_RTC_IsActiveFlag_RTOF(RTC) == 0);
    LL_RTC_DisableWriteProtection(RTC);
    LL_RTC_TIME_Set(RTC, tick & 0xffffffff);
    while (LL_RTC_IsActiveFlag_RTOF(RTC) == 0);
    LL_RTC_EnableWriteProtection(RTC);
}

void rtc_set_alarm_tick(uint64_t tick) {
    rtc_bkp_set_alarm_tick(tick);
    while (LL_RTC_IsActiveFlag_RTOF(RTC) == 0);
    LL_RTC_DisableWriteProtection(RTC);
    LL_RTC_ALARM_Set(RTC, (uint32_t)(tick & 0xffffffff));
    while (LL_RTC_IsActiveFlag_RTOF(RTC) == 0);
    LL_RTC_EnableWriteProtection(RTC);
    rtc_bkp_set_alarm_enabled(1);
}

void rtc_cancel_alarm(void) {
    if (rtc_bkp_is_alarm_enabled()) {
        rtc_bkp_set_alarm_enabled(0);
        while (LL_RTC_IsActiveFlag_RTOF(RTC) == 0);
        LL_RTC_DisableWriteProtection(RTC);
        LL_RTC_ALARM_Set(RTC, 0);
        while (LL_RTC_IsActiveFlag_RTOF(RTC) == 0);
        LL_RTC_EnableWriteProtection(RTC);
    }
}

uint64_t rtc_get_alarm_tick(void) {
    return rtc_bkp_get_alarm_tick();
}

void rtc_reset(void) {
    rtc_bkp_set_rtc_cycles(0);
    rtc_bkp_set_offs_tick(0);
    rtc_bkp_set_alarm_tick(0);
    rtc_bkp_set_alarm_enabled(0);
    while (LL_RTC_IsActiveFlag_RTOF(RTC) == 0);
    LL_RTC_DisableWriteProtection(RTC);
    LL_RTC_TIME_Set(RTC, 0);
    LL_RTC_EnableWriteProtection(RTC);
    while (LL_RTC_IsActiveFlag_RTOF(RTC) == 0);
}


int rtc_init(void (*alarm_callback)(void)) {
    int res = 0;
    rtc_cb = alarm_callback;
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_BKP | LL_APB1_GRP1_PERIPH_PWR);
    LL_PWR_EnableBkUpAccess();

    if (bkprd(CONFIG_STM32F1_BKP_REG_MAGIC) != MAGIC_ALARM_OFF &&
        bkprd(CONFIG_STM32F1_BKP_REG_MAGIC) != MAGIC_ALARM_ON) {
        printf("resetting RTC\n");
        LL_RCC_ForceBackupDomainReset();
        LL_RCC_ReleaseBackupDomainReset();
        rtc_bkp_set_rtc_cycles(0);
        rtc_bkp_set_offs_tick(0);
        rtc_bkp_set_alarm_enabled(0);
        bkpwr(CONFIG_STM32F1_BKP_REG_MAGIC, MAGIC_ALARM_OFF);
        res = 1;
    }

    rtc_clock_t rtc_clock = CONFIG_RTC_CLOCK;
    switch (rtc_clock) {
    case STM32F1_RTC_CLOCK_INTERNAL:
        LL_RCC_LSE_Disable();
        LL_RCC_LSI_Enable();
        while (LL_RCC_LSI_IsReady() == 0);
        LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSI);
        break;
    case STM32F1_RTC_CLOCK_EXTERNAL_LOW:
        LL_RCC_LSE_Enable();
        LL_RCC_LSI_Disable();
        while (LL_RCC_LSE_IsReady() == 0);
        LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSE);
        break;
    case STM32F1_RTC_CLOCK_EXTERNAL_HIGH:
        LL_RCC_LSE_Disable();
        LL_RCC_LSI_Disable();
        LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_HSE_DIV128);
        break;
    }
    LL_RCC_EnableRTC();
    __DSB();  
    while (LL_RTC_IsActiveFlag_RTOF(RTC) == 0);
    LL_RTC_DisableWriteProtection(RTC);
    LL_RTC_ClearFlag_ALR(RTC);
    LL_RTC_ClearFlag_OW(RTC);
    LL_RTC_ClearFlag_SEC(RTC);
    LL_RTC_ClearFlag_RS(RTC);
    LL_RTC_SetAsynchPrescaler(RTC, CONFIG_RTC_PRESCALER-1);
    LL_RTC_ALARM_Set(RTC, 0);
    while (LL_RTC_IsActiveFlag_RTOF(RTC) == 0);

    if (res) {
        // to prevent false coincisions(?) @ startup we start counter at 1
        LL_RTC_TIME_Set(RTC, 1);
        while (LL_RTC_IsActiveFlag_RTOF(RTC) == 0);
    }

    NVIC_ClearPendingIRQ(RTC_IRQn);
    NVIC_EnableIRQ(RTC_IRQn);
    LL_RTC_ClearFlag_ALR(RTC);
    LL_RTC_EnableIT_ALR(RTC);

    LL_RTC_EnableWriteProtection(RTC);
    while (LL_RTC_IsActiveFlag_RTOF(RTC) == 0);

    // do not use anything else than alarm interrupt as it is the only
    // one that can wake us from standby
    #if 0 // TODO
    EXTI_InitTypeDef exti_cfg;
    EXTI_ClearITPendingBit(EXTI_Line17);
    exti_cfg.EXTI_Line = EXTI_Line17;
    exti_cfg.EXTI_Mode = EXTI_Mode_Interrupt;
    exti_cfg.EXTI_Trigger = EXTI_Trigger_Rising;
    exti_cfg.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti_cfg);
    #endif
    return res;
}

static void rtc_handle_alarm(void) {
    if (rtc_bkp_is_alarm_enabled()) {
        // user alarm active
        // check that it is the correct cycle
        uint64_t cur_tick = rtc_get_tick();
        uint64_t alarm_tick = rtc_bkp_get_alarm_tick();
        if ((uint32_t)(cur_tick >> 32) >= (uint32_t)(alarm_tick >> 32)) {
            // correct cycle, user alarm has went off
            int coincide = (uint32_t)alarm_tick == 0;
            if (coincide) {
                // the user alarm coincides with rtc cycle, update super cycle counter
                rtc_bkp_set_rtc_cycles(rtc_bkp_get_rtc_cycles() + 1);
            }
            rtc_bkp_set_alarm_enabled(0);
            while (LL_RTC_IsActiveFlag_RTOF(RTC) == 0);
            LL_RTC_DisableWriteProtection(RTC);
            LL_RTC_ALARM_Set(RTC, 0);
            LL_RTC_EnableWriteProtection(RTC);
            while (LL_RTC_IsActiveFlag_RTOF(RTC) == 0);
            // blip user
            if (rtc_cb) rtc_cb();
        } else {
            // too early cycle
            rtc_bkp_set_rtc_cycles(rtc_bkp_get_rtc_cycles() + 1);
            while (LL_RTC_IsActiveFlag_RTOF(RTC) == 0);
            LL_RTC_DisableWriteProtection(RTC);
            if ((uint32_t)(cur_tick >> 32) == (uint32_t)(alarm_tick >> 32)) {
                // the user alarm will go off this rtc cycle, update rtc alarm
                LL_RTC_ALARM_Set(RTC, alarm_tick && 0xffffffff);
            } else {
                // the user alarm will go off some other cycle, alarm on overflow
                LL_RTC_ALARM_Set(RTC, 0);
            }
            LL_RTC_EnableWriteProtection(RTC);
            while (LL_RTC_IsActiveFlag_RTOF(RTC) == 0);
        }
    } else {
        // user alarm inactive, rtc cycle overflow
        rtc_bkp_set_rtc_cycles(rtc_bkp_get_rtc_cycles() + 1);
        while (LL_RTC_IsActiveFlag_RTOF(RTC) == 0);
        LL_RTC_DisableWriteProtection(RTC);
        LL_RTC_ALARM_Set(RTC, 0);
        LL_RTC_EnableWriteProtection(RTC);
        while (LL_RTC_IsActiveFlag_RTOF(RTC) == 0);
    }
}

void rtc_get_date_time(rtc_datetime_t *datetime) {
    uint64_t ticks = rtc_get_tick();
    ticks += rtc_bkp_get_offs_tick();
    int64_t seconds = (int64_t)(RTC_TICK_TO_S(ticks));
    rtc_secs2datetime(seconds, datetime);
    datetime->time.millisecond = RTC_TICK_TO_MS(ticks % RTC_TICKS_PER_SEC);
}

void rtc_set_date_time(rtc_datetime_t *datetime) {
    uint64_t tick_now = rtc_get_tick();
    int64_t dt_secs_set = rtc_datetime2secs(datetime);
    uint64_t tick_set = RTC_S_TO_TICK(dt_secs_set);
    int64_t offs = tick_set - tick_now;
    rtc_bkp_set_offs_tick(offs);
}

void rtc_set_alarm(rtc_datetime_t *datetime) {
    int64_t dt_secs_alarm = rtc_datetime2secs(datetime);
    int64_t ticks_alarm = RTC_S_TO_TICK(dt_secs_alarm);
    ticks_alarm -= rtc_bkp_get_offs_tick();
    rtc_set_alarm_tick(ticks_alarm);
}

void RTC_IRQHandler(void);
void RTC_IRQHandler(void) {
    if (LL_RTC_IsActiveFlag_ALR(RTC)) {
        LL_RTC_ClearFlag_ALR(RTC);
        while (LL_RTC_IsActiveFlag_RTOF(RTC) == 0);
        rtc_handle_alarm();
    }
    NVIC_ClearPendingIRQ(RTC_IRQn);
}


static int cli_rtc_tick(int argc, const char **argv) {
    if (argc == 0) {
        uint64_t t = rtc_get_tick();
        printf("tick: %08x%08x\n", (uint32_t)(t>>32), (uint32_t)(t&0xffffffff));
    } else {
        uint64_t hi = argc == 1 ? 0 : atoi(argv[0]);
        uint64_t lo = argc == 1 ? atoi(argv[0]) : atoi(argv[1]);
        uint64_t t = (uint64_t)(hi << 32) | lo;
        rtc_set_tick(t);
    }
    return 0;
}
CLI_FUNCTION(cli_rtc_tick, "rtc_tick", "(<hi>) (<lo>): sets or shows  rtc tick");
static int cli_rtc_alarm_tick(int argc, const char **argv) {
    if (argc == 0) {
        uint64_t t = rtc_get_alarm_tick();
        printf("alarm: %08x%08x\n", (uint32_t)(t>>32), (uint32_t)(t&0xffffffff));
    } else {
        uint64_t hi = argc == 1 ? 0 : atoi(argv[0]);
        uint64_t lo = argc == 1 ? atoi(argv[0]) : atoi(argv[1]);
        uint64_t t = (uint64_t)(hi << 32) | lo;
        rtc_set_alarm_tick(t);
    }
    return 0;
}
CLI_FUNCTION(cli_rtc_alarm_tick, "rtc_alarm_tick", "(<hi>) (<lo>): sets or shows alarm tick");

static int cli_rtc_date(int argc, const char **argv) {
    if (argc == 0) {
        rtc_datetime_t dt;
        rtc_get_date_time(&dt);
        printf("%d-%02d-%02d %02d:%02d:%02d.%04d\n",
            dt.date.year,dt.date.month+1,dt.date.month_day+1,
            dt.time.hour,dt.time.minute,dt.time.second,dt.time.millisecond);
    } else {
        rtc_datetime_t dt;
        rtc_get_date_time(&dt);
        if (argc >= 1) dt.date.year = atoi(argv[0]);
        if (argc >= 2) dt.date.month = atoi(argv[1])-1;
        if (argc >= 3) dt.date.month_day = atoi(argv[2])-1;
        if (argc >= 4) dt.time.hour = atoi(argv[3]);
        if (argc >= 5) dt.time.minute = atoi(argv[4]);
        if (argc >= 6) dt.time.second = atoi(argv[5]);
        if (argc >= 7) dt.time.millisecond = atoi(argv[6]);
        rtc_set_date_time(&dt);
    }
    return 0;
}
CLI_FUNCTION(cli_rtc_date, "rtc_date", "(<year> <month> <day> <h> <m> <s> <millis>): sets and shows rtc data");
