/**
 * ==============================================================================
 * Project: ZeroHAL
 * File: iwdg.h
 * Description: Independent Watchdog (IWDG) Driver for ISO 26262 ASIL Compliance
 * ==============================================================================
 */

#ifndef ZERO_HAL_IWDG_H
#define ZERO_HAL_IWDG_H

#include <stdint.h>

/**
 * @brief Initialize and start the independent watchdog with specified timeout.
 *        Once started, the hardware cannot be disabled except via CPU reset.
 * @param timeout_ms Watchdog timeout duration in milliseconds.
 */
void iwdg_init(uint16_t timeout_ms);

/**
 * @brief Feed/refresh the watchdog downcounter with key 0xAAAA.
 */
void iwdg_feed(void);

/**
 * @brief Inspect RCC_CSR register to verify if prior boot was caused by IWDG timeout.
 * @return 1 if reset was caused by watchdog timeout, 0 otherwise.
 */
uint8_t iwdg_caused_reset(void);

/**
 * @brief Clear hardware reset flags in RCC_CSR.
 */
void iwdg_clear_reset_flags(void);

/**
 * @brief Trigger immediate ARM Cortex-M core reset via SCB_AIRCR.
 */
void mcu_system_reset(void);

#endif /* ZERO_HAL_IWDG_H */
