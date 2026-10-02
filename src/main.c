/**
 * ==============================================================================
 * Project: ZeroHAL
 * File: main.c
 * Description: Bare-Metal Application Super-Loop & Diagnostic Runtime
 * Target: STM32F4 Series (ARM Cortex-M4F)
 * Standard: ISO 26262 Deterministic Architecture (Zero Vendor HAL)
 * ==============================================================================
 */

#include "zero_hal.h"

int main(void) {
    /* 1. Initialize ARM SysTick 1ms core timer */
    systick_init();

    /* 2. Initialize bare-metal UART2, PA5 LED, and NVIC IRQ 38 */
    uart2_init();

    /* 3. Initialize Diagnostic CLI engine */
    cli_init();

    /* 4. Boot Banner */
    uart2_write_string("\r\n================================================\r\n");
    uart2_write_string(" [ZeroHAL] Bare-Metal Embedded OS v2.0 Online!  \r\n");
    uart2_write_string("================================================\r\n");

    /* 5. Safety Audit: Inspect silicon reset flags in RCC_CSR */
    if (iwdg_caused_reset()) {
        uart2_write_string("[SAFETY ALERT] System recovered from IWDG WATCHDOG TIMEOUT!\r\n");
        uart2_write_string("[SAFETY ALERT] Hardware reset asserted safely. Restoring operations.\r\n\r\n");
        iwdg_clear_reset_flags();
    } else {
        uart2_write_string("[BOOT] Cold start / Software reset detected.\r\n\r\n");
    }

    /* 6. Arm Independent Hardware Watchdog (2000ms safety window) */
    iwdg_init(2000);
    uart2_write_string("[IWDG] Hardware Watchdog Armed: 2000ms timeout\r\n\r\n");

    /* Render initial prompt */
    cli_prompt();

    uint32_t last_heartbeat = millis();

    while (1) {
        /* Refresh hardware watchdog counter during normal operational flow */
        iwdg_feed();

        /* Drain asynchronous characters received by USART2 ISR into CLI parser */
        char incoming;
        RingBuffer *rx_ring = uart2_get_rx_buffer();
        while (ring_buffer_pop(rx_ring, &incoming)) {
            cli_handle_char(incoming);
        }

        /* Periodic 1000ms background heartbeat on PA5 User LED */
        if ((millis() - last_heartbeat) >= 1000U) {
            last_heartbeat = millis();
            GPIOA_ODR ^= GPIO_PIN_5;
        }
    }

    return 0;
}
