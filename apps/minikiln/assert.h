#ifndef _ASSERT_H_
#define _ASSERT_H_

#include "minio.h"
#include "sys.h"

#ifndef ASSERT
#define ASSERT(x)                                         \
    do                                                    \
    {                                                     \
        if (!(x))                                         \
        {                                                 \
            printf("ASSERT %s:%d\n", __FILE__, __LINE__); \
            sys_force_dump();                             \
        }                                                 \
    } while (0);
#endif

#endif