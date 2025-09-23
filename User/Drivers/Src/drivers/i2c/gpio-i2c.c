#include <device/i2c/i2c.h>
#include <device/i2c/gpio-i2c.h>
#include <device/gpio/gpio.h>

#include <gpio.h>

static void delay_us(uint32_t us)
{
    extern uint32_t SystemCoreClock;
    uint32_t count = us * (SystemCoreClock / 1000000 / 8);
    while (count--) {
        __NOP();
    }
}

static void scl_high(struct gpio_i2c_data *data)
{
    gpiod_set_value(data->scl, 1);
}

static void scl_low(struct gpio_i2c_data *data)
{
     gpiod_set_value(data->scl, 0);
}

static void sda_high(struct gpio_i2c_data *data)
{
     gpiod_set_value(data->sda, 1);
}

static void sda_low(struct gpio_i2c_data *data)
{
     gpiod_set_value(data->sda, 0);
}

static int sda_read(struct gpio_i2c_data *data)
{
    return gpiod_get_value(data->sda);
}

static int scl_read(struct gpio_i2c_data *data)
{
    return gpiod_get_value(data->scl);
}

static void i2c_start(struct gpio_i2c_data *data)
{
    sda_high(data);
    scl_high(data);
    delay_us(data->delay_us);
    
    sda_low(data);
    delay_us(data->delay_us);
    
    scl_low(data);
    delay_us(data->delay_us);
}

static void i2c_stop(struct gpio_i2c_data *data)
{
    sda_low(data);
    delay_us(data->delay_us);
    
    scl_high(data);
    delay_us(data->delay_us);
    
    sda_high(data);
    delay_us(data->delay_us);
}

static int i2c_wait_ack(struct gpio_i2c_data *data)
{
    int timeout = data->timeout;
    int ack;
    
    sda_high(data);
    delay_us(data->delay_us / 2);
    
    scl_high(data);
    while (scl_read(data) == 0 && timeout--) {
        delay_us(1);
    }
    
    if (timeout <= 0) {
        scl_low(data);
        return -1;
    }
    
    delay_us(data->delay_us / 2);
    
    ack = sda_read(data);
    
    scl_low(data);
    delay_us(data->delay_us);
    
    return ack ? -1 : 0;
}

static void i2c_ack(struct gpio_i2c_data *data)
{
    int timeout = data->timeout;
    
    sda_low(data);
    delay_us(data->delay_us / 2);
    
    while (scl_read(data) == 0 && timeout--) {
        delay_us(1);
    }
    
    if (timeout <= 0) {
        return;
    }
    
    scl_high(data);
    delay_us(data->delay_us);
    
    scl_low(data);
    delay_us(data->delay_us / 2);
    
    sda_high(data);
    delay_us(data->delay_us / 2);
}

static void i2c_nack(struct gpio_i2c_data *data)
{
    int timeout = data->timeout;
    
    sda_high(data);
    delay_us(data->delay_us / 2);
    
    while (scl_read(data) == 0 && timeout--) {
        delay_us(1);
    }
    
    if (timeout <= 0) {
        return;
    }
    
    scl_high(data);
    delay_us(data->delay_us);
    
    scl_low(data);
    delay_us(data->delay_us / 2);
}

static int gpio_i2c_check_bus_busy(struct gpio_i2c_data *data)
{
    int timeout = data->timeout;
    
    while (timeout--) {
        if (sda_read(data) && scl_read(data)) {
            return 0;
        }
        delay_us(10);
    }
    
    return -1;
}

static int i2c_write_byte(struct gpio_i2c_data *data, uint8_t byte)
{
    int i;

    for (i = 7; i >= 0; i--) {
        if (byte & (1 << i)) {
            sda_high(data);
        } else {
            sda_low(data);
        }
        
        delay_us(data->delay_us / 2);
        
        scl_high(data);
        delay_us(data->delay_us);

        scl_low(data);
        delay_us(data->delay_us / 2);
    }

    sda_high(data);
    delay_us(data->delay_us / 2);
    
    return i2c_wait_ack(data);
}

static uint8_t i2c_read_byte(struct gpio_i2c_data *data, int ack)
{
    int i;
    uint8_t byte = 0;

    sda_high(data);
    delay_us(data->delay_us / 2);
    
    for (i = 7; i >= 0; i--) {
        scl_high(data);
        delay_us(data->delay_us / 2);
        
        if (sda_read(data)) {
            byte |= (1 << i);
        }
        
        scl_low(data);
        delay_us(data->delay_us);
    }
    
    if (ack) {
        i2c_ack(data);
    } else {
        i2c_nack(data);
    }
    
    return byte;
}

