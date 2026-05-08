#include <stdbool.h>
#include <stdint.h>

#include "fsl_common.h"
#include "fsl_gpio.h"
#include "magnet.h"
#include "pin_mux.h"
#include "robot_config.h"
#include "magnet_config.h"


static volatile bool s_pwm_enabled = false;
static volatile uint32_t s_pwm_counter = 0u;
static volatile uint32_t s_pwm_on_ticks =
(MAGNET_PWM_PERIOD_TICKS * MAGNET_PWM_DEFAULT_DUTY_PERCENT) / 100u;

static uint8_t clamp_duty_percent(int dutyValue){
    if(dutyValue < 0){
        return 0U;
    }

    if(dutyValue > 100){
        return 100U;
    }

    return (uint8_t)dutyValue;
}

static uint32_t duty_percent_to_ticks(uint8_t dutyPercent){
    if(dutyPercent == 0U){
        return 0u;
    }

    if(dutyPercent >= 100U){
        return MAGNET_PWM_PERIOD_TICKS;
    }

    return ((MAGNET_PWM_PERIOD_TICKS * dutyPercent) + 99u) / 100u;
}

static void magnet_pin_write(bool on){
    GPIO_PinWrite(BOARD_INITPINS_Magnet_GPIO,
        BOARD_INITPINS_Magnet_PIN,
        on ? 1U : 0U);
}

static void magnet_apply_output(void){
    uint32_t onTicks = s_pwm_on_ticks;

    if((!s_pwm_enabled) || (onTicks == 0u)){
        magnet_pin_write(false);
        return;
    }

    if(onTicks >= MAGNET_PWM_PERIOD_TICKS){
        magnet_pin_write(true);
        return;
    }

    magnet_pin_write(s_pwm_counter < onTicks);
}

void magnet_init(int dutyValue){
    magnet_set_duty_cycle(dutyValue);
    magnet_pwm_enable(false);
}

void magnet_set_duty_cycle(int dutyValue){
    uint8_t dutyPercent = clamp_duty_percent(dutyValue);
    uint32_t irqMask = DisableGlobalIRQ();

    s_pwm_on_ticks = duty_percent_to_ticks(dutyPercent);
    magnet_apply_output();

    EnableGlobalIRQ(irqMask);
}

void magnet_pwm_enable(bool enable){
    uint32_t irqMask = DisableGlobalIRQ();

    s_pwm_enabled = enable;
    s_pwm_counter = 0u;
    magnet_apply_output();

    EnableGlobalIRQ(irqMask);
}

bool magnet_pwm_is_enabled(void){
    return s_pwm_enabled;
}

void magnet_pwm_tick(void){
    uint32_t onTicks;
    uint32_t counter;

    if(!s_pwm_enabled){
        return;
    }

    onTicks = s_pwm_on_ticks;
    if((onTicks == 0u) || (onTicks >= MAGNET_PWM_PERIOD_TICKS)){
        return;
    }

    counter = s_pwm_counter;
    if(counter == 0u){
        magnet_pin_write(true);
    }
    else if(counter == onTicks){
        magnet_pin_write(false);
    }

    counter++;
    if(counter >= MAGNET_PWM_PERIOD_TICKS){
        counter = 0u;
    }

    s_pwm_counter = counter;
}
