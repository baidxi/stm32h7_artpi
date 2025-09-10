#pragma once

#include <stm32h7xx.h>
#include <stm32h7xx_hal.h>
#include <stm32h7xx_hal_uart.h>

struct tty_device;

int stm32h7_uart_device_register(struct tty_device *tty);
