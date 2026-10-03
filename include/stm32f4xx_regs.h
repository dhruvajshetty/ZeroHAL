/**
 * ==============================================================================
 * Project: ZeroHAL
 * File: stm32f4xx_regs.h
 * Description: Bare-Metal Silicon Memory Map and Volatile Register Pointers
 * Target: STM32F4 Series (ARM Cortex-M4F)
 * Standard: ISO 26262 Deterministic Hardware Register Mapping
 * ==============================================================================
 * 
 * NOTE: Bypasses all vendor Hardware Abstraction Layers (HAL / CMSIS).
 * All hardware interactions occur through direct, volatile 32-bit MMIO pointers.
 */

#ifndef STM32F4XX_REGS_H
#define STM32F4XX_REGS_H

#include <stdint.h>

/* ==============================================================================
 * 1. PERIPHERAL BUS BASE ADDRESSES (Physical Memory Map)
 * ============================================================================== */
#define PERIPH_BASE             0x40000000UL
#define APB1PERIPH_BASE         PERIPH_BASE
#define AHB1PERIPH_BASE         (PERIPH_BASE + 0x00020000UL)

/* Core System Control Space (ARM Cortex-M Internal Peripherals) */
#define SCS_BASE                0xE000E000UL
#define SYSTICK_BASE            (SCS_BASE + 0x0010UL)
#define NVIC_BASE               (SCS_BASE + 0x0100UL)
#define SCB_BASE                (SCS_BASE + 0x0D00UL)

/* Individual Peripheral Base Addresses */
#define GPIOA_BASE              (AHB1PERIPH_BASE + 0x0000UL)
#define RCC_BASE                (AHB1PERIPH_BASE + 0x3800UL)
#define USART2_BASE             (APB1PERIPH_BASE + 0x4400UL)
#define IWDG_BASE               (APB1PERIPH_BASE + 0x3000UL)
#define TIM2_BASE               (APB1PERIPH_BASE + 0x0000UL)

/* ==============================================================================
 * 2. MEMORY-MAPPED VOLATILE REGISTERS (Hardware Mailboxes)
 * ============================================================================== */

/* --- Reset and Clock Control (RCC) --- */
#define RCC_AHB1ENR             (*((volatile uint32_t *)(RCC_BASE + 0x30UL)))
#define RCC_APB1ENR             (*((volatile uint32_t *)(RCC_BASE + 0x40UL)))
#define RCC_CSR                 (*((volatile uint32_t *)(RCC_BASE + 0x74UL)))

/* RCC Bitfield Definitions */
#define RCC_AHB1ENR_GPIOAEN     (1UL << 0)   /* Bit 0: GPIO Port A Clock Enable */
#define RCC_APB1ENR_TIM2EN      (1UL << 0)   /* Bit 0: TIM2 Peripheral Clock Enable */
#define RCC_APB1ENR_USART2EN    (1UL << 17)  /* Bit 17: USART2 Peripheral Clock Enable */
#define RCC_CSR_RMVF            (1UL << 24)  /* Bit 24: Remove Reset Flag */
#define RCC_CSR_IWDGRSTF        (1UL << 29)  /* Bit 29: Independent Watchdog Reset Flag */

/* --- General Purpose I/O Port A (GPIOA) --- */
#define GPIOA_MODER             (*((volatile uint32_t *)(GPIOA_BASE + 0x00UL)))
#define GPIOA_ODR               (*((volatile uint32_t *)(GPIOA_BASE + 0x14UL)))
#define GPIOA_PUPDR             (*((volatile uint32_t *)(GPIOA_BASE + 0x0CUL)))
#define GPIOA_AFRL              (*((volatile uint32_t *)(GPIOA_BASE + 0x20UL)))
#define GPIOA_BSRR              (*((volatile uint32_t *)(GPIOA_BASE + 0x18UL)))

/* GPIO Bitfield Definitions */
#define GPIO_PIN_2              (1UL << 2)
#define GPIO_PIN_3              (1UL << 3)
#define GPIO_PIN_5              (1UL << 5)

/* --- USART2 Serial Controller --- */
#define USART2_SR               (*((volatile uint32_t *)(USART2_BASE + 0x00UL)))
#define USART2_DR               (*((volatile uint32_t *)(USART2_BASE + 0x04UL)))
#define USART2_BRR              (*((volatile uint32_t *)(USART2_BASE + 0x08UL)))
#define USART2_CR1              (*((volatile uint32_t *)(USART2_BASE + 0x0CUL)))

