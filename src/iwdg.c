/**
 * ==============================================================================
 * Project: ZeroHAL
 * File: iwdg.c
 * Description: Independent Watchdog (IWDG) Driver for ISO 26262 ASIL Compliance
 * ==============================================================================
 */

#include "iwdg.h"
#include "stm32f4xx_regs.h"

void iwdg_init(uint16_t timeout_ms) {
    /* 1. Unlock write access to PR and RLR registers */
    IWDG_KR = IWDG_KEY_ACCESS;

    /* 2. Wait for prescaler register update flag to clear */
    while (IWDG_SR & IWDG_SR_PVU) {
        /* Wait for hardware register sync */
    }

    /* Set prescaler to /64 (32 kHz LSI / 64 = 500 Hz -> 2ms per tick) */
    IWDG_PR = IWDG_PR_DIV64;

    /* 3. Wait for reload register update flag to clear */
    while (IWDG_SR & IWDG_SR_RVU) {
        /* Wait for hardware register sync */
    }

    /* Calculate 12-bit reload value (max 4095 ticks = 8190 ms) */
    uint32_t reload = (uint32_t)timeout_ms / 2U;
    if (reload > 0x0FFFUL) {
        reload = 0x0FFFUL;
    }
    IWDG_RLR = reload;

    /* 4. Reload counter with initial RLR value */
    IWDG_KR = IWDG_KEY_RELOAD;

    /* 5. Start the watchdog timer. Irreversible in silicon until CPU reset! */
    IWDG_KR = IWDG_KEY_ENABLE;
}

void iwdg_feed(void) {
    /* Reload the 12-bit downcounter with RLR */
    IWDG_KR = IWDG_KEY_RELOAD;
}

uint8_t iwdg_caused_reset(void) {
    /* Check bit 29 of RCC_CSR */
    return (RCC_CSR & RCC_CSR_IWDGRSTF) ? 1U : 0U;
}

void iwdg_clear_reset_flags(void) {
    /* Writing 1 to RMVF (bit 24) clears all reset status flags */
    RCC_CSR |= RCC_CSR_RMVF;
}

void mcu_system_reset(void) {
    /* ARM Cortex-M System Reset: Key 0x05FA + SYSRESETREQ bit */
    SCB_AIRCR = SCB_AIRCR_VECTKEY | SCB_AIRCR_SYSRESETREQ;
    while (1) {
        /* Wait for core reset assertion */
    }
}
