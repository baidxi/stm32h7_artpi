#pragma once

#include <device/gpio/gpio.h>
#include <gpio.h>

struct stm32_gpio_chip {
    struct gpio_chip chip;
    GPIO_TypeDef *port;
    uint16_t mask;
};


extern struct stm32_gpio_chip stm32_gpioa_chip;
extern struct stm32_gpio_chip stm32_gpiob_chip;
extern struct stm32_gpio_chip stm32_gpioc_chip;
extern struct stm32_gpio_chip stm32_gpiod_chip;
extern struct stm32_gpio_chip stm32_gpioe_chip;
extern struct stm32_gpio_chip stm32_gpiof_chip;
extern struct stm32_gpio_chip stm32_gpiog_chip;
extern struct stm32_gpio_chip stm32_gpioh_chip;
extern struct stm32_gpio_chip stm32_gpioi_chip;
extern struct stm32_gpio_chip stm32_gpioj_chip;

void stm32_gpio_init(struct device *dev);
