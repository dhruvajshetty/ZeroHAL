/**
 * ==============================================================================
 * Project: ZeroHAL
 * File: cli.c
 * Description: Zero-Libc Interactive Diagnostic Serial Console
 * ==============================================================================
 */

#include "cli.h"
#include "uart.h"
#include "systick.h"
#include "iwdg.h"
#include "stm32f4xx_regs.h"

static char    s_cli_buf[CLI_BUFFER_SIZE];
static uint8_t s_cli_idx = 0;

/**
 * @brief Zero-libc string equality comparison.
 */
static int cli_strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return (*(const unsigned char *)s1 - *(const unsigned char *)s2) == 0;
}

void cli_init(void) {
    s_cli_idx = 0;
    s_cli_buf[0] = '\0';
}

void cli_prompt(void) {
    uart2_write_string("ZeroHAL> ");
}

void cli_execute(const char *cmd) {
    if (!cmd) return;

    if (cli_strcmp(cmd, "help")) {
        uart2_write_string("\r\n========================================\r\n");
        uart2_write_string("  ZeroHAL Bare-Metal Diagnostic Console  \r\n");
        uart2_write_string("========================================\r\n");
        uart2_write_string("  help       - Show available ECU commands\r\n");
        uart2_write_string("  status     - Show uptime, LED, and IWDG status\r\n");
        uart2_write_string("  led on     - Drive PA5 output HIGH\r\n");
        uart2_write_string("  led off    - Drive PA5 output LOW\r\n");
        uart2_write_string("  led toggle - Invert PA5 output state\r\n");
        uart2_write_string("  feed       - Manually kick the IWDG watchdog\r\n");
        uart2_write_string("  hang       - Freeze CPU to trigger Watchdog Reset\r\n");
        uart2_write_string("  reset      - Trigger SCB_AIRCR system reset\r\n\r\n");
    } else if (cli_strcmp(cmd, "status")) {
        uart2_write_string("\r\n[ECU TELEMETRY]\r\n");
        uart2_write_string("  Uptime        : ");
        uart2_write_dec(millis() / 1000U);
        uart2_write_string("s (");
        uart2_write_dec(millis());
        uart2_write_string(" ms)\r\n");
        uart2_write_string("  PA5 LED State : ");
        uart2_write_string((GPIOA_ODR & GPIO_PIN_5) ? "ON (3.3V)\r\n" : "OFF (0.0V)\r\n");
        uart2_write_string("  IWDG Hardware : ACTIVE (2000ms window, 32kHz LSI)\r\n");
        uart2_write_string("  Ring Buffer   : Lock-free SPSC FIFO decoupled\r\n\r\n");
    } else if (cli_strcmp(cmd, "led on")) {
        GPIOA_ODR |= GPIO_PIN_5;
        uart2_write_string("[OK] PA5 LED is ON\r\n");
    } else if (cli_strcmp(cmd, "led off")) {
        GPIOA_ODR &= ~GPIO_PIN_5;
        uart2_write_string("[OK] PA5 LED is OFF\r\n");
    } else if (cli_strcmp(cmd, "led toggle")) {
        GPIOA_ODR ^= GPIO_PIN_5;
        uart2_write_string("[OK] PA5 LED toggled\r\n");
    } else if (cli_strcmp(cmd, "feed")) {
        iwdg_feed();
        uart2_write_string("[OK] IWDG watchdog timer refreshed.\r\n");
    } else if (cli_strcmp(cmd, "hang")) {
        uart2_write_string("\r\n[SIMULATION] Entering infinite loop without feeding watchdog...\r\n");
        uart2_write_string("[SIMULATION] IWDG hardware will force-reset MCU in 2 seconds!\r\n");
        while (1) {
            /* CPU intentionally deadlocked: no iwdg_feed() called! */
        }
    } else if (cli_strcmp(cmd, "reset")) {
        uart2_write_string("\r\n[SYSTEM] Triggering immediate Cortex-M Software Reset...\r\n");
        mcu_system_reset();
    } else if (cmd[0] != '\0') {
        uart2_write_string("Unknown command: '");
        uart2_write_string(cmd);
        uart2_write_string("'. Type 'help' for command list.\r\n");
    }
}

void cli_handle_char(char c) {
    if (c == '\r' || c == '\n') {
        uart2_write_string("\r\n");
        s_cli_buf[s_cli_idx] = '\0';
        cli_execute(s_cli_buf);
        s_cli_idx = 0;
        cli_prompt();
    } else if (c == '\b' || c == 127) { /* Backspace or ASCII DEL */
        if (s_cli_idx > 0) {
            s_cli_idx--;
            uart2_write_string("\b \b");
        }
    } else if (s_cli_idx < (CLI_BUFFER_SIZE - 1U)) {
        s_cli_buf[s_cli_idx++] = c;
        uart2_write_char(c); /* Terminal echo */
    }
}
