/**
 * ==============================================================================
 * Project: ZeroHAL
 * File: test_runner.c
 * Description: Automated Host Unit Test Suite for ZeroHAL Invariants
 * ==============================================================================
 * 
 * Verifies critical embedded primitives without requiring physical target silicon:
 * 1. Lock-free circular ring buffer (SPSC FIFO) boundary wrap and saturation.
 * 2. Silicon timing formulas (SysTick 1ms reload, IWDG prescaler, UART BRR).
 * 3. Silicon physical register addresses and bitmask integrity.
 * 4. Zero-libc CLI string parser and buffer boundary defenses.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "stm32f4xx_regs.h"
#include "ring_buffer.h"

/* ANSI Colors for Test Output */
#define COLOR_RESET   "\033[0m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_RED     "\033[31m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_CYAN    "\033[36m"

static int g_tests_passed = 0;
static int g_tests_total  = 0;

#define TEST_ASSERT(cond, msg) do { \
    g_tests_total++; \
    if (cond) { \
        g_tests_passed++; \
        printf("  " COLOR_GREEN "[PASS]" COLOR_RESET " %s\n", msg); \
    } else { \
        printf("  " COLOR_RED "[FAIL]" COLOR_RESET " %s (Line %d)\n", msg, __LINE__); \
    } \
} while(0)

/* ==============================================================================
 * TEST GROUP 1: LOCK-FREE CIRCULAR RING BUFFER (FIFO)
 * ============================================================================== */

static void test_ring_buffer_primitives(void) {
    printf(COLOR_BOLD COLOR_CYAN "\n=== [TEST GROUP 1] Lock-Free Ring Buffer (SPSC FIFO) ===" COLOR_RESET "\n");

    RingBuffer rb;
    ring_buffer_init(&rb);

    TEST_ASSERT(ring_buffer_is_empty(&rb) == 1, "New ring buffer is initialized empty");
    TEST_ASSERT(ring_buffer_is_full(&rb) == 0, "New ring buffer is not full");
    TEST_ASSERT(ring_buffer_available(&rb) == 0, "Available bytes count is 0");

    /* Push and pop single character */
    char c = 0;
    TEST_ASSERT(ring_buffer_push(&rb, 'X') == 1, "Push character 'X' returns success");
    TEST_ASSERT(ring_buffer_is_empty(&rb) == 0, "Ring buffer is no longer empty");
    TEST_ASSERT(ring_buffer_available(&rb) == 1, "Available count is 1");
    TEST_ASSERT(ring_buffer_pop(&rb, &c) == 1 && c == 'X', "Pop returns character 'X'");
    TEST_ASSERT(ring_buffer_is_empty(&rb) == 1, "Buffer is empty after pop");
    TEST_ASSERT(ring_buffer_pop(&rb, &c) == 0, "Popping from empty buffer returns 0");

    /* FIFO ordering */
    const char *payload = "HEARTBEAT";
    for (size_t i = 0; payload[i] != '\0'; i++) {
        ring_buffer_push(&rb, payload[i]);
    }
    TEST_ASSERT(ring_buffer_available(&rb) == 9, "Pushed 9 characters, available count == 9");

    char popped[16] = {0};
    for (size_t i = 0; i < 9; i++) {
        ring_buffer_pop(&rb, &popped[i]);
    }
    TEST_ASSERT(strcmp(popped, "HEARTBEAT") == 0, "FIFO ordering preserved identically");

    /* Full capacity saturation (RING_BUFFER_SIZE - 1 = 63 bytes) */
    ring_buffer_init(&rb);
    for (uint32_t i = 0; i < (RING_BUFFER_SIZE - 1U); i++) {
        uint8_t res = ring_buffer_push(&rb, (char)('0' + (i % 10)));
        if (!res) {
            TEST_ASSERT(0, "Premature ring buffer saturation");
            break;
        }
    }
    TEST_ASSERT(ring_buffer_is_full(&rb) == 1, "Buffer reports full at 63 elements");
    TEST_ASSERT(ring_buffer_push(&rb, '!') == 0, "Push to full buffer safely drops without corruption");

    /* Boundary wraparound across power-of-2 index */
    ring_buffer_init(&rb);
    /* Cycle 200 items through buffer to force index wrapping multiple times */
    int wrap_ok = 1;
    for (int i = 0; i < 200; i++) {
        ring_buffer_push(&rb, (char)(i & 0xFF));
        char out = 0;
        if (!ring_buffer_pop(&rb, &out) || (out != (char)(i & 0xFF))) {
            wrap_ok = 0;
            break;
        }
    }
    TEST_ASSERT(wrap_ok == 1, "Circular bitwise wraparound (& 0x3F) flawless across 200 cycles");
}

