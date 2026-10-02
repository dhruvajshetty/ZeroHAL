/**
 * ==============================================================================
 * Project: ZeroHAL
 * File: systick.h
 * Description: ARM Cortex-M 24-bit SysTick Hardware Timer Driver
 * ==============================================================================
 */

#ifndef ZERO_HAL_SYSTICK_H
#define ZERO_HAL_SYSTICK_H

#include <stdint.h>

/**
 * @brief Configure SysTick for 1ms periodic tick interrupts at 16 MHz HSI.
 */
void systick_init(void);

/**
 * @brief SysTick Exception Handler (Vector 15).
 */
void SysTick_Handler(void);

/**
 * @brief Returns system uptime in milliseconds since boot.
 */
uint32_t millis(void);

/**
 * @brief Blocking millisecond delay using the hardware SysTick counter.
 * @param ms Delay duration in milliseconds.
 */
void delay_ms(uint32_t ms);

#endif /* ZERO_HAL_SYSTICK_H */
