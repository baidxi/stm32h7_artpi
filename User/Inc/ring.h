#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>

struct ring {
    uint32_t head;
    uint32_t tail;
    uint32_t mask;
};

static inline uint32_t ring_count(struct ring *r)
{
    return (r->head - r->tail) & r->mask;
}

static inline bool ring_is_empty(struct ring *r)
{
    return ring_count(r) == 0;
}

static inline bool ring_is_full(struct ring *r)
{
    return ring_count(r) == r->mask;
}

static inline uint32_t ring_size(struct ring *r)
{
    return r->mask - ring_count(r);
}

static inline uint32_t ring_enqueue(struct ring *r, uint32_t count)
{
    return __atomic_add_fetch(&r->head, count, memory_order_relaxed);
}

static inline uint32_t ring_dequeue(struct ring *r, uint32_t count)
{
    return __atomic_fetch_add(&r->tail, count, memory_order_relaxed);
}