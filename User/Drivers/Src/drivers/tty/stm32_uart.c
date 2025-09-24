#include <device/tty/tty.h>
#include <device/tty/stm32_uart.h>

#include <init.h>
#include <bus.h>
#include <ring.h>

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include <cmsis_os2.h>

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#define to_stm32_uart(d)  container_of(d, struct stm32_uart, tty)

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    struct stm32_uart *tty = (struct stm32_uart *)tty_device_lookup_by_handle(huart);
    tty->tx_cplt = true;
}

void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
    struct stm32_uart *uart = tty_device_lookup_by_handle(huart);
    struct ring *r = &uart->ringbuf;

    // if (uart->tty.mode == TTY_MODE_CONSOLE) {
    //     uart->buf[r->head & r->mask] = uart->chart;
    //     ring_enqueue(r, 1);
    //     if (!ring_is_full(r))
    //         HAL_UART_Receive_DMA(huart, &uart->chart, 1);
    // } else {
    //     ring_enqueue(r, uart->buf_len -1);
    // }
}

const osThreadAttr_t uartTask_attrbutes = {
  .name = "uartTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t)osPriorityNormal1,
};

static void uart_task(void *args)
{
    struct stm32_uart *uart = args;
    UART_HandleTypeDef *handle = uart->tty.dev.private_data;
    struct ring *r = &uart->ringbuf;
    int ret;

    while(uart->is_open) {
        if (ring_size(r)) {
            ret = HAL_UART_Receive(handle, &uart->buf[(r->head & r->mask)], 1 , 10);
            if (ret == HAL_OK) {
                    ring_enqueue(r, 1);
            }
        }
    }
}

static int stm32_uart_open(struct device *dev)
{
    struct stm32_uart *uart = to_stm32_uart(dev);
    UART_HandleTypeDef *handle = uart->tty.dev.private_data;
    struct ring *r = &uart->ringbuf;
    int ret = 0;

    if (uart->is_open) {
        return -EBUSY;
    }

    // uart->is_open = true;
    // r->head = r->tail = 0;
    // r->mask = uart->buf_len -1;

    // memset(uart->buf, 0, uart->buf_len);

    // if (uart->tty.use_dma) {
    //     if (uart->tty.mode == TTY_MODE_CONSOLE)
    //         ret = HAL_UART_Receive_DMA(handle, &uart->chart, 1);
    //     else
    //         ret = HAL_UART_Receive_DMA(handle, uart->buf, uart->buf_len - 1);
    // } else {
    //     uart->tid = osThreadNew(uart_task, uart, &uartTask_attrbutes);
    // }

    return ret;
}

static int stm32_uart_close(struct device *dev)
{
    struct stm32_uart *uart =(struct stm32_uart *) to_tty_device(dev);
    UART_HandleTypeDef *handle = uart->tty.dev.private_data;

    if (!uart->is_open) {
        return 0;
    }

    uart->is_open = false;

    if (uart->tty.use_dma) {
        HAL_UART_Abort(handle);
        HAL_UART_AbortReceive(handle);
    }

    return 0;
}

static int stm32_uart_ioctl(struct device *dev, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static size_t stm32_uart_read(struct device *dev, void *buf, size_t count)
{
    struct stm32_uart *uart =(struct stm32_uart *) to_tty_device(dev);
    UART_HandleTypeDef *handle = uart->tty.dev.private_data;
    uint8_t *out = buf;
    struct ring *r = &uart->ringbuf;
    int i = 0;
    bool dma_start = false;

    if (ring_is_empty(r))
        return 0;

    // if (uart->tty.mode == TTY_MODE_CONSOLE)
    //     dma_start = ring_is_full(r);

    // for (i = 0; i < count; i++) {
    //     out[i] = uart->buf[(r->tail & r->mask)];
    //     ring_dequeue(r, 1);
    //     if (ring_is_empty(r))
    //         break;
    // }

    // if (uart->tty.use_dma) {
    //     if (uart->tty.mode == TTY_MODE_CONSOLE && dma_start) {
    //         HAL_UART_Receive_DMA(handle, &uart->chart, 1);
    //     }
    // }

    return i;
}

static size_t stm32_uart_write(struct device *dev, const void *buf, size_t len)
{
    struct tty_device *tty = to_tty_device(dev);
    struct stm32_uart *uart = (struct stm32_uart *)tty;
    UART_HandleTypeDef *handle = tty->dev.private_data;
    int ret;

    if (uart->tty.use_dma) {
        uart->tx_cplt = false;

        ret = HAL_UART_Transmit_DMA(handle, buf, len);

        if (ret == HAL_OK) {
            while(!uart->tx_cplt) {
                taskYIELD();
                ;
            }
        }
    } else {
        ret = HAL_UART_Transmit(handle, buf, len, 10);
        if (ret == HAL_OK) {
            ret = len;
        }
    }

    return len;
}

const struct tty_operations stm32_uart_ops = {
    .open = stm32_uart_open,
    .close = stm32_uart_close,
    .ioctl = stm32_uart_ioctl,
    .read = stm32_uart_read,
    .write = stm32_uart_write,
};

static int stm32_uart_probe(struct tty_device *tty)
{
    struct stm32_uart *uart = (struct stm32_uart *)tty;

    tty->ops = &stm32_uart_ops;

    uart->is_open = false;

    uart->buf_len = sizeof(uart->buf);

    return 0;
}

static int stm32_uart_remove(struct tty_device *tty)
{
    // struct stm32_uart *uart = (struct stm32_uart *)tty;
    tty->ops = NULL;
    return 0;
}

static const struct device_match_table stm32_uart_ids[] = {
    {
        .compatible = "stm32-uart"
    },
    {

    }
};

static void tty_driver_init(struct device_driver *drv)
{
    tty_driver_register(to_tty_driver(drv));
}

static struct tty_driver stm32_uart_drv = {
    .drv = {
        .match_ptr = stm32_uart_ids,
        .name = "stm32-uart-drv",
        .init = tty_driver_init,
    },
    .probe = stm32_uart_probe,
    .remove = stm32_uart_remove,
};

register_driver(stm32_uart, stm32_uart_drv.drv);
