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

typedef enum {
    TMC2209_STANDSTILL_NORMAL = 0,
    TMC2209_STANDSTILL_FREEWHEEL = 1,
    TMC2209_STANDSTILL_COIL_SHORT_LS = 2,
    TMC2209_STANDSTILL_COIL_SHORT_HS = 3
} tmc2209_standstill_mode_e;

void tmc2209_init(void);
tmc2209_status_e tmc2209_configure_defaults(void);

tmc2209_status_e tmc2209_write_reg(driver_motor_e motor, uint8_t reg, uint32_t value);
tmc2209_status_e tmc2209_read_reg(driver_motor_e motor, uint8_t reg, uint32_t *value);

tmc2209_status_e tmc2209_set_hold_current(driver_motor_e motor, float current_a);
tmc2209_status_e tmc2209_set_run_current(driver_motor_e motor, float current_a);
tmc2209_status_e tmc2209_set_currents(driver_motor_e motor, float run_current_a, float hold_current_a);
tmc2209_status_e tmc2209_set_microsteps(driver_motor_e motor, uint16_t microsteps);
tmc2209_status_e tmc2209_set_microsteps_all(uint16_t microsteps);

/*
 * Selects the quiet voltage-mode chopper (StealthChop) when enable=true.
 * Practical effect: the motor sounds much smoother and quieter, especially at
 * low speed and standstill. The tradeoff is less immediate torque feedback than
 * SpreadCycle, so if an axis loses steps under load or during hard acceleration,
 * try disabling StealthChop for that axis.
 */
tmc2209_status_e tmc2209_set_stealthchop(driver_motor_e motor, bool enable);
tmc2209_status_e tmc2209_set_stealthchop_all(bool enable);

/*
 * Enables the driver's pulse filter for STEP input timing.
 * Practical effect: tiny jitter or uneven spacing on STEP pulses is smoothed a
 * bit before the driver acts on it. Keep this enabled for normal motion. If you
 * ever debug very high step rates or exact edge timing, this is one of the first
 * filters to temporarily disable.
 */
tmc2209_status_e tmc2209_set_multistep_filter(driver_motor_e motor, bool enable);

/*
 * Sets how long the driver waits after the motor becomes idle before applying
 * power-down behavior. A larger value keeps full/hold current active longer
 * after a move. Increase it if the axis relaxes too quickly after stopping;
 * decrease it if motors stay warm while idle.
 */
tmc2209_status_e tmc2209_set_powerdown_delay(driver_motor_e motor, uint8_t delay);

/*
 * Sets the speed threshold where StealthChop may hand over to SpreadCycle.
 * Practical effect: low values keep quiet StealthChop active over more of the
 * speed range; higher values make the driver switch earlier to the stronger,
 * more audible SpreadCycle behavior. If the motor is quiet but weak at speed,
 * this threshold is worth tuning.
 */
tmc2209_status_e tmc2209_set_tpwmthrs(driver_motor_e motor, uint32_t threshold);

/*
 * Lets the driver automatically adapt the PWM amplitude in StealthChop.
 * Practical effect: normally smoother and more reliable quiet running across
 * different speeds and loads. If StealthChop behaves unstable or makes odd
 * speed-dependent noise, compare with autoscale disabled.
 */
tmc2209_status_e tmc2209_set_pwm_autoscale(driver_motor_e motor, bool enable);

/*
 * Lets the driver automatically adapt the PWM gradient in StealthChop.
 * Practical effect: improves StealthChop tuning over varying load/speed. Keep
 * it enabled for normal use; disable only when you want fully manual PWM
 * behavior while chasing a specific noise or vibration issue.
 */
tmc2209_status_e tmc2209_set_pwm_autograd(driver_motor_e motor, bool enable);

/*
 * Changes what the driver does when the motor is commanded to stand still.
 * NORMAL holds position with configured hold current. FREEWHEEL releases the
 * motor so it can move by hand and stays cooler, but position can drift. The
 * coil-short modes brake the motor electrically. If an idle axis gets hot, try
 * lower hold current first, then consider FREEWHEEL only where drift is safe.
 */
