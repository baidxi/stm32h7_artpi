#include <device/tty/tty.h>
#include <device/tty/stm32h7_uart.h>

#include <init.h>
#include <bus.h>
#include <ring.h>

#include <FreeRTOS.h>
#include <semphr.h>

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

struct stm32h7_uart {
    struct tty_device device;
    uint8_t port_num;
    uint8_t buf[1024];
    size_t buf_len;
    bool is_open;
    struct ring ringbuf;
    xSemaphoreHandle lock;
};

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    struct stm32h7_uart *uart = (struct stm32h7_uart *)tty_device_lookup_by_handle(huart);
    uint32_t head;
    uint32_t remain;
    uint32_t req_size = Size;
    uint32_t offset;
    
    remain = ring_size(&uart->ringbuf);

    if (req_size > remain) {
        req_size = remain;
    }

    head = ring_enqueue(&uart->ringbuf, req_size);

    offset = head & uart->ringbuf.mask;

    if (!ring_is_full(&uart->ringbuf))
        HAL_UARTEx_ReceiveToIdle_DMA(huart, &uart->buf[offset], uart->buf_len - offset);
}

static int stm32h7_uart_open(struct device *dev)
{
    struct stm32h7_uart *uart = (struct stm32h7_uart *)to_tty_device(dev);
    struct ring *ring = &uart->ringbuf;

    xSemaphoreTake(uart->lock, portMAX_DELAY);

    if (uart->is_open) {
        xSemaphoreGive(uart->lock);
        return -EBUSY;
    }


    uart->buf_len = sizeof(uart->buf);

    ring->head = 0;
    ring->tail = 0;
    ring->mask = uart->buf_len - 1;

    uart->is_open = true;

    xSemaphoreGive(uart->lock);

    __HAL_UART_ENABLE_IT((UART_HandleTypeDef *)uart->device.dev.private_data, UART_IT_IDLE);

    return HAL_UARTEx_ReceiveToIdle_DMA(uart->device.dev.private_data, uart->buf, ring->mask);
}

static int stm32h7_uart_close(struct device *dev)
{
    struct stm32h7_uart *uart = (struct stm32h7_uart *)to_tty_device(dev);

    xSemaphoreTake(uart->lock, portMAX_DELAY);

    if (!uart->is_open) {
        xSemaphoreGive(uart->lock);
        return 0;
    }

    uart->is_open = false;

    xSemaphoreGive(uart->lock);

    __HAL_UART_DISABLE_IT((UART_HandleTypeDef *)uart->device.dev.private_data, UART_IT_IDLE);

    HAL_UART_AbortReceive((UART_HandleTypeDef *)uart->device.dev.private_data);
    HAL_UART_AbortTransmit((UART_HandleTypeDef *)uart->device.dev.private_data);

    return 0;
}

static int stm32h7_uart_ioctl(struct device *dev, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static size_t stm32h7_uart_read(struct device *dev, void *buf, size_t count)
{
    struct stm32h7_uart *uart = (struct stm32h7_uart *)to_tty_device(dev);
    bool start_dma = false;
    int acquire = count;
    int offset;

    if (!uart->is_open)
        return -ENXIO;

    xSemaphoreTake(uart->lock, portMAX_DELAY);

    start_dma = ring_is_full(&uart->ringbuf);

    if (ring_count(&uart->ringbuf) < count) 
        acquire = ring_count(&uart->ringbuf);

    if (acquire) {
        offset = ring_dequeue(&uart->ringbuf, acquire) & uart->ringbuf.mask;

        memcpy(buf, &uart->buf[offset], acquire);
    }

    if (start_dma) {
        offset = uart->ringbuf.head & uart->ringbuf.mask;
        HAL_UARTEx_ReceiveToIdle_DMA(uart->device.dev.private_data, &uart->buf[offset], count);
    }

    xSemaphoreGive(uart->lock);

    return acquire;
}

static size_t stm32h7_uart_write(struct device *dev, const void *buf, size_t size)
{
    struct stm32h7_uart *uart = (struct stm32h7_uart *)to_tty_device(dev);

    if (!uart->is_open)
        return -ENXIO;

    xSemaphoreTake(uart->lock, portMAX_DELAY);

    HAL_UART_Transmit(uart->device.dev.private_data, buf, size, 10);

    xSemaphoreGive(uart->lock);

    return size;
}

const struct tty_operations stm32h7_uart_ops = {
    .open = stm32h7_uart_open,
    .close =stm32h7_uart_close,
    .ioctl = stm32h7_uart_ioctl,
    .read = stm32h7_uart_read,
    .write = stm32h7_uart_write,
};

static int stm32h7_uart_probe(struct tty_device *tty)
{
    struct stm32h7_uart *uart = (struct stm32h7_uart *)tty;

    tty->ops = &stm32h7_uart_ops;

    uart->lock = xSemaphoreCreateMutex();

    return 0;
}

static void stm32h7_uart_remove(struct tty_device *tty)
{
    struct stm32h7_uart *uart = (struct stm32h7_uart *)tty;

    vSemaphoreDelete(uart->lock);

    tty->ops = NULL;
}

static const struct driver_match_table stm32h7_uart_ids[] = {
    {
        .compatible = "stm32h7-uart"
    },
    {

    }
};

#define to_stm32h7_uart(d)  container_of(d, struct stm32h7_uart, device)

int stm32h7_uart_device_register(struct tty_device *tty, const char *name, uint8_t port_num)
{

    struct stm32h7_uart *uart = to_stm32h7_uart(tty);

    if (!uart)
        return -ENOMEM;

    sprintf(uart->device.dev.name, name);
    uart->port_num = port_num;

    return tty_device_register(&uart->device);
}

static void tty_driver_init(struct driver *drv)
{
    tty_driver_register(to_tty_driver(drv));
}

static struct tty_driver stm32h7_uart_drv = {
    .drv = {
        .match_ptr = stm32h7_uart_ids,
        .name = "stm32h7-uart-drv",
        .init = tty_driver_init,
    },
    .probe = stm32h7_uart_probe,
    .remove = stm32h7_uart_remove,
};

extern void stm32h7_usart3_init(struct device *dev);
extern void stm32h7_uart4_init(struct device *dev);

static struct stm32h7_uart stm32h7_usart3 = {
    .device = {
        .dev ={
            .init_name = "stm32h7-uart",
            .init = stm32h7_usart3_init,
        }
    }
};

static struct stm32h7_uart stm32h7_uart4 = {
    .device = {
        .dev = {
            .init_name = "stm32h7-uart",
            .init = stm32h7_uart4_init,
        }
    }
};

register_device(stm32h7_uart3, stm32h7_usart3.device.dev);
register_device(stm32h7_uart4, stm32h7_uart4.device.dev);

register_driver(stm32h7_uart, stm32h7_uart_drv.drv);

