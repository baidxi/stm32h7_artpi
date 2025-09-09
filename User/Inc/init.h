#pragma once

typedef struct {
    int (*init)(void);
}initcall_t;


#define early_initcall(_name, fn)   \
static initcall_t _name section("early_init_list") = {  \
    .init = fn, \
}

#define __init section("init_function_list")
