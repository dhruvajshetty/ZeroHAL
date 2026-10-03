#include "zero_hal.h"

void pwm_init(void) {
    // 1. Enable clocks for GPIOA and TIM2
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC_APB1ENR |= RCC_APB1ENR_TIM2EN;

    // 2. Configure PA5 as Alternate Function mode (AF1 for TIM2_CH1)
    // Clear mode bits for PA5, then set to 10 (Alternate Function)
    GPIOA_MODER &= ~(3UL << (5 * 2));
    GPIOA_MODER |=  (2UL << (5 * 2));

    // Set AF1 (0001) in AFRL (Alternate Function Low) for PA5 (bits 20-23)
    GPIOA_AFRL &= ~(15UL << (5 * 4));
    GPIOA_AFRL |=  (1UL << (5 * 4));

    // 3. Configure TIM2
    // Assuming a 16MHz default CPU clock (HSI)
    // We want a 1MHz timer clock, so PSC = 16 - 1 = 15
    TIM2_PSC = 16 - 1;

    // We want a 1kHz PWM frequency, so ARR = 1MHz / 1kHz - 1 = 999
    TIM2_ARR = 1000 - 1;

    // 4. Configure Channel 1 for PWM Mode 1
    // Clear CCMR1 bits 4-6, then set to 110 (PWM Mode 1)
    // And enable Output Compare Preload (bit 3)
    TIM2_CCMR1 &= ~(7UL << 4);
    TIM2_CCMR1 |=  (6UL << 4) | (1UL << 3);

    // 5. Enable output on Channel 1 (CC1E bit 0)
    TIM2_CCER |= (1UL << 0);

    // 6. Set initial duty cycle to 0
    TIM2_CCR1 = 0;

    // 7. Enable the timer counter (CEN bit 0)
    TIM2_CR1 |= (1UL << 0);
}

void pwm_set_duty(uint8_t duty_percent) {
    if (duty_percent > 100) {
        duty_percent = 100;
    }
    
    // Calculate register value based on Auto-Reload Register (ARR)
    // If ARR = 999, 100% duty = 1000.
    TIM2_CCR1 = (TIM2_ARR + 1) * duty_percent / 100;
}
