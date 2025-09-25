#include <bus.h>

#include <irq.h>
#include <FreeRTOS.h>

#include <string.h>
#include <errno.h>

static struct list_head irq_chip_list = LIST_HEAD_INIT(irq_chip_list);

static struct irq_desc *irq_to_desc(unsigned int irq);

static int irq_bus_probe(struct device *dev)
{
    struct irq_chip *chip = to_irq_chip(dev);
    return chip->driver->probe(chip);
}

static int irq_bus_remove(struct device *dev)
{
    struct irq_chip *chip = to_irq_chip(dev);
    return chip->driver->remove(chip);
}

static int irq_bus_match(struct device *dev, struct device_driver *drv)
{
    const struct device_match_table *ptr;
    struct irq_chip_driver *_drv = container_of(drv, struct irq_chip_driver, drv);
    struct irq_chip *chip = to_irq_chip(dev);

    for (ptr = drv->match_ptr; ptr && ptr->compatible; ptr++)
    {
        if (strcmp(ptr->compatible, dev->init_name) == 0) {
            chip->driver = _drv;
            return 1;
        }
    }
    return 0;
}

static struct bus_type irq_bus_type = {
    .name = "irq",
    .probe = irq_bus_probe,
    .remove = irq_bus_remove,
    .match = irq_bus_match,
};

register_bus_type(irq_bus_type);

int register_irq_driver(struct irq_chip_driver *drv)
{
    if (!drv->drv.name)
        return -EINVAL;

    drv->drv.bus = &irq_bus_type,
    drv->drv.probe = irq_bus_probe,
    drv->drv.remove = irq_bus_remove,

    driver_register(&drv->drv);
    
    return 0;
}

int register_irq_chip(struct irq_chip *chip)
{
    int ret;

    if (!chip->dev.init_name || !chip->nr_irq)
        return -EINVAL;

    if (chip->descs)
        return -EINVAL;

    chip->dev.bus = &irq_bus_type;

    ret = device_register(&chip->dev);

    if (ret)
        return ret;

    list_add_tail(&chip->list, &irq_chip_list);

    return 0;
}

int request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags, const char *name, void *dev)
{
    struct irq_desc *desc = irq_to_desc(irq);

    if (!desc)
        return -EINVAL;

    if (desc->name)
        return -EEXIST;

    desc->dev = dev;
    desc->handler = handler;
    desc->name = name;

    return 0;
}

void free_irq(unsigned int irq, void *dev)
{
    struct irq_desc *desc = irq_to_desc(irq);
    if (desc)
    {
        if (desc->dev == dev)
        {
            desc->dev = NULL;
            desc->handler = NULL;
        }
    }
}

static struct irq_desc *irq_to_desc(unsigned int irq)
{
    struct irq_chip *chip;
    unsigned int nr_irq;

    list_for_each_entry(chip, &irq_chip_list, list)
    {
        if ((chip->nr_irq + chip->irq_offset) < irq)
            continue;

        nr_irq = irq - chip->irq_offset;

        return &chip->descs[nr_irq];
    }

    return NULL;
}

void handle_irq(unsigned int irq)
{
    struct irq_desc *desc = irq_to_desc(irq);
    if (desc && desc->handler)
    {
        desc->handler(irq, desc->dev);
    }
}