#include <bus.h>
#include <device/gpio/gpio.h>
#include <device/driver.h>
#include <list.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <shell.h>

static struct list_head gpio_device_list = LIST_HEAD_INIT(gpio_device_list);
static struct list_head gpio_chip_list = LIST_HEAD_INIT(gpio_chip_list);
static int gpio_device_id = 0;

int desc_to_gpio(const struct gpio_desc *desc)
{
    return desc->gdev->base + (desc - &desc->gdev->descs[0]);
}

struct gpio_chip *gpiod_to_chip(const struct gpio_desc *desc)
{
    if (!desc || !desc->gdev) {
        return NULL;
    }

    return desc->gdev->chip;
}

static int gpio_chip_probe(struct device *dev)
{
    struct gpio_device *gdev;
    struct gpio_chip *chip;
    
    gdev = container_of(dev, struct gpio_device, dev);
    
    chip = gdev->chip;
    if (!chip) {
        return -ENODEV;
    }
    
    if (chip->request) {
        int i;
        for (i = 0; i < chip->ngpio; i++) {
            int ret = chip->request(chip, i);
            if (ret != 0) {
                while (i--) {
                    if (chip->free)
                        chip->free(chip, i);
                }
                return -ENODEV;
            }
        }
    }
    
    return 0;
}

static int gpio_chip_remove(struct device *dev)
{
    struct gpio_device *gdev;
    struct gpio_chip *chip;
    
    gdev = container_of(dev, struct gpio_device, dev);

    chip = gdev->chip;
    if (!chip) {
        return -ENODEV;
    }
    
    if (chip->free) {
        int i;
        for (i = 0; i < chip->ngpio; i++) {
            chip->free(chip, i);
        }
    }
    
    return 0;
}

static int gpio_chip_match(struct device *dev, struct device_driver *drv)
{
    const struct device_match_table *ptr;
    
    if (!dev || !drv) {
        return 0;
    }
        
    for (ptr = drv->match_ptr; ptr && ptr->compatible; ptr++) {
        if (dev->init_name && strcmp(dev->init_name, ptr->compatible) == 0) {
            dev->driver = drv;
            return 1;
        }
    }
    
    if (dev->init_name && drv->name) {
        if (strcmp(dev->init_name, drv->name) == 0) {
            dev->driver = drv;
            return 1;
        }
    }
    
    return 0;
}

static struct bus_type gpio_bus_type = {
    .name = "gpio",
    .probe = gpio_chip_probe,
    .remove = gpio_chip_remove,
    .match = gpio_chip_match,
};

int gpio_device_register(struct gpio_device *gdev)
{
    int ret;
    
    if (gdev->ngpio <= 0) {
        return -EINVAL;
    }

    gdev->descs = calloc(gdev->ngpio, sizeof(struct gpio_desc));
    if (!gdev->descs) {
        return -ENOMEM;
    }
        
    for (int i = 0; i < gdev->ngpio; i++) {
        gdev->descs[i].gdev = gdev;
        gdev->descs[i].flags = GPIOF_IN;
        gdev->descs[i].irq = -1;
        if (gdev->chip->names && gdev->chip->names[i])
            gdev->descs[i].name = gdev->chip->names[i];
    }
    
    gdev->id = gpio_device_id++;
    
    INIT_LIST_HEAD(&gdev->pin_ranges);
    
    gdev->dev.bus = &gpio_bus_type;
    
    ret = device_register(&gdev->dev);
    if (ret) {
        goto err_free_descs;
    }
        
    list_add_tail(&gdev->list, &gpio_device_list);
    
    return 0;
    
err_free_descs:
    free(gdev->descs);
    gdev->descs = NULL;
    return ret;
}

void gpio_device_unregister(struct gpio_device *gdev)
{
    if (!gdev)
        return;
        
    list_del(&gdev->list);
    
    if (gdev->descs)
        free(gdev->descs);
        
}

struct gpio_desc *gpio_to_desc(unsigned int gpio)
{
    struct gpio_device *gdev;
    
    list_for_each_entry(gdev, &gpio_device_list, list) {
        if (gpio >= gdev->base && gpio < gdev->base + gdev->ngpio) {
            return &gdev->descs[gpio - gdev->base];
        }
    }
    
    return NULL;
}

struct gpio_desc *gpiochip_get_desc(struct gpio_chip *gc, unsigned int offset)
{
    if (!gc || offset >= gc->ngpio)
        return NULL;
        
    if (!gc->gpiodev || !gc->gpiodev->descs)
        return NULL;
        