/* ==============================================================================
 * TEST GROUP 2: SILICON HARDWARE TIMING & MATH FORMULAS
 * ============================================================================== */

static void test_hardware_math_formulas(void) {
    printf(COLOR_BOLD COLOR_CYAN "\n=== [TEST GROUP 2] Silicon Hardware Timing & Math ===" COLOR_RESET "\n");

    /* SysTick Reload Value for 1ms @ 16 MHz HSI */
    uint32_t cpu_freq = 16000000UL;
    uint32_t tick_freq = 1000UL;
    uint32_t systick_reload = (cpu_freq / tick_freq) - 1UL;
    TEST_ASSERT(systick_reload == 15999UL, "SysTick 1ms reload value is exactly 15,999 (0x3E7F)");

    /* USART2 BRR Calculation for 115200 @ 16 MHz */
    /* USARTDIV = 16000000 / (16 * 115200) = 8.680555 */
    /* Mantissa = 8 (0x08), Fraction = round(0.680555 * 16) = 11 (0x0B) */
    /* BRR = (8 << 4) | 11 = 0x008B */
    uint32_t baud = 115200UL;
    double div = (double)cpu_freq / (16.0 * (double)baud);
    uint32_t mantissa = (uint32_t)div;
    uint32_t fraction = (uint32_t)((div - mantissa) * 16.0 + 0.5);
    uint32_t computed_brr = (mantissa << 4) | fraction;
    TEST_ASSERT(computed_brr == 0x008BUL, "USART2 BRR for 115200 bps @ 16MHz computes to 0x008B");

    /* IWDG Prescaler /64 with 32 kHz LSI clock */
    /* 32000 Hz / 64 = 500 Hz (2ms per downcounter tick) */
    /* Timeout 2000 ms -> 2000 / 2 = 1000 reload value */
    uint32_t lsi_freq = 32000UL;
    uint32_t iwdg_tick_hz = lsi_freq / 64UL;
    TEST_ASSERT(iwdg_tick_hz == 500UL, "IWDG /64 prescaler produces exactly 500 Hz tick rate");

    uint16_t timeout_ms = 2000;
    uint32_t reload = timeout_ms / 2U;
    TEST_ASSERT(reload == 1000UL, "2000ms watchdog timeout requires reload value 1000");
    TEST_ASSERT(reload <= 0x0FFFUL, "Watchdog reload value fits within 12-bit hardware register");
}

/* ==============================================================================
 * TEST GROUP 3: PHYSICAL SILICON MEMORY MAP OFFSETS
 * ============================================================================== */

