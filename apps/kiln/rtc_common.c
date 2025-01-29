#include "rtc.h"

//
// Kudos:
// taken from http://git.musl-libc.org/cgit/musl/tree/src/time/
//

/* 2000-03-01 (mod 400 year, immediately after feb29 */
#define LEAPOCH (946684800LL + 86400 * (31 + 29))

#define DAYS_PER_400Y (365 * 400 + 97)
#define DAYS_PER_100Y (365 * 100 + 24)
#define DAYS_PER_4Y (365 * 4 + 1)

#define S32_MIN (int32_t)(-0x7fffffffl)
#define S32_MAX (int32_t)(0x7fffffffl)

int rtc_secs2datetime(int64_t t, rtc_datetime_t *tm)
{
    int64_t secs64;
    int32_t remdays, remsecs, remyears;
    int32_t qc_cycles, c_cycles, q_cycles;
    int32_t years, months;
    int32_t wday, yday, leap;
    static const char days_in_month[] = {31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 31, 29};

    /* Reject time_t values whose year would overflow int */
    if (t < S32_MIN * 31622400LL || t > S32_MAX * 31622400LL)
        return -1;

    secs64 = t - LEAPOCH;
    // days64 = secs64 / 86400;
    uint32_t days = ((uint32_t)(secs64 >> 7)) / 675;
    // remsecs = secs64 % 86400;
    remsecs = (uint32_t)(secs64 - ((uint64_t)days * 86400));
    if (remsecs < 0)
    {
        remsecs += 86400;
        days--;
    }

    wday = (3 + days) % 7;
    if (wday < 0)
        wday += 7;

    qc_cycles = days / DAYS_PER_400Y;
    remdays = days % DAYS_PER_400Y;
    if (remdays < 0)
    {
        remdays += DAYS_PER_400Y;
        qc_cycles--;
    }

    c_cycles = remdays / DAYS_PER_100Y;
    if (c_cycles == 4)
        c_cycles--;
    remdays -= c_cycles * DAYS_PER_100Y;

    q_cycles = remdays / DAYS_PER_4Y;
    if (q_cycles == 25)
        q_cycles--;
    remdays -= q_cycles * DAYS_PER_4Y;

    remyears = remdays / 365;
    if (remyears == 4)
        remyears--;
    remdays -= remyears * 365;

    leap = !remyears && (q_cycles || !c_cycles);
    yday = remdays + 31 + 28 + leap;
    if (yday >= 365 + leap)
        yday -= 365 + leap;

    years = remyears + 4 * q_cycles + 100 * c_cycles + 400 * qc_cycles;

    for (months = 0; days_in_month[months] <= remdays; months++)
        remdays -= days_in_month[months];

    if (years + 100 > S32_MAX || years + 100 < S32_MIN)
        return -1;

    tm->date.year = years + 100;
    tm->date.month = months + 2;
    if (tm->date.month >= 12)
    {
        tm->date.month -= 12;
        tm->date.year++;
    }
    tm->date.month_day = remdays + 1;
    tm->date.week_day = wday;
    tm->date.year_day = yday;

    tm->time.hour = remsecs / 3600;
    tm->time.minute = remsecs / 60 % 60;
    tm->time.second = remsecs % 60;
    tm->time.millisecond = 0;
    return 0;
}

uint64_t rtc_datetime2secs(const rtc_datetime_t *tm)
{
    int is_leap;
    int64_t year = tm->date.year;
    int32_t month = tm->date.month;
    if (month >= 12 || month < 0)
    {
        int adj = month / 12;
        month %= 12;
        if (month < 0)
        {
            adj--;
            month += 12;
        }
        year += adj;
    }
    uint64_t t = rtc_year2secs(year, &is_leap);
    t += rtc_month2secs(month, is_leap);
    t += 86400LL * (tm->date.month_day - 1);
    t += 3600LL * tm->time.hour;
    t += 60LL * tm->time.minute;
    t += tm->time.second;
    return t;
}

int64_t rtc_year2secs(int32_t year32, int *is_leap)
{
    if (year32 - 2ULL <= 136)
    {
        int32_t leaps = (year32 - 68) >> 2;
        if (!((year32 - 68) & 3))
        {
            leaps--;
            if (is_leap)
                *is_leap = 1;
        }
        else if (is_leap)
            *is_leap = 0;
        return 31536000LL * ((int64_t)year32 - 70) + 86400LL * leaps;
    }

    int32_t cycles, centuries, leaps, rem;

    cycles = (year32 - 100) / 400;
    rem = (year32 - 100) % 400;
    if (rem < 0)
    {
        cycles--;
        rem += 400;
    }
    if (!rem)
    {
        if (is_leap)
            *is_leap = 1;
        centuries = 0;
        leaps = 0;
    }
    else
    {
        if (rem >= 200)
        {
            if (rem >= 300)
                centuries = 3, rem -= 300;
            else
                centuries = 2, rem -= 200;
        }
        else
        {
            if (rem >= 100)
                centuries = 1, rem -= 100;
            else
                centuries = 0;
        }
        if (!rem)
        {
            if (is_leap)
                *is_leap = 0;
            leaps = 0;
        }
        else
        {
            leaps = rem / 4U;
            rem %= 4U;
            if (is_leap)
                *is_leap = !rem;
        }
    }

    leaps += 97 * cycles + 24 * centuries - *is_leap;

    return ((uint64_t)year32 - 100) * 31536000LL + leaps * 86400LL + 946684800 + 86400;
}

uint32_t rtc_month2secs(uint8_t month, int is_leap)
{
    static const uint32_t secs_through_month[] = {
        0, 31 * 86400, 59 * 86400, 90 * 86400,
        120 * 86400, 151 * 86400, 181 * 86400, 212 * 86400,
        243 * 86400, 273 * 86400, 304 * 86400, 334 * 86400};
    uint32_t t = secs_through_month[month];
    if (is_leap && month >= 2)
        t += 86400;
    return t;
}