    return &gc->gpiodev->descs[offset];
}

struct gpio_desc *gpiochip_request_own_desc(struct gpio_chip *gc, unsigned int offset,
                                           const char *label)
{
    struct gpio_desc *desc;
    int ret;
    
    if (!gc || offset >= gc->ngpio)
        return NULL;
        
    desc = gpiochip_get_desc(gc, offset);
    if (!desc)
        return NULL;
        
    if (gc->request) {
        ret = gc->request(gc, offset);
        if (ret)
            return NULL;
    }
    
    desc->label = label;
    
    return desc;
}

void gpiochip_free_own_desc(struct gpio_desc *desc)
{
    struct gpio_chip *gc;
    
    if (!desc)
        return;
        
    gc = gpiod_to_chip((const struct gpio_desc *)desc);
    if (!gc)
        return;
        
    if (gc->free) {
        unsigned int offset = desc - &desc->gdev->descs[0];
        gc->free(gc, offset);
    }
    
    desc->label = NULL;
}

int gpiod_direction_input(struct gpio_desc *desc)
{
    struct gpio_chip *gc;
    unsigned int offset;
    int ret;
        
    gc = gpiod_to_chip(desc);
    if (!gc) {
        return -ENODEV;
    }
        
    offset = desc - &desc->gdev->descs[0];
    
    ret = gc->direction_input(gc, offset);
    if (ret < 0) {
        return ret;
    }
    
    desc->flags &= ~GPIOF_DIR_MASK;
    desc->flags |= GPIOF_DIR_IN;
    
    return 0;
}

int gpiod_direction_output(struct gpio_desc *desc, int value)
{
    struct gpio_chip *gc;
    unsigned int offset;
    int ret;
    
    if (value != GPIO_VALUE_LOW && value != GPIO_VALUE_HIGH) {
        return -EINVAL;
    }
        
    gc = gpiod_to_chip(desc);
    if (!gc) {
        return -ENODEV;
    }
        
    offset = desc - &desc->gdev->descs[0];
    
    ret = gc->direction_output(gc, offset, value);
    if (ret < 0) {
        return ret;
    }
    
    desc->flags &= ~GPIOF_DIR_MASK;
    desc->flags |= GPIOF_DIR_OUT;
    if (value == GPIO_VALUE_HIGH)
        desc->flags |= GPIOF_OUT_INIT_HIGH;
    else
        desc->flags |= GPIOF_OUT_INIT_LOW;
    
    return 0;
}

int gpiod_get_direction(const struct gpio_desc *desc)
{
    struct gpio_chip *gc;
    unsigned int offset;
        
    gc = gpiod_to_chip(desc);
    if (!gc) {
        return -ENODEV;
    }
        
    offset = desc - &desc->gdev->descs[0];
    
    if (gc->get_direction)
        return gc->get_direction(gc, offset);
        
    if (desc->flags & GPIOF_DIR_IN)
        return GPIO_DIRECTION_IN;
    else if (desc->flags & GPIOF_DIR_OUT)
        return GPIO_DIRECTION_OUT;
        
    return -ENODEV;
}

int gpiod_get_value(const struct gpio_desc *desc)
{
    struct gpio_chip *gc;
    unsigned int offset;
    int value;
        
    gc = gpiod_to_chip(desc);
    if (!gc) {
        return -ENODEV;
    }
        
    offset = desc - &desc->gdev->descs[0];
    
    value = gc->get(gc, offset);
    if (value < 0) {
        return value;
    }
    
    return value;
}

void gpiod_set_value(struct gpio_desc *desc, int value)
{
    struct gpio_chip *gc;
    unsigned int offset;
    
    if (!desc) {
        return;
    }
    
    if (value != GPIO_VALUE_LOW && value != GPIO_VALUE_HIGH) {
        return;
    }
        
    gc = gpiod_to_chip(desc);
    if (!gc) {
        return;
    }
    
    if (!gc->set) {
        return;
    }
        
    offset = desc - &desc->gdev->descs[0];
    if (offset >= gc->ngpio) {
        return;
    }
    
    gc->set(gc, offset, value);
}

int gpiod_set_config(struct gpio_desc *desc, unsigned long config)
{
    struct gpio_chip *gc;
    unsigned int offset;
    int ret;
        
    gc = gpiod_to_chip(desc);
    if (!gc) {
        return -ENODEV;
    }
        
    offset = desc - &desc->gdev->descs[0];
    
    if (gc->set_config) {
        ret = gc->set_config(gc, offset, config);
        if (ret < 0) {
            return ret;
        }
        return 0;
    }
        
    return -ENODEV;
}

