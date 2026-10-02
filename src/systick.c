/**
 * ==============================================================================
 * Project: ZeroHAL
 * File: systick.c
 * Description: Bare-Metal SysTick Hardware Timer Driver (1ms precision heartbeat)
 * ==============================================================================
 */

#include "systick.h"
#include "stm32f4xx_regs.h"

static volatile uint32_t s_system_millis = 0;

void systick_init(void) {
    /* STM32F4 default system clock is 16 MHz HSI (High-Speed Internal).
     * To generate an interrupt every 1 ms (1000 Hz):
     * Reload = (16,000,000 Hz / 1000 Hz) - 1 = 15,999 (0x3E7F) */
    STK_LOAD = 16000UL - 1UL;

    /* Clear current value counter */
    STK_VAL = 0;

    /* Arm SysTick: Processor core clock, exception request enabled, counter started */
    STK_CTRL = STK_CTRL_ENABLE | STK_CTRL_TICKINT | STK_CTRL_CLKSOURCE;
}

void SysTick_Handler(void) {
    s_system_millis++;
}

uint32_t millis(void) {
    return s_system_millis;
}

void delay_ms(uint32_t ms) {
    uint32_t start = millis();
    while ((millis() - start) < ms) {
        /* Busy wait against hardware millisecond counter */
    }
}
