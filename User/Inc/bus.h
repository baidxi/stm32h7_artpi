#pragma once

#include "list.h"

struct device;
struct driver;

struct bus_type {
    const char *name;
    int (*probe)(struct device *dev);
    int (*remove)(struct device *dev);
    int (*match)(struct device *dev, struct driver *drv);
    void (*init)(void);
    struct list_head list;
};

// extern struct bus_type virtual_bus_type;

struct bus_type *get_virtual_bus_type();

#define section(name)   __attribute__((used,section(name)))

#define BUS_TYPE(name_str)  \
static struct bus_type section("bus_type_list") name_str##_bus_type

/*

#define BUS_DECLEARE(name_str, probe_fn, remove_fn, match_fn, init_fn)  \
static struct bus_type section("bus_type_list") name_str##_bus_type = { \
    .name = #name_str,  \
    .probe = probe_fn,  \
    .remove = remove_fn,\
    .match = match_fn,  \
    .init = init_fn,    \
}

*/
void bus_type_init(void);
int device_probe(struct driver *);