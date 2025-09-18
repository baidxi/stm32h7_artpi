#include <device/gpio/stm32_gpio.h>

#include <gpio.h>
#include <errno.h>

static int stm32_gpio_request(struct gpio_chip *gc, unsigned int offset);
static void stm32_gpio_free(struct gpio_chip *gc, unsigned int offset);
static int stm32_gpio_get_direction(struct gpio_chip *gc, unsigned int offset);
static int stm32_gpio_direction_input(struct gpio_chip *gc, unsigned int offset);
static int stm32_gpio_direction_output(struct gpio_chip *gc, unsigned int offset, int value);
static int stm32_gpio_get(struct gpio_chip *gc, unsigned int offset);
static void stm32_gpio_set(struct gpio_chip *gc, unsigned int offset, int value);
static int stm32_gpio_set_config(struct gpio_chip *gc, unsigned int offset, unsigned long config);
static int stm32_to_irq(struct gpio_chip *gc, unsigned int offset);
static struct gpio_irq_chip stm32_gpio_irq_chip;


struct stm32_gpio_chip stm32_gpioa_chip = {
    .chip = {
        .label = "PA",
        .request = stm32_gpio_request,
        .free = stm32_gpio_free,
        .get_direction = stm32_gpio_get_direction,
        .direction_input = stm32_gpio_direction_input,
        .direction_output = stm32_gpio_direction_output,
        .get = stm32_gpio_get,
        .set = stm32_gpio_set,
        .set_config = stm32_gpio_set_config,
        .to_irq = stm32_to_irq,
        .irq_chip = &stm32_gpio_irq_chip,
        .base = 0,
        .ngpio = 16,
    },
    .port = GPIOA,
};

struct stm32_gpio_chip stm32_gpiob_chip = {
    .chip = {
        .label = "PB",
        .request = stm32_gpio_request,
        .free = stm32_gpio_free,
        .get_direction = stm32_gpio_get_direction,
        .direction_input = stm32_gpio_direction_input,
        .direction_output = stm32_gpio_direction_output,
        .get = stm32_gpio_get,
        .set = stm32_gpio_set,
        .set_config = stm32_gpio_set_config,
        .to_irq = stm32_to_irq,
        .irq_chip = &stm32_gpio_irq_chip,
        .base = 16,
        .ngpio = 16,
    },
    .port = GPIOB,
};

struct stm32_gpio_chip stm32_gpioc_chip = {
    .chip = {
        .label = "PC",
        .request = stm32_gpio_request,
        .free = stm32_gpio_free,
        .get_direction = stm32_gpio_get_direction,
        .direction_input = stm32_gpio_direction_input,
        .direction_output = stm32_gpio_direction_output,
        .get = stm32_gpio_get,
        .set = stm32_gpio_set,
        .set_config = stm32_gpio_set_config,
        .to_irq = stm32_to_irq,
        .irq_chip = &stm32_gpio_irq_chip,
        .base = 32,
        .ngpio = 16,
    },
    .port = GPIOC,
};

struct stm32_gpio_chip stm32_gpiod_chip = {
    .chip = {
        .label = "PD",
        .request = stm32_gpio_request,
        .free = stm32_gpio_free,
        .get_direction = stm32_gpio_get_direction,
        .direction_input = stm32_gpio_direction_input,
        .direction_output = stm32_gpio_direction_output,
        .get = stm32_gpio_get,
        .set = stm32_gpio_set,
        .set_config = stm32_gpio_set_config,
        .to_irq = stm32_to_irq,
        .irq_chip = &stm32_gpio_irq_chip,
        .base = 48,
        .ngpio = 16,
    },
    .port = GPIOD,
};

struct stm32_gpio_chip stm32_gpioe_chip = {
    .chip = {
        .label = "PE",
        .request = stm32_gpio_request,
        .free = stm32_gpio_free,
        .get_direction = stm32_gpio_get_direction,
        .direction_input = stm32_gpio_direction_input,
        .direction_output = stm32_gpio_direction_output,
        .get = stm32_gpio_get,
        .set = stm32_gpio_set,
        .set_config = stm32_gpio_set_config,
        .to_irq = stm32_to_irq,
        .irq_chip = &stm32_gpio_irq_chip,
        .base = 64,
        .ngpio = 16,
    },
    .port = GPIOE,
};

struct stm32_gpio_chip stm32_gpiof_chip = {
    .chip = {
        .label = "PF",
        .request = stm32_gpio_request,
        .free = stm32_gpio_free,
        .get_direction = stm32_gpio_get_direction,
        .direction_input = stm32_gpio_direction_input,
        .direction_output = stm32_gpio_direction_output,
        .get = stm32_gpio_get,
        .set = stm32_gpio_set,
        .set_config = stm32_gpio_set_config,
        .to_irq = stm32_to_irq,
        .irq_chip = &stm32_gpio_irq_chip,
        .base = 80,
        .ngpio = 16,
    },
    .port = GPIOF,
};

