#pragma once

#include "../list.h"

#include <stddef.h>
#include <stdbool.h>

struct device;
struct bus_type;

struct driver_match_table {
    const char *compatible;
    uint32_t data;
};

struct driver {
    const char *name;
    struct list_head list;
    struct list_head device_list;
    int (*probe)(struct device *dev);
    void (*remove)(struct device *dev);
    size_t private_data_size;
    void *private_data;
    bool private_data_auto_alloc;
    const struct driver_match_table *match_ptr;
    struct bus_type *bus;
    void (*init)(struct driver *);
};

extern struct driver *__device_driver_list_start[];
extern struct driver *__device_driver_list_end[];

int driver_register(struct driver *drv);
int driver_probe(struct device *dev);

#define register_driver(__name, __drv)  \
static struct driver *__name##_driver section("device_driver_list") = &__drv

void driver_init(void);

/* 
#define driver_init(name, fn)   \
static initcall_t name section("device_driver_init_list") = { \
    .init = fn, \
}
*/


