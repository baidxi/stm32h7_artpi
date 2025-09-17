#pragma once

#include <stdint.h>

struct gpio_desc;

struct gpio_i2c_data {
    struct gpio_desc *sda;
    struct gpio_desc *scl;
    const char *sda_pin_name;
    const char *scl_pin_name;
    uint16_t retries;
    uint16_t timeout;
    uint16_t delay_us;
};