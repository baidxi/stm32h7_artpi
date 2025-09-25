#pragma once

#include <ring.h>
#include <device/tty/tty.h>

#include <usart.h>

#define STM32_UART_BUFSZ    16
struct stm32_uart {
    struct tty_device tty;
    UART_HandleTypeDef handle;
    uint8_t *buf;
    size_t buf_len;
    bool is_open;
    struct ring ringbuf;
    void *tid;
    bool tx_cplt;
    struct {
        uint16_t last_counter;
        uint32_t next_channel;
    }dma;
};