tmc2209_status_e tmc2209_set_standstill_mode(driver_motor_e motor, tmc2209_standstill_mode_e mode);

/*
 * Sets the StallGuard sensitivity threshold.
 * Practical effect: changes when the driver considers mechanical load high.
 * This is useful for sensorless stall/load detection, not for normal stepping.
 * Tune while reading SG_RESULT: if stalls are missed, adjust the threshold; if
 * false stalls happen during normal motion, move it the other way.
 */
tmc2209_status_e tmc2209_set_stallguard_threshold(driver_motor_e motor, uint8_t threshold);

/*
 * Inverts the motor direction inside the driver.
 * Practical effect: same as swapping the logical direction for this driver, but
 * done in UART config. Prefer the existing geometry INVERT_ROT_* defines for
 * robot kinematics; use this only if wiring/driver orientation should be fixed
 * at the driver layer.
 */
tmc2209_status_e tmc2209_set_direction_inverted(driver_motor_e motor, bool inverted);

/*
 * Reads the interface counter. It increments after successful UART writes.
 * Practical effect: quick wiring/address sanity check. If writes return OK but
 * IFCNT never changes, suspect UART wiring, address pins, or single-wire setup.
 */
tmc2209_status_e tmc2209_read_ifcnt(driver_motor_e motor, uint8_t *ifcnt);

/*
 * Reads and reports global driver status flags such as reset and charge-pump
 * undervoltage. Use this after startup or after suspicious driver behavior to
 * see whether the chip recently reset or detected a supply problem.
 */
tmc2209_status_e tmc2209_read_gstat(driver_motor_e motor, uint32_t *gstat);

/*
 * Reads detailed driver status. This is the main "what is the driver unhappy
 * about?" register: overtemperature warnings, short detection, open-load hints,
 * standstill state and StallGuard-related status live here.
 */
tmc2209_status_e tmc2209_read_drv_status(driver_motor_e motor, uint32_t *drv_status);

/*
 * Reads the live StallGuard load result. Practical effect: lower/higher values
 * indicate changing mechanical load depending on operating point. Use it while
 * moving at a steady speed to tune SGTHRS or to see whether an axis is binding.
 */
tmc2209_status_e tmc2209_read_sg_result(driver_motor_e motor, uint16_t *sg_result);

/*
 * Clears latched GSTAT flags. Read GSTAT first, clear it, then read again later:
 * if the same flag comes back, the problem is current rather than historical.
 */
tmc2209_status_e tmc2209_clear_gstat(driver_motor_e motor);

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
#define driver_set_stealthchop tmc2209_set_stealthchop
#define driver_set_stealthchop_all tmc2209_set_stealthchop_all
#define driver_set_multistep_filter tmc2209_set_multistep_filter
#define driver_set_powerdown_delay tmc2209_set_powerdown_delay
#define driver_set_tpwmthrs tmc2209_set_tpwmthrs
#define driver_set_pwm_autoscale tmc2209_set_pwm_autoscale
#define driver_set_pwm_autograd tmc2209_set_pwm_autograd
#define driver_set_standstill_mode tmc2209_set_standstill_mode
#define driver_set_stallguard_threshold tmc2209_set_stallguard_threshold
#define driver_set_direction_inverted tmc2209_set_direction_inverted
#define driver_read_ifcnt tmc2209_read_ifcnt
#define driver_read_gstat tmc2209_read_gstat
#define driver_read_drv_status tmc2209_read_drv_status
#define driver_read_sg_result tmc2209_read_sg_result
#define driver_clear_gstat tmc2209_clear_gstat
#define driver_get_microsteps tmc2209_get_microsteps
#define driver_use_motion_microsteps tmc2209_use_motion_microsteps
#define driver_use_correction_microsteps tmc2209_use_correction_microsteps

#endif /* TMC2209_TMC2209_H_ */
