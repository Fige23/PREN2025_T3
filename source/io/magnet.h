#ifndef IO_MAGNET_H_
#define IO_MAGNET_H_

#include <stdbool.h>

void magnet_init(int dutyValue);
void magnet_set_duty_cycle(int dutyValue);
void magnet_pwm_enable(bool enable);
bool magnet_pwm_is_enabled(void);
void magnet_pwm_tick(void);

#endif /* IO_MAGNET_H_ */