struct stm32_gpio_chip stm32_gpioh_chip = {
    .chip = {
        .label = "PH",
        .request = stm32_gpio_request,
        .free = stm32_gpio_free,
        .get_direction = stm32_gpio_get_direction,
        .direction_input = stm32_gpio_direction_input,
        .direction_output = stm32_gpio_direction_output,
        .get = stm32_gpio_get,
        .set = stm32_gpio_set,
        .set_config = stm32_gpio_set_config,
        .to_irq = stm32_to_irq,
        .irq_chip = &stm32_gpio_irq_chip,
        .base = 112,
        .ngpio = 16,
    },
    .port = GPIOH,
};

struct stm32_gpio_chip stm32_gpioi_chip = {
    .chip = {
        .label = "PI",
        .request = stm32_gpio_request,
        .free = stm32_gpio_free,
        .get_direction = stm32_gpio_get_direction,
        .direction_input = stm32_gpio_direction_input,
        .direction_output = stm32_gpio_direction_output,
        .get = stm32_gpio_get,
        .set = stm32_gpio_set,
        .set_config = stm32_gpio_set_config,
        .to_irq = stm32_to_irq,
        .irq_chip = &stm32_gpio_irq_chip,
        .base = 128,
        .ngpio = 16,
    },
    .port = GPIOI,
};

struct stm32_gpio_chip stm32_gpioj_chip = {
    .chip = {
        .label = "PJ",
        .request = stm32_gpio_request,
        .free = stm32_gpio_free,
        .get_direction = stm32_gpio_get_direction,
        .direction_input = stm32_gpio_direction_input,
        .direction_output = stm32_gpio_direction_output,
        .get = stm32_gpio_get,
        .set = stm32_gpio_set,
        .set_config = stm32_gpio_set_config,
        .to_irq = stm32_to_irq,
        .irq_chip = &stm32_gpio_irq_chip,
        .base = 134,
        .ngpio = 16,
    },
    .port = GPIOJ,
};

static int stm32_gpio_request(struct gpio_chip *gc, unsigned int offset)
{
    struct stm32_gpio_chip *chip = container_of(gc, struct stm32_gpio_chip, chip);
    GPIO_TypeDef *port = chip->port;
    
    if (offset >= gc->ngpio) {
        return -EINVAL;
    }
    
    if (chip->mask & offset)
        return -EBUSY;

    chip->mask |= (1 << offset);
    
    return 0;
}

static void stm32_gpio_free(struct gpio_chip *gc, unsigned int offset)
{
    struct stm32_gpio_chip *chip = container_of(gc, struct stm32_gpio_chip, chip);
    GPIO_TypeDef *port = chip->port;
    
    if (offset >= gc->ngpio) {
        return;
    }

    chip->mask &= ~(1 << offset);
    
    /* 将GPIO设置为模拟模式，相当于释放 */
    port->MODER &= ~(3U << (2 * offset));
    port->PUPDR &= ~(3U << (2 * offset));
    port->OTYPER &= ~(1U << offset);
    port->OSPEEDR &= ~(3U << (2 * offset));
}

static int stm32_gpio_get_direction(struct gpio_chip *gc, unsigned int offset)
{
    struct stm32_gpio_chip *chip = container_of(gc, struct stm32_gpio_chip, chip);
    GPIO_TypeDef *port = chip->port;
    uint32_t mode;
    
    if (offset >= gc->ngpio) {
        return -EINVAL;
    }
    
    mode = (port->MODER >> (2 * offset)) & 3;
    
    if (mode == 0) {
        return GPIO_DIRECTION_IN;
    } else if (mode == 1) {
        return GPIO_DIRECTION_OUT;
    } else {
        return -EINVAL;
    }
}

static int stm32_gpio_direction_input(struct gpio_chip *gc, unsigned int offset)
{
    struct stm32_gpio_chip *chip = container_of(gc, struct stm32_gpio_chip, chip);
    GPIO_TypeDef *port = chip->port;
    
    if (offset >= gc->ngpio) {
        return -EINVAL;
    }
    
    /* 设置为输入模式 */
    port->MODER = (port->MODER & ~(3U << (2 * offset))) | (0U << (2 * offset));
    
    return 0;
}