int gpiod_to_irq(const struct gpio_desc *desc)
{
    struct gpio_chip *gc;
    unsigned int offset;
    int irq;
    
    gc = gpiod_to_chip(desc);
    if (!gc) {
        return -ENODEV;
    }
        
    offset = desc - &desc->gdev->descs[0];
    
    irq = gc->to_irq(gc, offset);
    if (irq < 0) {
        return irq;
    }
    
    ((struct gpio_desc *)desc)->irq = irq;
    
    return irq;
}

int gpiod_irq_request(struct gpio_desc *desc, gpio_irq_handler_t handler, void *data)
{
    struct gpio_chip *gc;
    struct gpio_irq_chip *irq_chip;
    unsigned int offset;
    int ret;
    
    gc = gpiod_to_chip(desc);
    if (!gc) {
        return -ENODEV;
    }
    
    if (!gc->irq_chip) {
        return -ENODEV;
    }
        
    irq_chip = gc->irq_chip;
    if (!irq_chip->irq_request) {
        return -ENODEV;
    }
        
    offset = desc - &desc->gdev->descs[0];
    
    ret = irq_chip->irq_request(gc, offset, handler, data);
    if (ret < 0) {
        return ret;
    }
    
    desc->data = data;
    
    return 0;
}

void gpiod_irq_free(struct gpio_desc *desc)
{
    struct gpio_chip *gc;
    struct gpio_irq_chip *irq_chip;
    unsigned int offset;
    
    if (!desc) {
        return;
    }
        
    gc = gpiod_to_chip(desc);
    if (!gc) {
        return;
    }
    
    if (!gc->irq_chip) {
        return;
    }
        
    irq_chip = gc->irq_chip;
    if (!irq_chip->irq_free) {
        return;
    }
        
    offset = desc - &desc->gdev->descs[0];
    if (offset >= gc->ngpio) {
        return;
    }
    
    irq_chip->irq_free(gc, offset);
    
    desc->data = NULL;
    desc->irq = -1;
}

int gpiod_irq_set_type(struct gpio_desc *desc, unsigned int type)
{
    struct gpio_chip *gc;
    struct gpio_irq_chip *irq_chip;
    unsigned int offset;
    int ret;
    
    if (type > GPIO_INT_TYPE_LEVEL_LOW) {
        return -EINVAL;
    }
        
    gc = gpiod_to_chip(desc);
    if (!gc) {
        return -ENODEV;
    }
    
    if (!gc->irq_chip) {
        return -ENODEV;
    }
        
    irq_chip = gc->irq_chip;
    if (!irq_chip->irq_set_type) {
        return -ENODEV;
    }
        
    offset = desc - &desc->gdev->descs[0];
    
    ret = irq_chip->irq_set_type(gc, offset, type);
    if (ret < 0) {
        return ret;
    }
    
    return 0;
}

int gpiod_irq_enable(struct gpio_desc *desc)
{
    struct gpio_chip *gc;
    struct gpio_irq_chip *irq_chip;
    unsigned int offset;
    int ret;
    
    gc = gpiod_to_chip(desc);
    if (!gc) {
        return -ENODEV;
    }
    
    if (!gc->irq_chip) {
        return -ENODEV;
    }
        
    irq_chip = gc->irq_chip;
    if (!irq_chip->irq_enable) {
        return -ENODEV;
    }
        
    offset = desc - &desc->gdev->descs[0];
    
    ret = irq_chip->irq_enable(gc, offset);
    if (ret < 0) {
        return ret;
    }
    
    return 0;
}

int gpiod_irq_disable(struct gpio_desc *desc)
{
    struct gpio_chip *gc;
    struct gpio_irq_chip *irq_chip;
    unsigned int offset;
    int ret;
    
    gc = gpiod_to_chip(desc);
    if (!gc) {
        return -ENODEV;
    }
    
    if (!gc->irq_chip) {
        return -ENODEV;
    }
        
    irq_chip = gc->irq_chip;
    if (!irq_chip->irq_disable) {
        return -ENODEV;
    }
        
    offset = desc - &desc->gdev->descs[0];
    
    ret = irq_chip->irq_disable(gc, offset);
    if (ret < 0) {
        return ret;
    }
    
    return 0;
}