static int gpio_i2c_master_xfer(struct i2c_adapter *adap, struct i2c_message *msgs, int num)
{
    struct gpio_i2c_data *data = dev_get_drvdata(&adap->dev);
    int i, ret;
    int retries = data->retries;
    
    if (!data || !msgs || num <= 0) {
        return -1;
    }
    
    if (gpio_i2c_check_bus_busy(data) < 0) {
        i2c_stop(data);
        delay_us(100);
        
        if (gpio_i2c_check_bus_busy(data) < 0) {
            return -1;
        }
    }
    
retry:
    for (i = 0; i < num; i++) {
        struct i2c_message *msg = &msgs[i];
        uint8_t addr = msg->addr << 1;
        
        if (msg->flags & I2C_M_RD) {
            addr |= 1;
        }
        
        if (!(msg->flags & I2C_M_NOSTART)) {
            i2c_start(data);
            delay_us(data->delay_us);
        }
        
        ret = i2c_write_byte(data, addr);
        if (ret < 0) {
            i2c_stop(data);
            delay_us(data->delay_us * 10);
            if (retries--) {
                goto retry;
            }
            return ret;
        }

        if (msg->flags & I2C_M_PROBE)
            return 0;
        
        if (msg->flags & I2C_M_RD) {
            int j;
            for (j = 0; j < msg->len; j++) {
                msg->buf[j] = i2c_read_byte(data, j < msg->len - 1);
                delay_us(data->delay_us / 2);
            }
        } else {
            int j;
            for (j = 0; j < msg->len; j++) {
                ret = i2c_write_byte(data, msg->buf[j]);
                if (ret < 0) {
                    i2c_stop(data);
                    delay_us(data->delay_us * 10);
                    if (retries--) {
                        goto retry;
                    }
                    return ret;
                }
                delay_us(data->delay_us / 2);
            }
        }
    }
    
    i2c_stop(data);
    delay_us(data->delay_us * 2);
    
    return num;
}

static int gpio_i2c_smbus_xfer(struct i2c_adapter *adap, unsigned short addr,
                     unsigned short flags, char read_write,
                     unsigned char command, int size, void *data)
{
    struct gpio_i2c_data *i2c_data = (struct gpio_i2c_data *)adap->algo_data;
    int ret;
    uint8_t addr_byte = addr << 1;
    
    if (read_write) {
        addr_byte |= 1;
    }
    
    if (gpio_i2c_check_bus_busy(i2c_data) < 0) {
        i2c_stop(i2c_data);
        delay_us(100);
        
        if (gpio_i2c_check_bus_busy(i2c_data) < 0) {
            return -1;
        }
    }
    
    i2c_start(i2c_data);
    delay_us(i2c_data->delay_us);
    
    ret = i2c_write_byte(i2c_data, addr_byte);
    if (ret < 0) {
        i2c_stop(i2c_data);
        delay_us(i2c_data->delay_us * 10);
        return ret;
    }
    
    if (size != SMBUS_QUICK) {
        ret = i2c_write_byte(i2c_data, command);
        if (ret < 0) {
            i2c_stop(i2c_data);
            delay_us(i2c_data->delay_us * 10);
            return ret;
        }
    }
    
    switch (size) {
    case SMBUS_QUICK:
        break;
        
    case SMBUS_BYTE:
        if (read_write) {
            uint8_t *byte = (uint8_t *)data;
            *byte = i2c_read_byte(i2c_data, 0);
            delay_us(i2c_data->delay_us);
        } else {
            uint8_t byte = *(uint8_t *)data;
            ret = i2c_write_byte(i2c_data, byte);
            if (ret < 0) {
                i2c_stop(i2c_data);
                delay_us(i2c_data->delay_us * 10);
                return ret;
            }
            delay_us(i2c_data->delay_us);
        }
        break;
        
    case SMBUS_BYTE_DATA:
        if (read_write) {
            uint8_t *byte = (uint8_t *)data;

            i2c_start(i2c_data);
            delay_us(i2c_data->delay_us);
            
            ret = i2c_write_byte(i2c_data, addr_byte | 1);
            if (ret < 0) {
                i2c_stop(i2c_data);
                delay_us(i2c_data->delay_us * 10);
                return ret;
            }
            
            *byte = i2c_read_byte(i2c_data, 0);
            delay_us(i2c_data->delay_us);
        } else {
            uint8_t byte = *(uint8_t *)data;
            ret = i2c_write_byte(i2c_data, byte);
            if (ret < 0) {
                i2c_stop(i2c_data);
                delay_us(i2c_data->delay_us * 10);
                return ret;
            }
            delay_us(i2c_data->delay_us);
        }
        break;
        
    case SMBUS_WORD_DATA:
        if (read_write) {
            uint16_t *word = (uint16_t *)data;
            uint8_t low, high;
            
            i2c_start(i2c_data);
            delay_us(i2c_data->delay_us);
            
            ret = i2c_write_byte(i2c_data, addr_byte | 1);
            if (ret < 0) {
                i2c_stop(i2c_data);
                delay_us(i2c_data->delay_us * 10);
                return ret;
            }

            low = i2c_read_byte(i2c_data, 1);
            delay_us(i2c_data->delay_us / 2);
            
            high = i2c_read_byte(i2c_data, 0);
            delay_us(i2c_data->delay_us);
            
            *word = (high << 8) | low;
        } else {
            uint16_t word = *(uint16_t *)data;
            uint8_t low = word & 0xFF;
            uint8_t high = (word >> 8) & 0xFF;
            
            ret = i2c_write_byte(i2c_data, low);
            if (ret < 0) {
                i2c_stop(i2c_data);
                delay_us(i2c_data->delay_us * 10);
                return ret;
            }
            delay_us(i2c_data->delay_us / 2);
            
            ret = i2c_write_byte(i2c_data, high);
            if (ret < 0) {
                i2c_stop(i2c_data);
                delay_us(i2c_data->delay_us * 10);
                return ret;
            }
            delay_us(i2c_data->delay_us);
        }
        break;
        
    case SMBUS_BLOCK_DATA:
        if (read_write) {
            uint8_t *block = (uint8_t *)data;
            uint8_t length;
            int i;
            
            i2c_start(i2c_data);
            delay_us(i2c_data->delay_us);
            
            ret = i2c_write_byte(i2c_data, addr_byte | 1);
            if (ret < 0) {
                i2c_stop(i2c_data);
                delay_us(i2c_data->delay_us * 10);
                return ret;
            }
            
            length = i2c_read_byte(i2c_data, 1);
            delay_us(i2c_data->delay_us / 2);
            
            for (i = 0; i < length; i++) {
                block[i] = i2c_read_byte(i2c_data, i < length - 1);
                delay_us(i2c_data->delay_us / 2);
            }
            delay_us(i2c_data->delay_us);
        } else {
            uint8_t *block = (uint8_t *)data;
            uint8_t length = block[0];
            int i;
            
            ret = i2c_write_byte(i2c_data, length);
            if (ret < 0) {
                i2c_stop(i2c_data);
                delay_us(i2c_data->delay_us * 10);
                return ret;
            }
            delay_us(i2c_data->delay_us / 2);
            
            for (i = 1; i <= length; i++) {
                ret = i2c_write_byte(i2c_data, block[i]);
                if (ret < 0) {
                    i2c_stop(i2c_data);
                    delay_us(i2c_data->delay_us * 10);
                    return ret;
                }
                delay_us(i2c_data->delay_us / 2); 
            }
            delay_us(i2c_data->delay_us);
        }
        break;
        
    default:
        i2c_stop(i2c_data);
        delay_us(i2c_data->delay_us * 2);
        return -1;
    }
    
    i2c_stop(i2c_data);
    delay_us(i2c_data->delay_us * 2);
    
    return 0;
}

