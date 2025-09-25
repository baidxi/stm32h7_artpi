#include <device/tty/tty.h>
#include <device/tty/stm32_uart.h>

#include <init.h>
#include <bus.h>
#include <irq.h>
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

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    struct stm32_uart *uart = container_of(huart, struct stm32_uart, handle);
    struct ring *r = &uart->ringbuf;
    uint16_t rx_cnt = Size - uart->dma.last_counter;

    if (uart->dma.last_counter != Size)
    {
        ring_enqueue(r, 1);
        uart->dma.last_counter = Size;
    };
}

static int stm32_uart_irq_handler(unsigned int irq, void *dev)
{
    extern void HAL_UART_IRQHandler(UART_HandleTypeDef *huart);

    struct stm32_uart *uart = dev;

    HAL_UART_IRQHandler(&uart->handle);

    return 0;
}

static int stm32_uart_open(struct device *dev)
{
    struct stm32_uart *uart = to_stm32_uart(dev);
    struct ring *r = &uart->ringbuf;
    int ret = 0;

    if (uart->is_open) {
        return -EBUSY;
    }

    uart->is_open = true;

    uart->buf = pvPortMalloc(uart->buf_len);
    if (!uart->buf)
        return -ENOMEM;
    
    memset(uart->buf, 0, uart->buf_len);

    r->head = r->tail = 0;
    r->mask = uart->buf_len - 1;
    uart->dma.next_channel = 0;

    ret = request_irq(dev->irq, stm32_uart_irq_handler, 0, dev->name, uart);

    if (ret) {
        uart->is_open = false;
        return ret;
    }

    ret = HAL_UARTEx_ReceiveToIdle_DMA(&uart->handle, uart->buf, uart->buf_len);

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

    if (handle->hdmarx)
    {
        HAL_DMA_Abort(handle->hdmarx);
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
    uint8_t *out = buf;
    struct ring *r = &uart->ringbuf;
    int i = 0;

    if (ring_is_empty(r))
        return 0;

    do {
        out[i++] = uart->buf[ring_dequeue(r, 1) & r->mask];
    }while(!ring_is_empty(r) && i < count);

    return i;
}

static void stm32_uart_tx_complete(void *param)
{
    struct stm32_uart *uart = param;
    uart->tty.complete->complete = true;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    struct stm32_uart *uart = container_of(huart, struct stm32_uart, handle);
    if (uart->tty.complete)
    {
        uart->tty.complete->callback(uart);
    }
}

static size_t stm32_uart_write(struct device *dev, const void *buf, size_t len)
{
    struct tty_device *tty = to_tty_device(dev);
    struct stm32_uart *uart = (struct stm32_uart *)tty;
    UART_HandleTypeDef *handle = &uart->handle;
    struct complete complete = {0};

    int ret;

    if (handle->hdmatx) {
        complete.callback = stm32_uart_tx_complete;
        complete.complete = false;
        complete.param = uart;
        tty->complete = &complete;

        ret = HAL_UART_Transmit_DMA(handle, buf, len);
        if (ret != HAL_OK) {
            return ret;
        }
        while(!complete.complete)
        {
            osDelay(1);
        }
        tty->complete = NULL;
        return len;
    } else {
        ret = HAL_UART_Transmit(handle, buf, len, 10);
    }


    if (ret != HAL_OK)
        return ret;

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

    uart->buf_len = STM32_UART_BUFSZ;

    return 0;
}

static int stm32_uart_remove(struct tty_device *tty)
{
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
