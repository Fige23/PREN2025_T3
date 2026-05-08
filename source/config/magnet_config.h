#ifndef MAGNET_CONFIG_H
#define MAGNET_CONFIG_H

#define MAGNET_CHECK_ENABLE                     1

#define MAGNET_PWM_FREQUENCY_HZ             1000u
#define MAGNET_PWM_DEFAULT_DUTY_PERCENT       10u
#define MAGNET_PWM_PERIOD_TICKS              (STEP_TICK_HZ / MAGNET_PWM_FREQUENCY_HZ)

#if MAGNET_PWM_PERIOD_TICKS < 2u
#error "MAGNET_PWM_FREQUENCY_HZ is too high for STEP_TICK_HZ"
#endif


#endif 