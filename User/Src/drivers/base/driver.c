#include <device/driver.h>
#include <bus.h>
#include <list.h>
#include <init.h>

#include <FreeRTOS.h>

#include <errno.h>
#include <stdlib.h>

static struct list_head driver_list = LIST_HEAD_INIT(driver_list);

int driver_register(struct driver *drv)
{
    if (!drv ||
        !drv->probe ||
        !drv->remove ||
        !drv->name)
        return -EINVAL;

    if (drv->private_data_size && drv->private_data_auto_alloc) {
        drv->private_data = pvPortMalloc(drv->private_data_size);
        if (!drv->private_data)
            return -ENOMEM;
    }
    
    INIT_LIST_HEAD(&drv->device_list);
    list_add_tail(&drv->list, &driver_list);
    device_probe(drv);
    return 0;
}

int driver_probe(struct device *dev)
{
    struct driver *drv;
    
    list_for_each_entry(drv, &driver_list, list)
    {
        if (drv->bus->match(dev, drv)) {
            drv->bus->probe(dev);
        }
    }
    return -1;
}

void __init driver_init()
{
    volatile void *start = __device_driver_list_start;
    volatile void *end = __device_driver_list_end;
    volatile int count = (end - start) / sizeof(void *);
    int i;
    struct driver *drv;
    
    for (i = 0, drv = __device_driver_list_start[i]; i < count ; drv = __device_driver_list_start[i++])
    {
        drv->init(drv);
    }
}