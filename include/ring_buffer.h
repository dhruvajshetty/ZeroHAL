/**
 * ==============================================================================
 * Project: ZeroHAL
 * File: ring_buffer.h
 * Description: Lock-Free Single-Producer Single-Consumer (SPSC) Circular FIFO
 * Target: Embedded Systems / ISR Decoupling
 * ==============================================================================
 * 
 * DESIGN PRINCIPLES:
 * 1. Power-of-2 sizing (64 bytes) enables fast bitwise masking: (x & (SIZE - 1)).
 * 2. Lock-free SPSC: Head is exclusively modified by ISR (Producer);
 *    Tail is exclusively modified by the Main Application Super-Loop (Consumer).
 * 3. Never blocks interrupts; discards incoming byte safely on buffer saturation.
 */

#ifndef ZERO_HAL_RING_BUFFER_H
#define ZERO_HAL_RING_BUFFER_H

#include <stdint.h>

#define RING_BUFFER_SIZE        64U  /* Must remain a power of 2 */
#define RING_BUFFER_MASK        (RING_BUFFER_SIZE - 1U)

typedef struct {
    volatile char buffer[RING_BUFFER_SIZE];
    volatile uint32_t head; /* Producer index (ISR write) */
    volatile uint32_t tail; /* Consumer index (Application read) */
} RingBuffer;

/**
 * @brief Initialize ring buffer indices to 0.
 */
static inline void ring_buffer_init(RingBuffer *rb) {
    if (!rb) return;
    rb->head = 0U;
    rb->tail = 0U;
}

/**
 * @brief Returns non-zero if ring buffer contains no data.
 */
static inline uint8_t ring_buffer_is_empty(const RingBuffer *rb) {
    return (rb->head == rb->tail) ? 1U : 0U;
}

/**
 * @brief Returns non-zero if ring buffer is at maximum capacity (63 bytes).
 */
static inline uint8_t ring_buffer_is_full(const RingBuffer *rb) {
    return (((rb->head + 1U) & RING_BUFFER_MASK) == rb->tail) ? 1U : 0U;
}

/**
 * @brief Returns the count of unread bytes in the buffer.
 */
static inline uint32_t ring_buffer_available(const RingBuffer *rb) {
    return ((rb->head - rb->tail) & RING_BUFFER_MASK);
}

/**
 * @brief Push a character into the ring buffer (Producer / ISR).
 * @return 1 on success, 0 if buffer full (character dropped).
 */
static inline uint8_t ring_buffer_push(RingBuffer *rb, char c) {
    uint32_t next_head = (rb->head + 1U) & RING_BUFFER_MASK;
    if (next_head != rb->tail) {
        rb->buffer[rb->head] = c;
        rb->head = next_head;
        return 1U;
    }
    return 0U; /* Overflow drop to preserve oldest unhandled data */
}

/**
 * @brief Pop a character from the ring buffer (Consumer / Super-Loop).
 * @return 1 on success with character written to *out_char, 0 if empty.
 */
static inline uint8_t ring_buffer_pop(RingBuffer *rb, char *out_char) {
    if (rb->head == rb->tail) {
        return 0U;
    }
    *out_char = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1U) & RING_BUFFER_MASK;
    return 1U;
}

#endif /* ZERO_HAL_RING_BUFFER_H */
