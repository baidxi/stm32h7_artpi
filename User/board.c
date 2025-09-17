#include <shell.h>

#include <board.h>

#include <usart.h>

#include <device/device.h>
#include <device/tty/stm32_uart.h>
#include <device/i2c/i2c.h>
#include <device/i2c/gpio-i2c.h>
#include <device/i2c/mpu6050.h>
#include <device/gpio/stm32_gpio.h>
#include <device/spi/spi.h>

#include <gpio.h>
#include <spi.h>

void StartShellTask (void *argument)
{
  shell_init ("ttyS4", "stm32h7>");
  for (;;)
    {
      shell_run ();
    }
}

osThreadId_t shellTaskHandle;
const osThreadAttr_t shellTask_attrbutes = {
  .name = "shellTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t)osPriorityNormal1,
};

void stm32h7_usart3_init (struct device *dev)
{
  struct tty_device *tty = to_tty_device (dev);
  MX_USART3_UART_Init ();
  tty->baudrate = huart3.Init.BaudRate;
  tty->data_bits = huart3.Init.WordLength;
  tty->parity = huart3.Init.Parity;
  tty->stop_bits = huart3.Init.StopBits;
  dev->private_data = &huart3;

  tty_device_register (tty);
}

void stm32h7_uart4_init (struct device *dev)
{
  struct tty_device *tty = to_tty_device (dev);
  MX_UART4_Init ();
  tty->baudrate = huart4.Init.BaudRate;
  tty->data_bits = huart4.Init.WordLength;
  tty->parity = huart4.Init.Parity;
  tty->stop_bits = huart4.Init.StopBits;
  dev->private_data = &huart4;
  tty_device_register (tty);
}

static void gpio_i2c_preinit(struct device *dev)
{
  struct i2c_adapter *adap = to_i2c_adapter(dev);

    static struct gpio_i2c_data data = {
      .scl_pin_name = "PH11",
      .sda_pin_name = "PH12",
      .delay_us = 3,
      .retries = 3,
      .timeout = 3,
    };

    dev_set_drvdata(dev, &data);

    i2c_add_addapter(adap);
}

static void mpu6050_preinit(struct device *dev)
{
  struct i2c_client *client = to_i2c_device(dev);
  i2c_register_device(client);
}

static struct stm32_uart stm32h7_uart3 = {
    .tty = {
        .dev = {
            .init_name = "stm32-uart",
            .name = "ttyS3",
            .init = stm32h7_usart3_init,
        },
        .port_num = 3,
        .mode = TTY_MODE_STREAM,
    }
};

static struct stm32_uart stm32h7_uart4 = {
.tty = {
    .dev = {
        .init_name = "stm32-uart",
        .name = "ttyS4",
        .init = stm32h7_uart4_init,
    },
    .parity = 4,
    .mode = TTY_MODE_CONSOLE
    }
};

static struct i2c_adapter stm32_gpio_i2c = {
    .dev = {
        .init_name = "gpio-i2c",
        .init = gpio_i2c_preinit,
    }
};

static struct mpu6050_device mpu6050 = {
  .dev = {
    .dev = {
      .init = mpu6050_preinit,
      .init_name = "mpu6050-device"
    },
    .adap = &stm32_gpio_i2c,
    .addr = 0x68,
    .name = "mpu6050"
  }
};

static struct gpio_device stm32_gpioa_device = {
    .dev = {
        .init_name = "stm32-gpio",
        .init = stm32_gpio_init,
    },
    .chip = &stm32_gpioa_chip.chip,
};

static struct gpio_device stm32_gpiob_device = {
    .dev = {
        .init_name = "stm32-gpio",
        .init = stm32_gpio_init,
    },
    .chip = &stm32_gpiob_chip.chip,
};

static struct gpio_device stm32_gpioc_device = {
    .dev = {
        .init_name = "stm32-gpio",
        .init = stm32_gpio_init,
    },
    .chip = &stm32_gpioc_chip.chip,
};

static struct gpio_device stm32_gpiod_device = {
    .dev = {
        .init_name = "stm32-gpio",
        .init = stm32_gpio_init,
    },
    .chip = &stm32_gpiod_chip.chip,
};

static struct gpio_device stm32_gpioe_device = {
    .dev = {
        .init_name = "stm32-gpio",
        .init = stm32_gpio_init,
    },
    .chip = &stm32_gpioe_chip.chip,
};

static struct gpio_device stm32_gpiof_device = {
    .dev = {
        .init_name = "stm32-gpio",
        .init = stm32_gpio_init,
    },
    .chip = &stm32_gpiof_chip.chip,
};

static struct gpio_device stm32_gpioh_device = {
    .dev = {
        .init_name = "stm32-gpio",
        .init = stm32_gpio_init,
    },
    .chip = &stm32_gpioh_chip.chip,
};

static struct gpio_device stm32_gpioi_device = {
    .dev = {
        .init_name = "stm32-gpio",
        .init = stm32_gpio_init,
    },
    .chip = &stm32_gpioi_chip.chip,
};

static struct gpio_device stm32_gpioj_device = {
    .dev = {
        .init_name = "stm32-gpio",
        .init = stm32_gpio_init,
    },
    .chip = &stm32_gpioj_chip.chip,
};

static void stm32_spi_init(struct device *dev)
{
  struct spi_master *master = to_spi_master(dev);
  switch(master->bus_num) {
    case 1:
      MX_SPI1_Init();
      master->mode = hspi1.Init.Mode;
      dev->private_data = &hspi1;
      break;
    case 2:
      MX_SPI2_Init();
      master->mode = hspi2.Init.Mode;
      dev->private_data = &hspi2;
      break;
    case 4:
      MX_SPI4_Init();
      master->mode = hspi4.Init.Mode;
      dev->private_data = &hspi4;
      break;
  }

  if (master->mode == SPI_MODE_MASTER)
    spi_master_register(master);
}
static struct spi_master stm32h7_spi1 = {
  .dev = {
    .init_name = "stm32-spi-controller",
    .init = stm32_spi_init,
  },
  .bus_num = 1,
};

register_device(gpioa, stm32_gpioa_device.dev);
register_device(gpiob, stm32_gpiob_device.dev);
register_device(gpioc, stm32_gpioc_device.dev);
register_device(gpiod, stm32_gpiod_device.dev);
register_device(gpioe, stm32_gpioe_device.dev);
register_device(gpiof, stm32_gpiof_device.dev);
register_device(gpioh, stm32_gpioh_device.dev);
register_device(gpioi, stm32_gpioi_device.dev);
register_device(gpioj, stm32_gpioj_device.dev);
register_device(stm32h7_uart3, stm32h7_uart3.tty.dev);
register_device(stm32h7_uart4, stm32h7_uart4.tty.dev);
register_device(stm32_gpio_i2c, stm32_gpio_i2c.dev);
register_device(mpu6050, mpu6050.dev.dev);
register_device(stm32h7_spi1, stm32h7_spi1.dev);