#include <device/device.h>
#include <device/driver.h>

#include <bus.h>
#include <list.h>
#include <init.h>

#include <string.h>
#include <errno.h>

static struct list_head device_list = LIST_HEAD_INIT(device_list);

int device_register(struct device *dev)
{
    struct device *_dev = NULL;

    if (!dev->init_name)
        return -EINVAL;

    list_for_each_entry(_dev, &device_list, list)
    {
        if (strcmp(_dev->init_name, dev->init_name) == 0) {
            if (_dev == dev) {
                return -EEXIST;
            }
        }
    }

    list_add_tail(&dev->list, &device_list);

    driver_probe(dev);

    return 0;
}

int device_probe(struct driver *drv)
{
    struct bus_type *bus = drv->bus;
    struct device *dev;

    list_for_each_entry(dev, &device_list, list)
    {
        if (dev->bus == bus) {
            if (bus->match(dev, drv)) {
                dev->driver = drv;
                if (bus->probe(dev)) {
                    dev->driver = NULL;
                }
            }
        }
    }
    return 0;
}

void __init device_init()
{
    volatile struct device **start = __board_device_list_start;
    volatile struct device **end = __board_device_list_end;
    volatile int count = ((uint32_t)end - (uint32_t)start) / (sizeof(void *));
    int i;
    struct device *dev;

    for (i = 0; i < count; i++) {
        dev = start[i];
        dev->init(dev);
    }
}