static unsigned int gpio_i2c_functionality(struct i2c_adapter *adap)
{
    return I2C_FUNC_I2C | I2C_FUNC_PROTOCOL_MANGLING;
}

static void gpio_i2c_gpio_init(struct gpio_i2c_data *data)
{
    struct gpio_desc *desc;
    unsigned long flags = GPIO_CFG_DRIVE_OPEN_DRAIN | GPIO_CFG_SPEED_HIGH | GPIO_CFG_MODE_OUTPUT;

    desc = gpiod_request_with_label(data->scl_pin_name, flags, "i2c-scl");
    if (!desc)
        return;

    data->scl = desc;

    desc = gpiod_request_with_label(data->sda_pin_name, flags, "i2c-sda");
    if (!desc)
        return;

    data->sda = desc;
    
    scl_high(data);
    sda_high(data);
    
    delay_us(100);
    
    if (data->delay_us == 0) {
        data->delay_us = 5;
    }

    if (data->timeout == 0) {
        data->timeout = 1000;
    }
    
    if (gpio_i2c_check_bus_busy(data) < 0) {
        i2c_stop(data);
        delay_us(100);
    }
}


static const struct i2c_algorithm gpio_i2c_algo = {
    .master_xfer = gpio_i2c_master_xfer,
    .smbus_xfer = gpio_i2c_smbus_xfer,
    .functionality = gpio_i2c_functionality,
};

static int gpio_i2c_adapter_probe(struct device *dev)
{
    struct i2c_adapter *adap = to_i2c_adapter(dev);
    struct gpio_i2c_data *data = dev_get_drvdata(dev);

    gpio_i2c_gpio_init(data);

    adap->algo = &gpio_i2c_algo;

    if (!data->sda || !data->scl)
        return -1;
    
    return 0;
}

static int gpio_i2c_adapter_remove(struct device *dev)
{
    struct i2c_adapter *adap = to_i2c_adapter(dev);

    adap->algo = NULL;

    return 0;
}

static void gpio_i2c_adapter_init(struct device_driver *drv)
{
    driver_register(drv);
}

static const struct device_match_table gpio_i2c_ids[] = {
    {
        .compatible = "gpio-i2c-adapter"
    },
    {
       
    }
};

static struct device_driver gpio_i2c_gpio_i2c_drv = {
    .name = "gpio-i2c-drv",
    .init = gpio_i2c_adapter_init,
    .bus = &i2c_bus_type,
    .match_ptr = gpio_i2c_ids,
    .probe = gpio_i2c_adapter_probe,
    .remove = gpio_i2c_adapter_remove,
};

register_driver(gpio_i2c_gpio_i2c, gpio_i2c_gpio_i2c_drv);