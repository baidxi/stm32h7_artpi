#include <init.h>
#include <bus.h>
#include <list.h>
#include <device/device.h>
#include <device/driver.h>

#include <string.h>

static struct list_head bus_list;

// struct bus_type virtual_bus_type;
static void virtual_bus_init(void);

extern struct bus_type __bus_type_list_start[];
extern struct bus_type __bus_type_list_end[];

static int virtual_bus_probe(struct device *dev)
{
    int ret;

    struct driver *drv = dev->driver;

    ret = drv->probe(dev);
    if (ret)
        dev->driver = NULL;
    return ret;
}

static int virtual_bus_remove(struct device *dev)
{
    struct driver *drv = dev->driver;
    drv->remove(dev);
    dev->driver = NULL;
    return 0;
}

static int virtual_bus_match(struct device *dev, struct driver *drv)
{
    const struct driver_match_table *ptr;

    if (dev->bus != drv->bus)
        return 0;

    for (ptr = drv->match_ptr; ptr && ptr->compatible; ptr++) {
        if (strcmp(dev->init_name, ptr->compatible) == 0) {
            dev->driver = drv;
            return 1;
        }
    }
    return 0;
}

int bus_register(struct bus_type *bus)
{
    list_add_tail(&bus->list, &bus_list);
    return 0;
}



BUS_TYPE(virtual) = {
    .name = "virtual",
    .init = virtual_bus_init,
    .probe = virtual_bus_probe,
    .remove = virtual_bus_remove,
    .match = virtual_bus_match,
};

static void virtual_bus_init(void)
{
    INIT_LIST_HEAD(&bus_list);
    bus_register(&virtual_bus_type);
}

struct bus_type *get_virtual_bus_type()
{
    return &virtual_bus_type;
}

void __init bus_type_init(void)
{
    struct bus_type *bus =  __bus_type_list_start;

    while(bus < __bus_type_list_end) {
        bus->init();
        bus++;
    }
}