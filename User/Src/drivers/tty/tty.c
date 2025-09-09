#include <device/tty/tty.h>
#include <device/driver.h>
#include <bus.h>
#include <init.h>
#include <list.h>
#include <string.h>

#include <errno.h>

static struct list_head device_list = LIST_HEAD_INIT(device_list);

int tty_device_register(struct tty_device *tty)
{
    int ret;

    if (!tty)
        return -EINVAL;

    tty->dev.bus = get_virtual_bus_type();

    ret = device_register(&tty->dev);
    if (ret)
        return ret;

    list_add_tail(&tty->list, &device_list);

    return 0;
}

static int tty_driver_probe(struct device *dev)
{
    struct tty_device *tty = to_tty_device(dev);
    struct tty_driver *drv = to_tty_driver(dev->driver);
    const struct driver_match_table *ptr;

    for (ptr = drv->match_ptr; ptr && ptr->compatible; ptr++) {
        if (strcmp(ptr->compatible, tty->dev.name) == 0) {
            return drv->probe(tty);
        }
    }

    return drv->probe(tty);
}

static void tty_driver_remove(struct device *dev)
{
    struct tty_device *tty = to_tty_device(dev);
    struct tty_driver *drv = to_tty_driver(dev->driver);

    return drv->remove(tty);
}

int tty_driver_register(struct tty_driver *tty_drv)
{
    if (!tty_drv ||
        !tty_drv->probe || !tty_drv->remove)
    return -EINVAL;

    tty_drv->drv.bus = get_virtual_bus_type();
    tty_drv->drv.probe = tty_driver_probe;
    tty_drv->drv.remove = tty_driver_remove;

    return driver_register(&tty_drv->drv);
}

int tty_open(struct tty_device *tty)
{
    if (tty && tty->ops && tty->ops->open)
        return tty->ops->open(&tty->dev);
    return -EOPNOTSUPP;
}

void tty_close(struct tty_device *tty)
{
    if (tty && tty->ops && tty->ops->close)
        tty->ops->close(&tty->dev);
}

size_t tty_read(struct tty_device *tty, void *buf, size_t count)
{
    if (tty && tty->ops && tty->ops->read)
        return tty->ops->read(&tty->dev, buf, count);
    return -EOPNOTSUPP;
}

size_t tty_write(struct tty_device *tty, const void *buf, size_t count)
{
    if (tty && tty->ops && tty->ops->write)
        return tty->ops->write(&tty->dev, buf, count);
    return -EOPNOTSUPP;
}

int tty_ioctl(struct tty_device *tty, unsigned int cmd, unsigned long arg)
{
    if (tty && tty->ops && tty->ops->ioctl)
        return tty->ops->ioctl(&tty->dev, cmd, arg);
    return -EOPNOTSUPP;
}

struct tty_device *tty_device_lookup_by_handle(void *handle)
{
    struct tty_device *tty;

    list_for_each_entry(tty, &device_list, list)
    {
        if (tty->dev.private_data == handle) {
            return tty;
        }
    }

    return NULL;
}