int gpio_consumer_add(struct gpio_desc *desc, const char *label)
{
    struct gpio_consumer *consumer;
    
    if (!desc || !label)
        return -EINVAL;
        
    consumer = malloc(sizeof(struct gpio_consumer));
    if (!consumer)
        return -ENOMEM;
        
    consumer->desc = desc;
    consumer->label = label;
    
    /* 这里应该将消费者添加到GPIO描述符的消费者列表中 */
    /* 但是目前GPIO描述符结构体中没有消费者列表字段 */
    /* 可以在后续版本中添加 */
    
    return 0;
}

void gpio_consumer_remove(struct gpio_desc *desc, const char *label)
{
    /* 这里应该从GPIO描述符的消费者列表中移除指定消费者 */
    /* 但是目前GPIO描述符结构体中没有消费者列表字段 */
    /* 可以在后续版本中添加 */
    (void)desc;
    (void)label;
}

/* GPIO调试和测试接口 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

static struct option gpio_options[] = {
    {"list", no_argument, 0, 'l'},
    {"info", required_argument, 0, 'i'},
    {"read", required_argument, 0, 'r'},
    {"write", required_argument, 0, 'w'},
    {"dir", required_argument, 0, 'd'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
};

const char *gpio_help_str[] = {
    "list all gpio devices",
    "show gpio info <gpio_num>",
    "read gpio value <gpio_num>",
    "write gpio value <gpio_num> <value>",
    "set gpio direction <gpio_num> <in|out>",
    "show this help message"
};

static void gpio_print_direction(int direction)
{
    switch (direction) {
        case GPIO_DIRECTION_IN:
            shell_printf("input");
            break;
        case GPIO_DIRECTION_OUT:
            shell_printf("output");
            break;
        default:
            shell_printf("unknown");
            break;
    }
}

static void gpio_print_value(int value)
{
    switch (value) {
        case GPIO_VALUE_LOW:
            shell_printf("low");
            break;
        case GPIO_VALUE_HIGH:
            shell_printf("high");
            break;
        default:
            shell_printf("unknown");
            break;
    }
}

static int do_gpio_list(void)
{
    struct gpio_device *gdev;
    
    shell_printf("GPIO Devices:\r\n");
    shell_printf("------------\r\n");
    
    list_for_each_entry(gdev, &gpio_device_list, list) {
        shell_printf("Device: %s\r\n", gdev->dev.init_name ? gdev->dev.init_name : "unknown");
        shell_printf("  ID: %d\r\n", gdev->id);
        shell_printf("  Base: %d\r\n", gdev->base);
        shell_printf("  NGPIO: %d\r\n", gdev->ngpio);
        shell_printf("  Chip: %s\r\n", gdev->chip->label ? gdev->chip->label : "unknown");
        shell_printf("\r\n");
    }
    
    return 0;
}

static int do_gpio_info(int gpio_num)
{
    struct gpio_desc *desc;
    int direction, value;
    
    desc = gpio_to_desc(gpio_num);
    if (!desc) {
        shell_printf("GPIO %d not found\r\n", gpio_num);
        return -EINVAL;
    }
    
    shell_printf("GPIO %d Info:\r\n", gpio_num);
    shell_printf("------------\r\n");
    shell_printf("Name: %s\r\n", desc->name ? desc->name : "unknown");
    shell_printf("Label: %s\r\n", desc->label ? desc->label : "none");
    
    direction = gpiod_get_direction(desc);
    if (direction >= 0) {
        shell_printf("Direction: ");
        gpio_print_direction(direction);
        shell_printf("\r\n");
    }
    
    if (direction == GPIO_DIRECTION_IN) {
        value = gpiod_get_value(desc);
        if (value >= 0) {
            shell_printf("Value: ");
            gpio_print_value(value);
            shell_printf("\r\n");
        }
    }
    
    return 0;
}

static int do_gpio_read(int gpio_num)
{
    struct gpio_desc *desc;
    int value;
    
    desc = gpio_to_desc(gpio_num);
    if (!desc) {
        shell_printf("GPIO %d not found\r\n", gpio_num);
        return -EINVAL;
    }
    
    value = gpiod_get_value(desc);
    if (value < 0) {
        shell_printf("Failed to read GPIO %d\r\n", gpio_num);
        return value;
    }
    
    shell_printf("GPIO %d: ", gpio_num);
    gpio_print_value(value);
    shell_printf("\r\n");
    
    return 0;
}

static int do_gpio_write(int gpio_num, const char *value_str)
{
    struct gpio_desc *desc;
    int value;
    
    desc = gpio_to_desc(gpio_num);
    if (!desc) {
        shell_printf("GPIO %d not found\r\n", gpio_num);
        return -EINVAL;
    }
    
    value = atoi(value_str);
    if (value != GPIO_VALUE_LOW && value != GPIO_VALUE_HIGH) {
        shell_printf("Invalid value: %s\r\n", value_str);
        return -EINVAL;
    }
    
    gpiod_set_value(desc, value);
    
    shell_printf("GPIO %d set to ", gpio_num);
    gpio_print_value(value);
    shell_printf("\r\n");
    
    return 0;
}

static int do_gpio_dir(int gpio_num, const char *dir_str)
{
    struct gpio_desc *desc;
    int ret;
    
    desc = gpio_to_desc(gpio_num);
    if (!desc) {
        shell_printf("GPIO %d not found\r\n", gpio_num);
        return -EINVAL;
    }
    
    if (strcmp(dir_str, "in") == 0) {
        ret = gpiod_direction_input(desc);
    } else if (strcmp(dir_str, "out") == 0) {
        ret = gpiod_direction_output(desc, GPIO_VALUE_LOW);
    } else {
        shell_printf("Invalid direction: %s\r\n", dir_str);
        return -EINVAL;
    }
    
    if (ret < 0) {
        shell_printf("Failed to set GPIO %d direction\r\n", gpio_num);
        return ret;
    }
    
    shell_printf("GPIO %d direction set to %s\r\n", gpio_num, dir_str);
    
    return 0;
}

static int show_help(void)
{
    int i;
    
    shell_printf("GPIO utility commands:\r\n");
    shell_printf("---------------------\r\n");
    
    for (i = 0; i < ARRAY_SIZE(gpio_options) - 1; i++) {
        shell_printf("  --%-10s - %s\r\n", gpio_options[i].name, gpio_help_str[i]);
    }
    
    return 0;
}

static int do_gpio(int argc, char *argv[])
{
    int c;
    int opt_ind;
    
    if (argc < 2) {
        return show_help();
    }
    
    while ((c = getopt_long(argc, argv, "li:r:w:d:h", gpio_options, &opt_ind)) != -1) {
        switch (c) {
            case 'l':
                return do_gpio_list();
                
            case 'i':
                if (optind >= argc) {
                    shell_printf("Missing GPIO number\r\n");
                    return -EINVAL;
                }
                return do_gpio_info(atoi(optarg));
                
            case 'r':
                if (optind >= argc) {
                    shell_printf("Missing GPIO number\r\n");
                    return -EINVAL;
                }
                return do_gpio_read(atoi(optarg));
                
            case 'w':
                if (optind >= argc) {
                    shell_printf("Missing GPIO number or value\r\n");
                    return -EINVAL;
                }
                return do_gpio_write(atoi(optarg), argv[optind]);
                
            case 'd':
                if (optind >= argc) {
                    shell_printf("Missing GPIO number or direction\r\n");
                    return -EINVAL;
                }
                return do_gpio_dir(atoi(optarg), argv[optind]);
                
            case 'h':
                return show_help();
                
            case 0:
                switch (opt_ind) {
                    case 0: /* list */
                        return do_gpio_list();
                    case 1: /* info */
                        if (optind >= argc) {
                            shell_printf("Missing GPIO number\r\n");
                            return -EINVAL;
                        }
                        return do_gpio_info(atoi(optarg));
                    case 2: /* read */
                        if (optind >= argc) {
                            shell_printf("Missing GPIO number\r\n");
                            return -EINVAL;
                        }
                        return do_gpio_read(atoi(optarg));
                    case 3: /* write */
                        if (optind >= argc) {
                            shell_printf("Missing GPIO number or value\r\n");
                            return -EINVAL;
                        }
                        return do_gpio_write(atoi(optarg), argv[optind]);
                    case 4: /* dir */
                        if (optind >= argc) {
                            shell_printf("Missing GPIO number or direction\r\n");
                            return -EINVAL;
                        }
                        return do_gpio_dir(atoi(optarg), argv[optind]);
                    case 5: /* help */
                        return show_help();
                }
                break;
                
            default:
                return show_help();
        }
    }
    
    return show_help();
}

static void gpio_cmd_help(int argc, char *argv[])
{
    char *cmd;
    int i;
    
    if (argc != 2)
        return;
        
    cmd = argv[1];
    
    for (i = 0; i < ARRAY_SIZE(gpio_options); i++) {
        if (strcmp(cmd, gpio_options[i].name) == 0) {
            shell_printf("%s - %s\r\n", gpio_options[i].name, gpio_help_str[i]);
            break;
        }
    }
}

shell_command_register(gpio, "GPIO utility", do_gpio, gpio_cmd_help);
