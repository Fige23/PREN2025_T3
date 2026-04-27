/*Project: PREN_Puzzleroboter
 *
 * Minimal, register-level TMC2209 UART module for four addressed drivers.
 */

#ifndef TMC2209_TMC2209_H_
#define TMC2209_TMC2209_H_

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    DRIVER_MOTOR_X = 0,
    DRIVER_MOTOR_Y = 1,
    DRIVER_MOTOR_Z = 2,
    DRIVER_MOTOR_PHI = 3,
    DRIVER_MOTOR_COUNT = 4
} driver_motor_e;

typedef enum {
    TMC2209_STATUS_OK = 0,
    TMC2209_STATUS_BAD_ARG,
    TMC2209_STATUS_TIMEOUT,
    TMC2209_STATUS_UART_ERROR,
    TMC2209_STATUS_CRC_ERROR,
    TMC2209_STATUS_DISABLED
} tmc2209_status_e;

void tmc2209_init(void);
tmc2209_status_e tmc2209_configure_defaults(void);

tmc2209_status_e tmc2209_write_reg(driver_motor_e motor, uint8_t reg, uint32_t value);
tmc2209_status_e tmc2209_read_reg(driver_motor_e motor, uint8_t reg, uint32_t *value);

tmc2209_status_e tmc2209_set_hold_current(driver_motor_e motor, float current_a);
tmc2209_status_e tmc2209_set_run_current(driver_motor_e motor, float current_a);
tmc2209_status_e tmc2209_set_currents(driver_motor_e motor, float run_current_a, float hold_current_a);
tmc2209_status_e tmc2209_set_microsteps(driver_motor_e motor, uint16_t microsteps);
tmc2209_status_e tmc2209_set_microsteps_all(uint16_t microsteps);

uint16_t tmc2209_get_microsteps(driver_motor_e motor);
bool tmc2209_is_initialized(void);

tmc2209_status_e tmc2209_use_motion_microsteps(void);
tmc2209_status_e tmc2209_use_correction_microsteps(void);

const char *tmc2209_status_str(tmc2209_status_e status);

/* Plug-and-play aliases matching the rest of the robot code vocabulary. */
#define Motor driver_motor_e
#define Motor_X DRIVER_MOTOR_X
#define Motor_Y DRIVER_MOTOR_Y
#define Motor_Z DRIVER_MOTOR_Z
#define Motor_PHI DRIVER_MOTOR_PHI

#define driver_init tmc2209_init
#define driver_configure_defaults tmc2209_configure_defaults
#define driver_set_holdcurr tmc2209_set_hold_current
#define driver_set_runcurr tmc2209_set_run_current
#define driver_set_currents tmc2209_set_currents
#define driver_set_microsteps tmc2209_set_microsteps
#define driver_set_microsteps_all tmc2209_set_microsteps_all
#define driver_get_microsteps tmc2209_get_microsteps
#define driver_use_motion_microsteps tmc2209_use_motion_microsteps
#define driver_use_correction_microsteps tmc2209_use_correction_microsteps

#endif /* TMC2209_TMC2209_H_ */
