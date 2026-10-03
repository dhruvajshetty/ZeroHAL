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

/* --- System Tasks --- */

static void task_pwm_dimmer(void) {
    static int8_t duty = 0;
    static int8_t direction = 1;

    duty += (direction * 2);
    if (duty >= 100) {
        duty = 100;
        direction = -1;
    } else if (duty <= 0) {
        duty = 0;
        direction = 1;
    }

    pwm_set_duty((uint8_t)duty);
}

static void task_cli_process(void) {
    char incoming;
    RingBuffer *rx_ring = uart2_get_rx_buffer();
    int limit = 16; // Process up to 16 chars per tick to prevent starvation
    while (limit-- && ring_buffer_pop(rx_ring, &incoming)) {
        cli_handle_char(incoming);
    }
}

static void task_watchdog_feed(void) {
    iwdg_feed();
}

int main(void) {
    /* 1. Initialize ARM SysTick 1ms core timer */
    systick_init();

    /* 2. Initialize bare-metal UART2, PA5 LED, and NVIC IRQ 38 */
    uart2_init();

    /* 3. Initialize Diagnostic CLI engine */
    cli_init();

    /* 4. Boot Banner */
    uart2_write_string("\r\n================================================\r\n");
    uart2_write_string(" [ZeroHAL] Bare-Metal Embedded OS v2.1 Online!  \r\n");
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
    uart2_write_string("[IWDG] Hardware Watchdog Armed: 2000ms timeout\r\n");

    /* 6b. Initialize PWM for the LED */
    pwm_init();
    uart2_write_string("[PWM]  TIM2_CH1 initialized on PA5 for LED Dimming\r\n");

    /* 7. Setup Cooperative Task Scheduler */
    os_scheduler_init();
    os_add_task(task_watchdog_feed, 500); // Feed watchdog every 500ms safely
    os_add_task(task_cli_process,    10); // Poll UART queue every 10ms
    os_add_task(task_pwm_dimmer,     20); // Update PWM duty cycle every 20ms

    uart2_write_string("[OS] Cooperative Scheduler running with 3 tasks.\r\n\r\n");

    /* Render initial prompt */
    cli_prompt();

    /* 8. Main OS Loop */
    while (1) {
        os_run_scheduler();
    }

    return 0;
}