static int stm32_gpio_direction_output(struct gpio_chip *gc, unsigned int offset, int value)
{
    struct stm32_gpio_chip *chip = container_of(gc, struct stm32_gpio_chip, chip);
    GPIO_TypeDef *port = chip->port;
    
    if (offset >= gc->ngpio) {
        return -EINVAL;
    }
    
    /* 设置为输出模式 */
    port->MODER = (port->MODER & ~(3U << (2 * offset))) | (1U << (2 * offset));
    
    /* 设置输出值 */
    if (value) {
        port->BSRR = (1U << offset);
    } else {
        port->BSRR = (1U << (offset + 16));
    }
    
    return 0;
}

static int stm32_gpio_get(struct gpio_chip *gc, unsigned int offset)
{
    struct stm32_gpio_chip *chip = container_of(gc, struct stm32_gpio_chip, chip);
    GPIO_TypeDef *port = chip->port;
    
    if (offset >= gc->ngpio) {
        return -EINVAL;
    }
    
    /* 读取输入数据寄存器 */
    return (port->IDR >> offset) & 1;
}

static void stm32_gpio_set(struct gpio_chip *gc, unsigned int offset, int value)
{
    struct stm32_gpio_chip *chip = container_of(gc, struct stm32_gpio_chip, chip);
    GPIO_TypeDef *port = chip->port;
    
    if (offset >= gc->ngpio) {
        return;
    }
    
    /* 设置输出值 */
    if (value) {
        port->BSRR = (1U << offset);
    } else {
        port->BSRR = (1U << (offset + 16));
    }
}

static int stm32_gpio_set_config(struct gpio_chip *gc, unsigned int offset, unsigned long config)
{
    struct stm32_gpio_chip *chip = container_of(gc, struct stm32_gpio_chip, chip);
    GPIO_TypeDef *port = chip->port;
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    uint32_t speed, pull, drive;
    uint32_t mode;

    if (offset >= gc->ngpio)
        return -EINVAL;

    mode = (port->MODER >> (2 * offset)) & 3;
    
    GPIO_InitStruct.Pin = (1U << offset);
    
    speed = (config & GPIO_CFG_SPEED_MASK) >> GPIO_CFG_SPEED_SHIFT;
    switch (speed) {
        case GPIO_CFG_SPEED_LOW:
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
            break;
        case GPIO_CFG_SPEED_MEDIUM:
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
            break;
        case GPIO_CFG_SPEED_HIGH:
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
            break;
        default:
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
            break;
    }
    
    pull = (config & GPIO_CFG_PULL_MASK) >> GPIO_CFG_PULL_SHIFT;
    switch (pull) {
        case GPIO_CFG_PULL_NONE:
            GPIO_InitStruct.Pull = GPIO_NOPULL;
            break;
        case GPIO_CFG_PULL_UP:
            GPIO_InitStruct.Pull = GPIO_PULLUP;
            break;
        case GPIO_CFG_PULL_DOWN:
            GPIO_InitStruct.Pull = GPIO_PULLDOWN;
            break;
        default:
            GPIO_InitStruct.Pull = GPIO_NOPULL;
            break;
    }
    
    drive = (config & GPIO_CFG_DRIVE_MASK) >> GPIO_CFG_DRIVE_SHIFT;
    if (drive == GPIO_CFG_DRIVE_OPEN_DRAIN) {
        if (mode == 0) {
            GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        } else if (mode == 1) {
            GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
        } else {
            GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        }
    } else {
        if (mode == 0) {
            GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        } else if (mode == 1) {
            GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        } else {
            GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        }
    }
    
    HAL_GPIO_Init(port, &GPIO_InitStruct);
    
    return 0;
}

static int stm32_to_irq(struct gpio_chip *gc, unsigned int offset)
{
    struct stm32_gpio_chip *chip = container_of(gc, struct stm32_gpio_chip, chip);
    GPIO_TypeDef *port = chip->port;
    uint16_t pin = (1U << offset);
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    if (offset >= gc->ngpio) {
        return -EINVAL;
    }
    
    /* 配置GPIO为中断模式 */
    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;  /* 默认上升沿触发 */
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(port, &GPIO_InitStruct);
    
    /* 根据GPIO端口和引脚计算中断号 */
    /* STM32H7系列的EXTI中断号从EXTI0_IRQn到EXTI15_IRQn */
    /* 每个引脚对应一个中断号 */
    return EXTI0_IRQn + offset;
}

