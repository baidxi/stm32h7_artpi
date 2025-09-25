#pragma once

#include <device/device.h>
#include <device/driver.h>
#include <list.h>

struct irq_desc;

typedef int (*irq_handler_t)(unsigned irq, void *dev);

struct irq_chip_driver;

struct irq_chip {
    struct device dev;
    unsigned nr_irq;
    struct irq_desc *descs;
    struct list_head list;
    int irq_offset;
    struct irq_chip_driver *driver;
};

struct irq_desc {
    const char *name;
    irq_handler_t handler;
    void *dev;
};

struct irq_chip_driver {
    struct device_driver drv;
    int (*probe)(struct irq_chip *chip);
    int (*remove)(struct irq_chip *chip);
};

#define to_irq_chip(_d) container_of(_d, struct irq_chip, dev)

int register_irq_chip(struct irq_chip *chip);
int request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags, const char *name, void *dev);
void free_irq(unsigned int irq, void *dev);
void handle_irq(unsigned int irq);
int register_irq_driver(struct irq_chip_driver *drv);