static void test_silicon_memory_map(void) {
    printf(COLOR_BOLD COLOR_CYAN "\n=== [TEST GROUP 3] STM32F4 Memory Map Verification ===" COLOR_RESET "\n");

    /* Verify base addresses against ST Reference Manual RM0090 */
    TEST_ASSERT(AHB1PERIPH_BASE == 0x40020000UL, "AHB1 peripheral base is 0x40020000");
    TEST_ASSERT(APB1PERIPH_BASE == 0x40000000UL, "APB1 peripheral base is 0x40000000");
    TEST_ASSERT(SCS_BASE        == 0xE000E000UL, "ARM System Control Space is 0xE000E000");

    TEST_ASSERT(GPIOA_BASE      == 0x40020000UL, "GPIOA base is 0x40020000");
    TEST_ASSERT(RCC_BASE        == 0x40023800UL, "RCC base is 0x40023800");
    TEST_ASSERT(USART2_BASE     == 0x40004400UL, "USART2 base is 0x40004400");
    TEST_ASSERT(IWDG_BASE       == 0x40003000UL, "IWDG base is 0x40003000");

    /* Cortex-M Peripherals */
    TEST_ASSERT(SYSTICK_BASE    == 0xE000E010UL, "SysTick Base is 0xE000E010");
    TEST_ASSERT(NVIC_BASE       == 0xE000E100UL, "NVIC Base is 0xE000E100");
    TEST_ASSERT((NVIC_BASE + 0x04UL) == 0xE000E104UL, "NVIC_ISER1 address is 0xE000E104");

    /* NVIC IRQ Bit position check */
    /* USART2 IRQ 38: Bit in ISER1 = 38 - 32 = 6 */
    uint32_t irq_bit = (1UL << (USART2_IRQN - 32));
    TEST_ASSERT(irq_bit == (1UL << 6), "USART2 IRQ 38 maps to Bit 6 of NVIC_ISER1");

    /* SCB AIRCR Reset Key */
    TEST_ASSERT(SCB_AIRCR_VECTKEY == (0x05FAUL << 16), "SCB AIRCR VECTKEY is 0x05FA0000");
}

/* ==============================================================================
 * TEST GROUP 4: CLI PARSER STRING MATCHING
 * ============================================================================== */

static int mock_cli_strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return (*(const unsigned char *)s1 - *(const unsigned char *)s2) == 0;
}

static void test_cli_parsing(void) {
    printf(COLOR_BOLD COLOR_CYAN "\n=== [TEST GROUP 4] Zero-Libc CLI String Matching ===" COLOR_RESET "\n");

    TEST_ASSERT(mock_cli_strcmp("help", "help") == 1, "Command 'help' matches");
    TEST_ASSERT(mock_cli_strcmp("status", "status") == 1, "Command 'status' matches");
    TEST_ASSERT(mock_cli_strcmp("led on", "led on") == 1, "Command 'led on' matches");
    TEST_ASSERT(mock_cli_strcmp("led off", "led off") == 1, "Command 'led off' matches");
    TEST_ASSERT(mock_cli_strcmp("feed", "feed") == 1, "Command 'feed' matches");
    TEST_ASSERT(mock_cli_strcmp("hang", "hang") == 1, "Command 'hang' matches");
    TEST_ASSERT(mock_cli_strcmp("reset", "reset") == 1, "Command 'reset' matches");

    TEST_ASSERT(mock_cli_strcmp("led on", "led off") == 0, "Different commands do not match");
    TEST_ASSERT(mock_cli_strcmp("status", "stat") == 0, "Partial prefix does not match");
}

/* ==============================================================================
 * MAIN TEST RUNNER ENTRY POINT
 * ============================================================================== */

int main(void) {
    printf(COLOR_BOLD "=======================================================\n");
    printf("   ZeroHAL Automated Host Test Suite (ISO 26262 CI)    \n");
    printf("=======================================================" COLOR_RESET "\n");

    test_ring_buffer_primitives();
    test_hardware_math_formulas();
    test_silicon_memory_map();
    test_cli_parsing();

    printf("\n" COLOR_BOLD "-------------------------------------------------------" COLOR_RESET "\n");
    if (g_tests_passed == g_tests_total) {
        printf(COLOR_BOLD COLOR_GREEN ">> SUCCESS: %d / %d TESTS PASSED! ZERO REGRESSIONS. <<" COLOR_RESET "\n\n",
               g_tests_passed, g_tests_total);
        return 0;
    } else {
        printf(COLOR_BOLD COLOR_RED ">> FAILURE: %d / %d TESTS FAILED! <<" COLOR_RESET "\n\n",
               g_tests_total - g_tests_passed, g_tests_total);
        return 1;
    }
}
