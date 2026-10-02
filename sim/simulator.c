/**
 * ==============================================================================
 * Project: ZeroHAL
 * File: simulator.c
 * Description: Silicon-Level Bus & Register Virtual Emulator for Host Execution
 * ==============================================================================
 * 
 * This emulator creates a virtual memory-mapped silicon environment on your PC.
 * Every memory address, bus access (AHB/APB/Core System Control Space), and
 * peripheral state change is logged so you can inspect the inner workings of
 * microcontroller silicon gates and register mailboxes without physical hardware.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ANSI Terminal Colors for Silicon Visualization */
#define COLOR_RESET   "\033[0m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_RED     "\033[31m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_BLUE    "\033[34m"

/* ==============================================================================
 * 1. VIRTUAL SILICON MEMORY MAP (Simulating physical 32-bit address space)
 * ============================================================================== */

typedef struct {
    /* RCC Registers (0x40023800) */
    uint32_t RCC_AHB1ENR;
    uint32_t RCC_APB1ENR;
    uint32_t RCC_CSR;

    /* GPIOA Registers (0x40020000) */
    uint32_t GPIOA_MODER;
    uint32_t GPIOA_ODR;
    uint32_t GPIOA_PUPDR;
    uint32_t GPIOA_AFRL;

    /* USART2 Registers (0x40004400) */
    uint32_t USART2_SR;
    uint32_t USART2_DR;
    uint32_t USART2_BRR;
    uint32_t USART2_CR1;

    /* IWDG Registers (0x40003000) */
    uint32_t IWDG_KR;
    uint32_t IWDG_PR;
    uint32_t IWDG_RLR;
    uint32_t IWDG_SR;
    int      iwdg_active;
    int32_t  iwdg_counter_ms;

    /* ARM Cortex-M Core System Space (0xE000E000) */
    uint32_t NVIC_ISER1;
    uint32_t STK_CTRL;
    uint32_t STK_LOAD;
    uint32_t STK_VAL;
    uint32_t SCB_AIRCR;

    /* Physical Silicon Pin Voltages & States */
    float pin_PA2_tx_voltage;
    float pin_PA3_rx_voltage;
    int   pin_PA5_led_state; /* 0 = 0.0V (LOW), 1 = 3.3V (HIGH) */
} SiliconState;

static SiliconState silicon = {
    .USART2_SR = (1 << 7),      /* TXE = 1 (Transmit register empty at reset) */
    .iwdg_counter_ms = 2000,
    .pin_PA2_tx_voltage = 3.3f, /* UART idle line is HIGH */
    .pin_PA3_rx_voltage = 3.3f,
    .pin_PA5_led_state = 0
};

/* ==============================================================================
 * 2. BUS INTERCONNECT & MMIO LOGGING
 * ============================================================================== */

void log_bus_write(const char *bus, uint32_t addr, const char *reg_name,
                   uint32_t old_val, uint32_t new_val, const char *effect) {
    printf(COLOR_CYAN "[BUS WRITE: %-6s]" COLOR_RESET
           " Addr: 0x%08X (" COLOR_BOLD "%-12s" COLOR_RESET
           ") = 0x%08X (was 0x%08X) -> " COLOR_GREEN "%s" COLOR_RESET "\n",
           bus, addr, reg_name, new_val, old_val, effect);
}

void sim_write_RCC_AHB1ENR(uint32_t val) {
    uint32_t old = silicon.RCC_AHB1ENR;
    silicon.RCC_AHB1ENR = val;
    if ((val & (1 << 0)) && !(old & (1 << 0))) {
        log_bus_write("AHB1", 0x40023830, "RCC_AHB1ENR", old, val,
                      "Gating 16MHz clock to GPIO Port A silicon gates");
    }
}

void sim_write_RCC_APB1ENR(uint32_t val) {
    uint32_t old = silicon.RCC_APB1ENR;
    silicon.RCC_APB1ENR = val;
    if ((val & (1 << 17)) && !(old & (1 << 17))) {
        log_bus_write("APB1", 0x40023840, "RCC_APB1ENR", old, val,
                      "Gating clock to USART2 peripheral hardware");
    }
}

void sim_write_GPIOA_MODER(uint32_t val) {
    uint32_t old = silicon.GPIOA_MODER;
    silicon.GPIOA_MODER = val;
    uint32_t pa2_mode = (val >> (2 * 2)) & 0x3;
    uint32_t pa3_mode = (val >> (3 * 2)) & 0x3;
    uint32_t pa5_mode = (val >> (5 * 2)) & 0x3;
    char detail[128];
    snprintf(detail, sizeof(detail), "Pin Modes -> PA2: %s, PA3: %s, PA5: %s",
             pa2_mode == 2 ? "AlternateFunction" : "GPIO",
             pa3_mode == 2 ? "AlternateFunction" : "GPIO",
             pa5_mode == 1 ? "Output(LED)" : "Other");
    log_bus_write("AHB1", 0x40020000, "GPIOA_MODER", old, val, detail);
}

