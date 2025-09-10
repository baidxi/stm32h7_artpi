#pragma once

#include "../device.h"
#include "../driver.h"

#include <stdint.h>
#include <stddef.h>

struct tty_operations {
    int (*open)(struct device *dev);
    int (*close)(struct device *dev);
    size_t (*read)(struct device *dev, void *buf, size_t count);
    size_t (*write)(struct device *dev, const void *buf, size_t count);
    int (*ioctl)(struct device *dev, unsigned int cmd, unsigned long arg);
    void (*set_termios)(struct device *dev, void *termios);
};

struct tty_device {
    struct device dev;
    int port_num;
    uint32_t baudrate;
    uint8_t data_bits;
    uint8_t parity;
    uint8_t stop_bits;
    uint8_t flow_control;
    const struct tty_operations *ops;
    struct list_head list;
};

struct tty_driver {
    struct driver drv;
    int (*probe)(struct tty_device *tty);
    void (*remove)(struct tty_device *tty);
    const struct driver_match_table *match_ptr;
};

#define to_tty_device(d)    container_of(d, struct tty_device, dev)
#define to_tty_driver(d)    container_of(d, struct tty_driver, drv)

int tty_open(struct tty_device *tty);
void tty_close(struct tty_device *tty);
size_t tty_read(struct tty_device *tty, void *buf, size_t count);
size_t tty_write(struct tty_device *tty, const void *buf, size_t count);
int tty_ioctl(struct tty_device *tty, unsigned int cmd, unsigned long arg);
int tty_device_register(struct tty_device *tty);
int tty_driver_register(struct tty_driver *tty_drv);
struct tty_device *tty_device_lookup_by_handle(void *handle);
struct tty_device *tty_device_lookup_by_name(const char *name);