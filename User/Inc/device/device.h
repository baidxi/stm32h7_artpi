#pragma once

#include "../list.h"

#define DEVICE_NAME_MAX 32

struct driver;
struct bus_type;

struct device {
    char name[DEVICE_NAME_MAX];
    const char *init_name;
    struct list_head list;
    struct driver *driver;
    struct bus_type *bus;
    void *private_data;
    void (*init)(struct device *);
};

extern struct device *__board_device_list_start[];
extern struct device *__board_device_list_end[];

int device_register(struct device *dev);

#define register_device(__name, __drv)  \
static struct device *__name##_device section("board_device_list") = &__drv

void device_init(void);

