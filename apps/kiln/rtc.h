#ifndef _RTC_H_
#define _RTC_H_
#include "bmtypes.h"

#ifndef CONFIG_RTC_CLOCK_HZ
#define CONFIG_RTC_CLOCK_HZ 32768
#endif

#ifndef CONFIG_RTC_PRESCALER
 // this gives a tick resolution of 1/1024 seconds for a 32768 Hz clock
#define CONFIG_RTC_PRESCALER 32
#endif

#define RTC_TICKS_PER_SEC   (CONFIG_RTC_CLOCK_HZ/CONFIG_RTC_PRESCALER)

#define RTC_TICK_TO_MS(t)   (((t)*1000)/RTC_TICKS_PER_SEC)
#define RTC_MS_TO_TICK(ms)  (((ms)*RTC_TICKS_PER_SEC)/1000)
#define RTC_TICK_TO_S(t)    ((t)/RTC_TICKS_PER_SEC)
#define RTC_S_TO_TICK(s)    ((s)*RTC_TICKS_PER_SEC)

typedef struct {
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint16_t millisecond;
} rtc_time_t;

typedef struct {
  uint16_t year;
  uint8_t month;
  uint8_t year_day;
  uint8_t month_day;
  uint8_t week_day;
} rtc_date_t;

typedef struct {
  rtc_date_t date;
  rtc_time_t time;
} rtc_datetime_t;

uint32_t rtc_month2secs(uint8_t month, int is_leap);
int64_t rtc_year2secs(int32_t year, int *is_leap);
uint64_t rtc_datetime2secs(const rtc_datetime_t *tm);
int rtc_secs2datetime(int64_t t, rtc_datetime_t *tm);

int rtc_init(void (*alarm_callback)(void));
void rtc_reset(void);
void rtc_set_tick(uint64_t tick);
uint64_t rtc_get_tick(void);
void rtc_set_alarm_tick(uint64_t tick);
uint64_t rtc_get_alarm_tick(void);
void rtc_cancel_alarm(void);
void rtc_set_alarm(rtc_datetime_t *datetime);
void rtc_set_date_time(rtc_datetime_t *datetime);
void rtc_get_date_time(rtc_datetime_t *datetime);

#endif