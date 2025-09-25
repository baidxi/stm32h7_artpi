#pragma once

#include "../list.h"

#define DEVICE_NAME_MAX 16

struct device_driver;
struct bus_type;

struct device {
    char name[DEVICE_NAME_MAX];
    const char *init_name;
    struct list_head list;
    struct device_driver *driver;
    struct bus_type *bus;
    void *private_data;
    void (*init)(struct device *);
    int irq;
};

static inline void dev_set_drvdata(struct device *dev, void *data)
{
    dev->private_data = data;
}

static inline void* dev_get_drvdata(struct device *dev)
{
    return dev->private_data;
}

extern struct device *__board_device_list_start[];
extern struct device *__board_device_list_end[];

int device_register(struct device *dev);

#define register_device(__name, __dev)  \
static struct device *__name##_device __attribute__((used, section("board_device_list"))) = &__dev

void device_init(void);