void sim_write_GPIOA_AFRL(uint32_t val) {
    uint32_t old = silicon.GPIOA_AFRL;
    silicon.GPIOA_AFRL = val;
    uint32_t af_pa2 = (val >> 8) & 0xF;
    uint32_t af_pa3 = (val >> 12) & 0xF;
    char detail[128];
    snprintf(detail, sizeof(detail),
             "Pin Multiplexer -> PA2 routed to AF%d (USART2_TX), PA3 routed to AF%d (USART2_RX)",
             af_pa2, af_pa3);
    log_bus_write("AHB1", 0x40020020, "GPIOA_AFRL", old, val, detail);
}

void sim_write_GPIOA_ODR(uint32_t val) {
    int old_led = silicon.pin_PA5_led_state;
    silicon.GPIOA_ODR = val;
    silicon.pin_PA5_led_state = (val & (1 << 5)) ? 1 : 0;
    if (silicon.pin_PA5_led_state != old_led) {
        printf(COLOR_YELLOW "[SILICON PIN EVENT]" COLOR_RESET " PA5 Physical Voltage: %s (Green LED %s)\n",
               silicon.pin_PA5_led_state ? "3.3V (HIGH)" : "0.0V (LOW)",
               silicon.pin_PA5_led_state ? "ILLUMINATED [*]" : "OFF [ ]");
    }
}

void sim_write_USART2_BRR(uint32_t val) {
    uint32_t old = silicon.USART2_BRR;
    silicon.USART2_BRR = val;
    uint32_t mantissa = (val >> 4) & 0x0FFF;
    uint32_t fraction = val & 0x0F;
    char detail[128];
    snprintf(detail, sizeof(detail),
             "Baud Generator: Mantissa=%d, Fraction=%d/16 -> 115200 bps @ 16MHz",
             mantissa, fraction);
    log_bus_write("APB1", 0x40004408, "USART2_BRR", old, val, detail);
}

void sim_write_USART2_CR1(uint32_t val) {
    uint32_t old = silicon.USART2_CR1;
    silicon.USART2_CR1 = val;
    char detail[128];
    snprintf(detail, sizeof(detail),
             "Transmitter: %s | Receiver: %s | RXNE Interrupt: %s | Peripheral: %s",
             (val & (1 << 3)) ? "ENABLED" : "OFF",
             (val & (1 << 2)) ? "ENABLED" : "OFF",
             (val & (1 << 5)) ? "ARMED" : "MASKED",
             (val & (1 << 13)) ? "RUNNING" : "STOPPED");
    log_bus_write("APB1", 0x4000440C, "USART2_CR1", old, val, detail);
}

void sim_write_NVIC_ISER1(uint32_t val) {
    uint32_t old = silicon.NVIC_ISER1;
    silicon.NVIC_ISER1 |= val;
    if (val & (1 << (38 - 32))) {
        log_bus_write("SCS", 0xE000E104, "NVIC_ISER1", old, val,
                      "ARM Cortex-M4 Core unmasks IRQ 38 (USART2 global interrupt)");
    }
}

void sim_write_STK_CTRL(uint32_t val) {
    uint32_t old = silicon.STK_CTRL;
    silicon.STK_CTRL = val;
    log_bus_write("SCS", 0xE000E010, "STK_CTRL", old, val,
                  "SysTick 24-bit core down-counter activated (1ms interrupt armed)");
}

void sim_write_IWDG_KR(uint32_t val) {
    uint32_t old = silicon.IWDG_KR;
    silicon.IWDG_KR = val;
    if (val == 0x5555) {
        log_bus_write("APB1", 0x40003000, "IWDG_KR", old, val,
                      "Watchdog PR/RLR write access UNLOCKED");
    } else if (val == 0xCCCC) {
        silicon.iwdg_active = 1;
        silicon.iwdg_counter_ms = 2000;
        log_bus_write("APB1", 0x40003000, "IWDG_KR", old, val,
                      "Watchdog STARTED (2000ms countdown on 32kHz LSI clock)");
    } else if (val == 0xAAAA) {
        silicon.iwdg_counter_ms = 2000;
    }
}

/* ==============================================================================
 * 3. ARCHITECTURE VISUALIZATION
 * ============================================================================== */

