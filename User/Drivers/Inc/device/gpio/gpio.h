#pragma once

#include <device/device.h>
#include <device/driver.h>

#include <stdint.h>

/* GPIO标志位定义 */
#define GPIOF_IN          0x0000
#define GPIOF_OUT_INIT_LOW  0x0001
#define GPIOF_OUT_INIT_HIGH 0x0002
#define GPIOF_IN_INIT      0x0004
#define GPIOF_DIR_IN       0x0008
#define GPIOF_DIR_OUT      0x0010
#define GPIOF_DIR_MASK     (GPIOF_DIR_IN | GPIOF_DIR_OUT)
#define GPIOF_ACTIVE_LOW   0x0020
#define GPIOF_OPEN_DRAIN   0x0040
#define GPIOF_OPEN_SOURCE  0x0080
#define GPIOF_PULL_UP      0x0100
#define GPIOF_PULL_DOWN    0x0200
#define GPIOF_PULL_MASK    (GPIOF_PULL_UP | GPIOF_PULL_DOWN)

/* GPIO配置标志位 */
#define GPIO_CFG_SPEED_LOW     0x00000001
#define GPIO_CFG_SPEED_MEDIUM  0x00000002
#define GPIO_CFG_SPEED_HIGH    0x00000003
#define GPIO_CFG_SPEED_MASK    0x00000003
#define GPIO_CFG_SPEED_SHIFT   0

#define GPIO_CFG_PULL_NONE     0x00000000
#define GPIO_CFG_PULL_UP       0x00000004
#define GPIO_CFG_PULL_DOWN     0x00000008
#define GPIO_CFG_PULL_MASK     0x0000000C
#define GPIO_CFG_PULL_SHIFT    2

#define GPIO_CFG_DRIVE_PUSH_PULL   0x00000000
#define GPIO_CFG_DRIVE_OPEN_DRAIN  0x00000010
#define GPIO_CFG_DRIVE_OPEN_SOURCE 0x00000020
#define GPIO_CFG_DRIVE_MASK        0x00000030
#define GPIO_CFG_DRIVE_SHIFT       4

/* GPIO方向定义 */
#define GPIO_DIRECTION_IN   0
#define GPIO_DIRECTION_OUT  1

/* GPIO值定义 */
#define GPIO_VALUE_LOW     0
#define GPIO_VALUE_HIGH    1

/* GPIO中断类型 */
#define GPIO_INT_TYPE_NONE         0x00
#define GPIO_INT_TYPE_EDGE_RISING  0x01
#define GPIO_INT_TYPE_EDGE_FALLING 0x02
#define GPIO_INT_TYPE_EDGE_BOTH    0x03
#define GPIO_INT_TYPE_LEVEL_HIGH   0x04
#define GPIO_INT_TYPE_LEVEL_LOW    0x05

struct gpio_desc;
struct gpio_device;
struct gpio_irq_chip;
struct gpio_chip;

/* GPIO中断回调函数类型 */
typedef void (*gpio_irq_handler_t)(void *data);

/* GPIO中断芯片结构 */
struct gpio_irq_chip {
    int (*irq_set_type)(struct gpio_chip *gc, unsigned int offset, unsigned int type);
    int (*irq_enable)(struct gpio_chip *gc, unsigned int offset);
    int (*irq_disable)(struct gpio_chip *gc, unsigned int offset);
    int (*irq_request)(struct gpio_chip *gc, unsigned int offset, gpio_irq_handler_t handler, void *data);
    int (*irq_free)(struct gpio_chip *gc, unsigned int offset);
    void *parent_irq_data;
};

/* GPIO芯片结构 */
struct gpio_chip {
    const char *label;
    struct gpio_device *gpiodev;
    struct device *parent;
    
    /* GPIO基本操作 */
    int (*request)(struct gpio_chip *gc, unsigned int offset);
    void (*free)(struct gpio_chip *gc, unsigned int offset);
    int (*get_direction)(struct gpio_chip *gc, unsigned int offset);
    int (*direction_input)(struct gpio_chip *gc, unsigned int offset);
    int (*direction_output)(struct gpio_chip *gc, unsigned int offset, int value);
    int (*get)(struct gpio_chip *gc, unsigned int offset);
    void (*set)(struct gpio_chip *gc, unsigned int offset, int value);
    int (*set_config)(struct gpio_chip *gc, unsigned int offset, unsigned long config);
    
    /* GPIO中断操作 */
    int (*to_irq)(struct gpio_chip *gc, unsigned int offset);
    struct gpio_irq_chip *irq_chip;
    
    /* GPIO芯片信息 */
    int base;
    uint16_t ngpio;
    uint16_t offset;
    const char *const *names;
    void *data;
};

/* GPIO设备结构 */
struct gpio_device {
    struct device dev;
    struct gpio_chip *chip;
    struct gpio_desc *descs;
    int base;
    int ngpio;
    struct list_head list;
    struct list_head pin_ranges;
    int id;
};

/* GPIO描述符结构 */
struct gpio_desc {
    struct gpio_device *gdev;
    unsigned long flags;
    const char *label;
    const char *name;
    unsigned int irq;
    void *data;
};

/* GPIO消费者结构 */
struct gpio_consumer {
    struct list_head list;
    struct gpio_desc *desc;
    const char *label;
};

/* 工具函数 */
int desc_to_gpio(const struct gpio_desc *desc);
struct gpio_chip *gpiod_to_chip(const struct gpio_desc *desc);

/* GPIO设备管理函数 */
int gpio_device_register(struct gpio_device *gdev);
void gpio_device_unregister(struct gpio_device *gdev);

/* GPIO描述符管理函数 */
struct gpio_desc *gpio_to_desc(unsigned int gpio);
struct gpio_desc *gpiochip_get_desc(struct gpio_chip *gc, unsigned int offset);
struct gpio_desc *gpiochip_request_own_desc(struct gpio_chip *gc, unsigned int offset,
                                           const char *label);
void gpiochip_free_own_desc(struct gpio_desc *desc);

/* GPIO基本操作函数 */
int gpiod_direction_input(struct gpio_desc *desc);
int gpiod_direction_output(struct gpio_desc *desc, int value);
int gpiod_get_direction(const struct gpio_desc *desc);
int gpiod_get_value(const struct gpio_desc *desc);
void gpiod_set_value(struct gpio_desc *desc, int value);
int gpiod_set_config(struct gpio_desc *desc, unsigned long config);

/* GPIO中断操作函数 */
int gpiod_to_irq(const struct gpio_desc *desc);
int gpiod_irq_request(struct gpio_desc *desc, gpio_irq_handler_t handler, void *data);
void gpiod_irq_free(struct gpio_desc *desc);
int gpiod_irq_set_type(struct gpio_desc *desc, unsigned int type);
int gpiod_irq_enable(struct gpio_desc *desc);
int gpiod_irq_disable(struct gpio_desc *desc);

/* GPIO消费者管理函数 */
int gpio_consumer_add(struct gpio_desc *desc, const char *label);
void gpio_consumer_remove(struct gpio_desc *desc, const char *label);

