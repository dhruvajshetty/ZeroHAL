/**
 * ==============================================================================
 * Project: ZeroHAL
 * File: startup.c
 * Description: Bare-Metal Silicon Startup Routine & Vector Table (Pure C)
 * Target: STM32F4 Series (ARM Cortex-M4F)
 * Standard: ISO 26262 Deterministic Bootstrapping (No vendor startup assembly)
 * ==============================================================================
 * 
 * BOOTSTRAP RESPONSIBILITIES:
 * 1. Defines the ARM Cortex-M Vector Table placed at 0x08000000 (.isr_vector).
 * 2. Copies initialized data (.data) from FLASH (LMA) into SRAM (VMA).
 * 3. Clears uninitialized data (.bss) in SRAM to 0.
 * 4. Enables FPU hardware coprocessor CP10/CP11 in CPACR.
 * 5. Branches control to main().
 */

#include <stdint.h>

/* Forward declaration of the Reset Handler and application entry */
void Reset_Handler(void);
extern int main(void);

/* Symbols exported from bare-metal linker script (linker.ld) */
extern uint32_t _estack;   /* Initial Top of Stack (Highest SRAM address) */
extern uint32_t _sidata;   /* Start of .data image in FLASH (Load address) */
extern uint32_t _sdata;    /* Start of .data in SRAM (Virtual address) */
extern uint32_t _edata;    /* End of .data in SRAM */
extern uint32_t _sbss;     /* Start of .bss in SRAM */
extern uint32_t _ebss;     /* End of .bss in SRAM */

/**
 * @brief Default trap handler for unhandled exceptions & interrupts.
 */
void Default_Handler(void) {
    while (1) {
        /* Hang CPU in deterministic loop with breakpoint instruction */
        __asm__ volatile ("bkpt #0");
    }
}

/* System Exception Handlers (Weak aliases allow application overrides) */
void NMI_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)    __attribute__((weak, alias("Default_Handler")));

/* External Peripheral IRQs */
extern void USART2_IRQHandler(void);

void WWDG_IRQHandler(void)               __attribute__((weak, alias("Default_Handler")));
void PVD_IRQHandler(void)                __attribute__((weak, alias("Default_Handler")));
void TAMP_STAMP_IRQHandler(void)         __attribute__((weak, alias("Default_Handler")));
void RTC_WKUP_IRQHandler(void)           __attribute__((weak, alias("Default_Handler")));
void FLASH_IRQHandler(void)              __attribute__((weak, alias("Default_Handler")));
void RCC_IRQHandler(void)                __attribute__((weak, alias("Default_Handler")));
void EXTI0_IRQHandler(void)              __attribute__((weak, alias("Default_Handler")));
void EXTI1_IRQHandler(void)              __attribute__((weak, alias("Default_Handler")));
void EXTI2_IRQHandler(void)              __attribute__((weak, alias("Default_Handler")));
void EXTI3_IRQHandler(void)              __attribute__((weak, alias("Default_Handler")));
void EXTI4_IRQHandler(void)              __attribute__((weak, alias("Default_Handler")));

/*
 * ==============================================================================
 * ARM CORTEX-M4 VECTOR TABLE
 * Placed explicitly into the ".isr_vector" section defined in linker.ld
 * ==============================================================================
 */
__attribute__((section(".isr_vector")))
void (* const g_pfnVectors[])(void) = {
    (void (*)(void))(&_estack),     /* 0: Initial Main Stack Pointer (MSP) */
    Reset_Handler,                  /* 1: Reset Handler */
    NMI_Handler,                    /* 2: Non-Maskable Interrupt */
    HardFault_Handler,              /* 3: Hard Fault Handler */
    MemManage_Handler,              /* 4: Memory Management Fault */
    BusFault_Handler,               /* 5: Bus Fault */
    UsageFault_Handler,             /* 6: Usage Fault */
    0, 0, 0, 0,                     /* 7-10: Reserved */
    SVC_Handler,                    /* 11: Supervisor Call */
    DebugMon_Handler,               /* 12: Debug Monitor */
    0,                              /* 13: Reserved */
    PendSV_Handler,                 /* 14: PendSV */
    SysTick_Handler,                /* 15: System Tick Timer */

    /* External Interrupts (STM32F4 Specific) */
    WWDG_IRQHandler,                /* 16: Window Watchdog */
    PVD_IRQHandler,                 /* 17: PVD through EXTI Line detect */
    TAMP_STAMP_IRQHandler,          /* 18: Tamper and TimeStamp */
    RTC_WKUP_IRQHandler,            /* 19: RTC Wakeup */
    FLASH_IRQHandler,               /* 20: FLASH */
    RCC_IRQHandler,                 /* 21: RCC */
    EXTI0_IRQHandler,               /* 22: EXTI Line 0 */
    EXTI1_IRQHandler,               /* 23: EXTI Line 1 */
    EXTI2_IRQHandler,               /* 24: EXTI Line 2 */
    EXTI3_IRQHandler,               /* 25: EXTI Line 3 */
    EXTI4_IRQHandler,               /* 26: EXTI Line 4 */
    0, 0, 0, 0, 0, 0, 0,            /* 27-33: DMA, ADC, etc. */
    0, 0, 0, 0, 0, 0, 0, 0,         /* 34-41: CAN, TIM, etc. */
    0, 0, 0, 0, 0, 0, 0, 0,         /* 42-49: I2C, SPI, etc. */
    0, 0, 0, 0,                     /* 50-53: USART1, etc. */
    USART2_IRQHandler,              /* 54: Position 38 (IRQ 38 = 16 core exceptions + 38 = Vector 54) */
};

/**
 * ==============================================================================
 * RESET HANDLER
 * Silicon entry point following Power-On Reset (POR) or System Reset.
 * ==============================================================================
 */
void Reset_Handler(void) {
    /* 1. Copy initialized data (.data) from FLASH (LMA) to SRAM (VMA) */
    uint32_t *p_src = &_sidata;
    uint32_t *p_dst = &_sdata;

    while (p_dst < &_edata) {
        *p_dst++ = *p_src++;
    }

    /* 2. Zero-out uninitialized data (.bss) in SRAM */
    uint32_t *p_bss = &_sbss;
    while (p_bss < &_ebss) {
        *p_bss++ = 0U;
    }

    /* 3. Enable FPU (Floating Point Unit) hardware coprocessor CP10 and CP11 */
    volatile uint32_t *cpacr = (volatile uint32_t *)0xE000ED88UL;
    *cpacr |= ((3UL << (10 * 2)) | (3UL << (11 * 2)));

    /* 4. Branch to main application */
    (void)main();

    /* Safety trap: embedded systems should never return from main */
    while (1) {
        __asm__ volatile ("wfi");
    }
}
