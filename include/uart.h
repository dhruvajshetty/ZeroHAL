/**
 * ==============================================================================
 * Project: ZeroHAL
 * File: uart.h
 * Description: Bare-Metal USART2 Driver for Serial Telemetry & Asynchronous RX
 * ==============================================================================
 */

#ifndef ZERO_HAL_UART_H
#define ZERO_HAL_UART_H

#include <stdint.h>
#include "ring_buffer.h"

/**
 * @brief Initialize USART2 peripheral, GPIO pin routing (PA2=TX, PA3=RX, PA5=LED),
 *        115200 baud rate, and Cortex-M NVIC interrupt unmasking.
 */
void uart2_init(void);

/**
 * @brief Transmit a single character over USART2 (polls TXE).
 * @param ch Character byte to transmit.
 */
void uart2_write_char(char ch);

/**
 * @brief Transmit a null-terminated string over USART2.
 * @param str Null-terminated ASCII string.
 */
void uart2_write_string(const char *str);

/**
 * @brief Transmit an unsigned 32-bit integer as an ASCII decimal string.
 * @param val Number to format and print.
 */
void uart2_write_dec(uint32_t val);

/**
 * @brief Retrieve pointer to the RX ring buffer filled by the USART2 ISR.
 * @return Pointer to active RingBuffer instance.
 */
RingBuffer* uart2_get_rx_buffer(void);

/**
 * @brief USART2 Global Interrupt Service Routine (IRQ 38 / Vector 54).
 */
void USART2_IRQHandler(void);

#endif /* ZERO_HAL_UART_H */
