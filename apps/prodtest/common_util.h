#pragma once

static inline int abs(int x)
{
    return x < 0 ? -x : x;
}

static inline int max(int x, int y)
{
    return x > y ? x : y;
}

static inline int min(int x, int y)
{
    return x < y ? x : y;
}

static inline int sign(int x)
{
    return x < 0 ? -1 : 1;
}

static inline int clamp(int x, int min, int max)
{
    if (x < min)
        return min;
    if (x > max)
        return max;
    return x;
}
