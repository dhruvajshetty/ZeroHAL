/**
 * ==============================================================================
 * Project: ZeroHAL
 * File: uart.c
 * Description: Bare-Metal USART2 Driver (115200 8N1 with Asynchronous RX FIFO)
 * ==============================================================================
 */

#include "uart.h"
#include "stm32f4xx_regs.h"

static RingBuffer s_rx_ring;

RingBuffer* uart2_get_rx_buffer(void) {
    return &s_rx_ring;
}

void uart2_init(void) {
    /* Initialize asynchronous lock-free ring buffer */
    ring_buffer_init(&s_rx_ring);

    /* --------------------------------------------------------------------------
     * 1. CLOCK GATING: Enable AHB1 (GPIOA) and APB1 (USART2) bus clocks
     * -------------------------------------------------------------------------- */
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC_APB1ENR |= RCC_APB1ENR_USART2EN;

    /* --------------------------------------------------------------------------
     * 2. GPIO PIN CONFIGURATION: PA2 (TX), PA3 (RX), PA5 (Status LED)
     * -------------------------------------------------------------------------- */
    /* MODER: Alternate Function (10b) for PA2 and PA3 */
    GPIOA_MODER &= ~((3UL << (2 * 2)) | (3UL << (3 * 2)));
    GPIOA_MODER |=  ((2UL << (2 * 2)) | (2UL << (3 * 2)));

    /* PUPDR: Internal pull-up (01b) on PA3 (RX pin idle high) */
    GPIOA_PUPDR &= ~(3UL << (3 * 2));
    GPIOA_PUPDR |=  (1UL << (3 * 2));

    /* AFRL: Route Pin 2 and Pin 3 to AF7 (USART2) */
    GPIOA_AFRL &= ~((0xFUL << (2 * 4)) | (0xFUL << (3 * 4)));
    GPIOA_AFRL |=  ((0x7UL << (2 * 4)) | (0x7UL << (3 * 4)));

    /* --------------------------------------------------------------------------
     * 3. BAUD RATE GENERATOR: 115200 bps @ 16 MHz HSI Clock
     * --------------------------------------------------------------------------
     * USARTDIV = 16,000,000 / (16 * 115200) = 8.6805
     * Mantissa = 8 (0x08), Fraction = 0.6805 * 16 = 10.88 -> 11 (0x0B)
     * USART_BRR = 0x008B
     */
    USART2_BRR = 0x008BUL;

    /* --------------------------------------------------------------------------
     * 4. CONTROL REGISTER 1: Enable TX, RX, RXNE Interrupt, and USART Engine
     * -------------------------------------------------------------------------- */
    USART2_CR1 |= (USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE | USART_CR1_UE);

    /* --------------------------------------------------------------------------
     * 5. ARM CORTEX-M NVIC: Unmask USART2 Global Interrupt (IRQ 38)
     * --------------------------------------------------------------------------
     * IRQ 38 belongs to NVIC_ISER1 (handles IRQs 32..63). Target bit = 38 - 32 = 6.
     */
    NVIC_ISER1 |= (1UL << (USART2_IRQN - 32));
}

void uart2_write_char(char ch) {
    /* Poll until Transmit Data Register Empty (TXE == 1) */
    while (!(USART2_SR & USART_SR_TXE)) {
        /* Hardware serialization in progress */
    }
    USART2_DR = (uint32_t)ch;
}

void uart2_write_string(const char *str) {
    if (!str) return;
    while (*str) {
        uart2_write_char(*str++);
    }
}

void uart2_write_dec(uint32_t val) {
    char buf[12];
    int idx = 11;
    buf[idx] = '\0';

    if (val == 0) {
        uart2_write_char('0');
        return;
    }

    while (val > 0 && idx > 0) {
        buf[--idx] = (char)('0' + (val % 10));
        val /= 10;
    }

    uart2_write_string(&buf[idx]);
}

/**
 * @brief USART2 Global Interrupt Service Routine
 * Executed asynchronously when hardware receives a byte over the PA3 RX pin.
 */
void USART2_IRQHandler(void) {
    if (USART2_SR & USART_SR_RXNE) {
        /* Reading USART2_DR clears the hardware RXNE flag */
        char c = (char)(USART2_DR & 0xFFUL);
        ring_buffer_push(&s_rx_ring, c);
    }
}
