#include <device/i2c/mpu6050.h>
#include <device/gpio/gpio.h>
struct mpu6050_data {
    struct i2c_driver drv;
};

static int mpu6050_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct mpu6050_device *mpu6050 = to_mpu6050(client);
    if (mpu6050->intr_gpio_name)
    {
        client->intr_gpio = gpiod_lookup_byname(mpu6050->intr_gpio_name, GPIO_CFG_MODE_INPUT | GPIO_CFG_SPEED_HIGH);
    }
    return 0;
}

static int mpu6050_remove(struct i2c_client *client)
{
    return 0;
}

static void mpu6050_driver_init(struct device_driver *drv)
{
    struct i2c_driver *_drv = to_i2c_driver(drv);
    i2c_register_driver(_drv);
}

static const struct i2c_device_id  mpu6050_ids[] = {
    {
        .name = "mpu6050"
    },
    {

    }
};

static const struct device_match_table mpu6050_match_ptr[] = {
    {
        .compatible = "mpu6050-device",
    },
    {

    }
};

static struct mpu6050_data mpu6050_drv = {
    .drv = {
        .drv = {
            .bus = &i2c_bus_type,
            .init = mpu6050_driver_init,
            .name = "mpu6050-driver",
            .match_ptr = mpu6050_match_ptr,
        },
        .probe = mpu6050_probe,
        .remove = mpu6050_remove,
        .match_table = mpu6050_ids,
    }
};

register_driver(mpu6050, mpu6050_drv.drv.drv);
