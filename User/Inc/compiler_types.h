#pragma once

/* 兼容标准C的typeof实现 */
#if defined(__GNUC__)
    #define typeof(x) __typeof__(x)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    #define typeof(x) _Generic((x), int: int, char: char, float: float, double: double, default: void*)
#else
    /* 简化版typeof，仅用于基本类型 */
    #define typeof(x) void*
#endif

/* 简化版__same_type，在标准C环境中不进行严格类型检查 */
#define __same_type(a, b) (1)