static int stm32_gpio_irq_set_type(struct gpio_chip *gc, unsigned int offset, unsigned int type)
{
    struct stm32_gpio_chip *chip = container_of(gc, struct stm32_gpio_chip, chip);
    GPIO_TypeDef *port = chip->port;
    uint32_t trigger = 0;
    
    if (offset >= gc->ngpio) {
        return -EINVAL;
    }
    
    /* 配置EXTI触发方式 */
    switch (type) {
        case GPIO_INT_TYPE_EDGE_RISING:
            trigger = GPIO_MODE_IT_RISING;
            break;
        case GPIO_INT_TYPE_EDGE_FALLING:
            trigger = GPIO_MODE_IT_FALLING;
            break;
        case GPIO_INT_TYPE_EDGE_BOTH:
            trigger = GPIO_MODE_IT_RISING_FALLING;
            break;
        case GPIO_INT_TYPE_LEVEL_HIGH:
        case GPIO_INT_TYPE_LEVEL_LOW:
            /* STM32的EXTI不支持电平触发 */
            return -EINVAL;
        default:
            return -EINVAL;
    }
    
    /* 这里需要配置EXTI和SYSCFG，但需要HAL库的支持 */
    /* 由于我们使用HAL库，这里暂时返回成功 */
    /* 实际实现需要调用HAL_GPIO_Init函数 */
    
    return 0;
}

static int stm32_gpio_irq_enable(struct gpio_chip *gc, unsigned int offset)
{
    struct stm32_gpio_chip *chip = container_of(gc, struct stm32_gpio_chip, chip);
    
    if (offset >= gc->ngpio) {
        return -EINVAL;
    }
    
    /* 启用GPIO中断 */
    /* 这里需要使用HAL库来启用中断 */
    /* 实际实现需要调用HAL_NVIC_EnableIRQ函数 */
    
    return 0;
}

static int stm32_gpio_irq_disable(struct gpio_chip *gc, unsigned int offset)
{
    struct stm32_gpio_chip *chip = container_of(gc, struct stm32_gpio_chip, chip);
    
    if (offset >= gc->ngpio) {
        return -EINVAL;
    }
    
    /* 禁用GPIO中断 */
    /* 这里需要使用HAL库来禁用中断 */
    /* 实际实现需要调用HAL_NVIC_DisableIRQ函数 */
    
    return 0;
}

static int stm32_gpio_irq_request(struct gpio_chip *gc, unsigned int offset,
                                   gpio_irq_handler_t handler, void *data)
{
    struct stm32_gpio_chip *chip = container_of(gc, struct stm32_gpio_chip, chip);
    
    if (offset >= gc->ngpio) {
        return -EINVAL;
    }
    
    if (!handler) {
        return -EINVAL;
    }
    
    /* 请求GPIO中断 */
    /* 这里需要使用HAL库来请求中断 */
    /* 实际实现需要调用HAL_GPIO_Init和HAL_NVIC_SetPriority函数 */
    
    return 0;
}

static int stm32_gpio_irq_free(struct gpio_chip *gc, unsigned int offset)
{
    struct stm32_gpio_chip *chip = container_of(gc, struct stm32_gpio_chip, chip);
    
    if (offset >= gc->ngpio) {
        return -EINVAL;
    }
    
    /* 释放GPIO中断 */
    /* 这里需要使用HAL库来释放中断 */
    /* 实际实现需要调用HAL_GPIO_DeInit函数 */
    
    return 0;
}

static struct gpio_irq_chip stm32_gpio_irq_chip = {
    .irq_set_type = stm32_gpio_irq_set_type,
    .irq_enable = stm32_gpio_irq_enable,
    .irq_disable = stm32_gpio_irq_disable,
    .irq_request = stm32_gpio_irq_request,
    .irq_free = stm32_gpio_irq_free,
};


void stm32_gpio_init(struct device *dev)
{
    struct gpio_device *_dev = container_of(dev, struct gpio_device, dev);
    struct gpio_chip *chip = _dev->chip;
    struct stm32_gpio_chip *stm32 = (struct stm32_gpio_chip *)chip;

    stm32->mask = 0;

    _dev->ngpio = chip->ngpio;
    _dev->base = chip->base;

    switch(chip->label[1]) {
        case 'A':
            __HAL_RCC_GPIOA_CLK_ENABLE();
            break;
        case 'B':
            __HAL_RCC_GPIOB_CLK_ENABLE();
            break;
        case 'C':
            __HAL_RCC_GPIOC_CLK_ENABLE();
            break;
        case 'D':
            __HAL_RCC_GPIOD_CLK_ENABLE();
            break;
        case 'E':
            __HAL_RCC_GPIOE_CLK_ENABLE();
            break;
        case 'F':
            __HAL_RCC_GPIOF_CLK_ENABLE();
            break;
        case 'H':
            __HAL_RCC_GPIOH_CLK_ENABLE();
            break;
        case 'I':
            __HAL_RCC_GPIOI_CLK_ENABLE();
            break;
        case 'J':
            __HAL_RCC_GPIOJ_CLK_ENABLE();
            break;
    }

    gpio_device_register(_dev);
}

