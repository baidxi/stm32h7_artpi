#pragma once

#include <device/i2c/i2c.h>

struct mpu6050_device {
    struct i2c_client dev;
    const char *intr_gpio_name;
};

#define to_mpu6050(_d)  container_of(_d, struct mpu6050_device, dev)