void print_silicon_schematic(void) {
    printf("\n" COLOR_BOLD COLOR_MAGENTA);
    printf("===============================================================================\n");
    printf("                  STM32F4 PHYSICAL SILICON ARCHITECTURE                        \n");
    printf("===============================================================================\n" COLOR_RESET);
    printf("                                                                               \n");
    printf("  +-------------------------------------------------------------------------+  \n");
    printf("  |                          ARM Cortex-M4 CPU CORE                         |  \n");
    printf("  |                                                                         |  \n");
    printf("  |   [SysTick Timer: 0xE000E010]           [NVIC Controller: 0xE000E100]   |  \n");
    printf("  |       * 1ms system heartbeat                * IRQ 38 (USART2 Handler)   |  \n");
    printf("  +-----------------------------------+-------------------------------------+  \n");
    printf("                                      |                                        \n");
    printf("                                AHB Bus Matrix (16MHz)                         \n");
    printf("                                      |                                        \n");
    printf("         +----------------------------+-----------------------------+          \n");
    printf("         |                                                          |          \n");
    printf("  [Flash: 0x08000000]                                       [SRAM: 0x20000000] \n");
    printf("   * .isr_vector                                             * .data / .bss    \n");
    printf("   * .text (Code instructions)                               * Ring Buffer FIFO\n");
    printf("   * .rodata (Strings)                                       * Full-descending \n");
    printf("   * .data LMA (Flash Image)                                   Stack (_estack) \n");
    printf("         |                                                          |          \n");
    printf("  +------+----------------------------------------------------------+-------+  \n");
    printf("  | AHB1 Bus Bridge                                                         |  \n");
    printf("  |  * RCC Clock Gating (0x40023800)                                        |  \n");
    printf("  |  * GPIO Port A (0x40020000)                                             |  \n");
    printf("  |      Pin PA2 <---> Alternate Function AF7 <---> USART2_TX (Serial Out)  |  \n");
    printf("  |      Pin PA3 <---> Alternate Function AF7 <---> USART2_RX (Serial In)   |  \n");
    printf("  |      Pin PA5 <---> Output Driver <------------> User Green LED (LD2)    |  \n");
    printf("  +------+------------------------------------------------------------------+  \n");
    printf("         |                                                                     \n");
    printf("  +------+------------------------------------------------------------------+  \n");
    printf("  | APB1 Bus Bridge                                                         |  \n");
    printf("  |  * USART2 Engine (0x40004400) -> 115200 Baud Generator, SR, DR, CR1    |  \n");
    printf("  |  * IWDG Independent Watchdog (0x40003000) -> 32kHz LSI Clocked Reset    |  \n");
    printf("  +-------------------------------------------------------------------------+  \n");
    printf("\n");
}

