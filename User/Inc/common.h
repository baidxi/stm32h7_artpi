#pragma once

#include <stdint.h>

#define MAX_PATH    1024

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))

#ifndef static_assert

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    #define static_assert(expr, msg) _Static_assert(expr, msg)
#else
    #define static_assert(expr, msg) typedef char __static_assert_##__line__[(expr) ? 1 : -1]
#endif
#endif

static inline void halt()
{
    while(1) {
        
    }
}