/* USART Status Register (SR) Bitfields */
#define USART_SR_RXNE           (1UL << 5)   /* Bit 5: Read Data Register Not Empty */
#define USART_SR_TC             (1UL << 6)   /* Bit 6: Transmission Complete */
#define USART_SR_TXE            (1UL << 7)   /* Bit 7: Transmit Data Register Empty */

/* USART Control Register 1 (CR1) Bitfields */
#define USART_CR1_RE            (1UL << 2)   /* Bit 2: Receiver Enable */
#define USART_CR1_TE            (1UL << 3)   /* Bit 3: Transmitter Enable */
#define USART_CR1_RXNEIE        (1UL << 5)   /* Bit 5: RXNE Interrupt Enable */
#define USART_CR1_UE            (1UL << 13)  /* Bit 13: USART Peripheral Enable */

/* --- Independent Watchdog (IWDG - ISO 26262 Safety) --- */
#define IWDG_KR                 (*((volatile uint32_t *)(IWDG_BASE + 0x00UL)))
#define IWDG_PR                 (*((volatile uint32_t *)(IWDG_BASE + 0x04UL)))
#define IWDG_RLR                (*((volatile uint32_t *)(IWDG_BASE + 0x08UL)))
#define IWDG_SR                 (*((volatile uint32_t *)(IWDG_BASE + 0x0CUL)))

/* IWDG Magic Key Codes */
#define IWDG_KEY_ACCESS         0x5555UL     /* Unlock PR and RLR register write access */
#define IWDG_KEY_RELOAD         0xAAAAUL     /* Feed/reload the watchdog downcounter */
#define IWDG_KEY_ENABLE         0xCCCCUL     /* Start the watchdog timer (irreversible) */

/* IWDG Prescaler & Status Bitfields */
#define IWDG_PR_DIV64           0x04UL       /* 32kHz / 64 = 500 Hz (2ms per counter tick) */
#define IWDG_SR_PVU             (1UL << 0)   /* Prescaler Value Update flag */
#define IWDG_SR_RVU             (1UL << 1)   /* Reload Value Update flag */

/* --- ARM Cortex-M4 Core System Peripherals --- */
#define STK_CTRL                (*((volatile uint32_t *)(SYSTICK_BASE + 0x00UL)))
#define STK_LOAD                (*((volatile uint32_t *)(SYSTICK_BASE + 0x04UL)))
#define STK_VAL                 (*((volatile uint32_t *)(SYSTICK_BASE + 0x08UL)))

#define STK_CTRL_ENABLE         (1UL << 0)   /* SysTick Counter Enable */
#define STK_CTRL_TICKINT        (1UL << 1)   /* SysTick Interrupt Request Enable */
#define STK_CTRL_CLKSOURCE      (1UL << 2)   /* Clock Source (1 = Processor Core Clock) */

#define NVIC_ISER1              (*((volatile uint32_t *)(NVIC_BASE + 0x04UL)))
#define USART2_IRQN             38           /* USART2 Global Interrupt Position */

#define SCB_AIRCR               (*((volatile uint32_t *)(SCB_BASE + 0x0CUL)))
#define SCB_AIRCR_VECTKEY       (0x05FAUL << 16)
#define SCB_AIRCR_SYSRESETREQ   (1UL << 2)

#define SCB_CPACR               (*((volatile uint32_t *)(SCB_BASE + 0x88UL)))

/* --- TIM2 Timer --- */
#define TIM2_CR1                (*((volatile uint32_t *)(TIM2_BASE + 0x00UL)))
#define TIM2_CCMR1              (*((volatile uint32_t *)(TIM2_BASE + 0x18UL)))
#define TIM2_CCER               (*((volatile uint32_t *)(TIM2_BASE + 0x20UL)))
#define TIM2_PSC                (*((volatile uint32_t *)(TIM2_BASE + 0x28UL)))
#define TIM2_ARR                (*((volatile uint32_t *)(TIM2_BASE + 0x2CUL)))
#define TIM2_CCR1               (*((volatile uint32_t *)(TIM2_BASE + 0x34UL)))

#endif /* STM32F4XX_REGS_H */