int main(int argc, char *argv[]) {
    int headless = (argc > 1 && strcmp(argv[1], "--headless") == 0);

    print_silicon_schematic();

    printf(COLOR_BOLD "[STEP 1] EXECUTING BOOTLOADER / RESET_HANDLER (Memory Realignment)" COLOR_RESET "\n");
    printf("  -> Copying .data section from LMA (Flash 0x08000000) to VMA (SRAM 0x20000000)...\n");
    printf("  -> Zeroing .bss section in SRAM (clearing 64-byte circular ring buffer)...\n");
    printf("  -> Enabling FPU coprocessor CP10 & CP11 in CPACR...\n\n");

    printf(COLOR_BOLD "[STEP 2] HARDWARE BUS TRANSACTION TRACE (Silicon Boot):" COLOR_RESET "\n");

    /* 1. RCC Clock Gating */
    sim_write_RCC_AHB1ENR(0x00000001); /* GPIOA Clock */
    sim_write_RCC_APB1ENR(0x00020000); /* USART2 Clock (Bit 17) */

    /* 2. GPIO Pin Muxing */
    sim_write_GPIOA_MODER(0x000004A0); /* PA2=AF, PA3=AF, PA5=Out */
    sim_write_GPIOA_AFRL(0x00007700);  /* AF7 for PA2 & PA3 */

    /* 3. USART Configuration */
    sim_write_USART2_BRR(0x008B);      /* 115200 baud */
    sim_write_USART2_CR1(0x0000202C);  /* UE=1, RXNEIE=1, TE=1, RE=1 */

    /* 4. Cortex-M Core Peripherals */
    sim_write_NVIC_ISER1(1 << 6);      /* IRQ 38 unmask */
    sim_write_STK_CTRL(0x07);          /* SysTick enable */

    /* 5. IWDG Safety Watchdog */
    sim_write_IWDG_KR(0x5555);
    sim_write_IWDG_KR(0xCCCC);

    printf("\n" COLOR_BOLD COLOR_GREEN ">>> ZERO-HAL SILICON INITIALIZATION COMPLETE! <<<" COLOR_RESET "\n");
    printf("The MCU is now actively running its super-loop with SysTick and NVIC interrupts!\n\n");

    if (headless) {
        printf(COLOR_GREEN "[HEADLESS PASS] Silicon boot trace executed successfully.\n" COLOR_RESET);
        return 0;
    }

    printf(COLOR_BOLD "--- TEST THE RUNNING FIRMWARE INTERACTIVELY ---" COLOR_RESET "\n");
    printf("Type commands into the virtual terminal: " COLOR_CYAN
           "help, status, led on, led off, led toggle, feed, hang, exit" COLOR_RESET "\n\n");

    char line[128];
    while (1) {
        printf(COLOR_BOLD "ZeroHAL> " COLOR_RESET);
        if (!fgets(line, sizeof(line), stdin)) break;

        /* Strip trailing newlines */
        line[strcspn(line, "\r\n")] = 0;

        if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0) {
            printf("\nExiting silicon simulator.\n");
            break;
        } else if (strcmp(line, "help") == 0) {
            printf("\r\n========================================\r\n");
            printf("  ZeroHAL Bare-Metal Diagnostic Console  \r\n");
            printf("========================================\r\n");
            printf("  help       - Show available ECU commands\r\n");
            printf("  status     - Show uptime, LED, and IWDG status\r\n");
            printf("  led on     - Drive PA5 output HIGH\r\n");
            printf("  led off    - Drive PA5 output LOW\r\n");
            printf("  led toggle - Invert PA5 output state\r\n");
            printf("  feed       - Manually kick the IWDG watchdog\r\n");
            printf("  hang       - Freeze CPU to trigger Watchdog Reset\r\n");
            printf("  exit       - Exit this simulator\r\n\r\n");
        } else if (strcmp(line, "status") == 0) {
            printf("\r\n[ECU TELEMETRY]\r\n");
            printf("  Uptime        : 14 seconds (14000 ms)\r\n");
            printf("  PA5 LED State : %s\r\n", silicon.pin_PA5_led_state ? "ON (3.3V)" : "OFF (0.0V)");
            printf("  IWDG Hardware : ACTIVE (2000ms window, 32kHz LSI)\r\n");
            printf("  Ring Buffer   : 0 bytes pending, lock-free FIFO\r\n\r\n");
        } else if (strcmp(line, "led on") == 0) {
            sim_write_GPIOA_ODR(silicon.GPIOA_ODR | (1 << 5));
            printf("[OK] PA5 LED is ON\r\n");
        } else if (strcmp(line, "led off") == 0) {
            sim_write_GPIOA_ODR(silicon.GPIOA_ODR & ~(1 << 5));
            printf("[OK] PA5 LED is OFF\r\n");
        } else if (strcmp(line, "led toggle") == 0) {
            sim_write_GPIOA_ODR(silicon.GPIOA_ODR ^ (1 << 5));
            printf("[OK] PA5 LED toggled\r\n");
        } else if (strcmp(line, "feed") == 0) {
            sim_write_IWDG_KR(0xAAAA);
            printf("[OK] IWDG watchdog timer refreshed.\r\n");
        } else if (strcmp(line, "hang") == 0) {
            printf(COLOR_RED "\r\n[SIMULATION] Entering infinite loop without feeding watchdog...\r\n" COLOR_RESET);
            printf(COLOR_YELLOW "[T+0ms] Main loop halted. CPU stalled.\n" COLOR_RESET);
            printf(COLOR_YELLOW "[T+500ms] IWDG downcounter: 1500ms remaining...\n" COLOR_RESET);
            printf(COLOR_YELLOW "[T+1000ms] IWDG downcounter: 1000ms remaining...\n" COLOR_RESET);
            printf(COLOR_YELLOW "[T+1500ms] IWDG downcounter: 500ms remaining...\n" COLOR_RESET);
            printf(COLOR_RED "[T+2000ms] IWDG COUNTER REACHED 0! HARDWARE RESET ASSERTED!\n" COLOR_RESET);
            printf(COLOR_RED "[HARDWARE RESET] RCC_CSR Bit 29 (IWDGRSTF) set by silicon circuitry!\n" COLOR_RESET);
            printf(COLOR_GREEN "[RECOVERY] Reset_Handler restarted. System restored safely.\n\n" COLOR_RESET);
        } else if (line[0] != '\0') {
            printf("Unknown command: '%s'. Type 'help' for command list.\r\n", line);
        }
    }

    return 0;
}
