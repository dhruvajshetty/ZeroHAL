#ifndef PWM_H
#define PWM_H

#include <stdint.h>

/**
 * Initialize TIM2 Channel 1 on PA5 for PWM output.
 * Base frequency: 1 kHz (assuming 16MHz clock).
 */
void pwm_init(void);

/**
 * Set the duty cycle for the PWM output.
 * @param duty_percent Duty cycle from 0 to 100.
 */
void pwm_set_duty(uint8_t duty_percent);

#endif // PWM_H
