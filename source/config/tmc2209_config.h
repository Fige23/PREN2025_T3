/*Project: PREN_Puzzleroboter
 *
 * TMC2209 UART driver configuration.
 */

#ifndef CONFIG_TMC2209_CONFIG_H_
#define CONFIG_TMC2209_CONFIG_H_

/* ============================================================================
 * UART BUS
 * ========================================================================== */

#define TMC2209_ENABLE                  1
#define TMC2209_UART_BAUDRATE           115200u
#define TMC2209_UART_TIMEOUT_LOOPS      200000u
#define TMC2209_UART_BUS_NAME           "UART0"
#define TMC2209_UART_TX_PIN_NAME        "PTA2"
#define TMC2209_UART_RX_PIN_NAME        "PTA1"

/*
 * Board Config Tools currently configure UART0 as:
 * PTA1 = UART0_RX, PTA2 = UART0_TX.
 * TMC2209 single-wire UART usually connects TX to PDN_UART through a series
 * resistor and RX directly to the same bus node.
 */
#define TMC2209_UART_SINGLE_WIRE        1

/* ============================================================================
 * DRIVER ADDRESSES
 * ========================================================================== */

#define TMC2209_ADDR_X                  0u
#define TMC2209_ADDR_Y                  1u
#define TMC2209_ADDR_Z                  2u
#define TMC2209_ADDR_PHI                3u

/* ============================================================================
 * CURRENT SETTINGS
 * ========================================================================== */

/*
 * Current configuration is written in Ampere RMS motor phase current.
 *
 * Example:
 *   TMC2209_HOLDCURR_X_A = 0.10f  -> about 0.10 A while standing
 *   TMC2209_RUNCURR_X_A  = 0.42f  -> about 0.42 A while moving
 *
 * tmc2209.c converts these values to the TMC2209 current-scale register using
 * RSENSE and VSENSE. No manual register conversion is needed here.
 *
 * Set this to your board's actual sense resistor. Common modules are 0.110 Ohm.
 */
#define TMC2209_RSENSE_OHM              0.110f

/* 1 => lower current-scale voltage, better resolution for moderate currents. */
#define TMC2209_GLOBAL_SCALER           0u
#define TMC2209_VSENSE_LOW_CURRENT      1

/* Hold current in A RMS: current used after the motor has stopped. */
#define TMC2209_HOLDCURR_X_A            0.35f
#define TMC2209_HOLDCURR_Y_A            0.35f
#define TMC2209_HOLDCURR_Z_A            0.20f
#define TMC2209_HOLDCURR_PHI_A          0.10f

/* Run current in A RMS: current used while the motor is moving. */
#define TMC2209_RUNCURR_X_A             1.20f
#define TMC2209_RUNCURR_Y_A             1.20f
#define TMC2209_RUNCURR_Z_A             0.40f
#define TMC2209_RUNCURR_PHI_A           0.42f

/*
 * 0..15. Delay before switching from run current to hold current.
 * Higher: motor keeps full run torque briefly after stopping, useful if an axis
 * settles or drops immediately after a move. Lower: less heat after motion.
 */
#define TMC2209_IHOLDDELAY              8u

/* ============================================================================
 * MICROSTEPPING
 * ========================================================================== */

#define TMC2209_MICROSTEPS_MOVE         8u
#define TMC2209_MICROSTEPS_CORRECTION   64u
#define TMC2209_INTERPOLATION_ENABLE    1

/* Switch to 64 microsteps for closed-loop correction moves, then restore 8. */
#define TMC2209_CORRECTION_MICROSTEP_ENABLE 1

/* ============================================================================
 * DEFAULT BEHAVIOR
 * ========================================================================== */

/*
 * 1 => quiet, smooth motor behavior at low speed. If an axis is silent but loses
 * steps under load, test with StealthChop off / SpreadCycle on.
 */
#define TMC2209_DEFAULT_STEALTHCHOP     1

/*
 * 1 => stronger classic current chopper, more audible but usually better torque
 * robustness. Normally keep this 0 when StealthChop is enabled.
 */
#define TMC2209_DEFAULT_SPREADCYCLE     0

/*
 * Lets the driver adapt StealthChop PWM amplitude/gradient automatically.
 * Usually keep both enabled. If you hear speed-dependent whining or unstable
 * quiet-mode behavior, these are good A/B-test switches.
 */
#define TMC2209_DEFAULT_PWM_AUTOSCALE   1
#define TMC2209_DEFAULT_PWM_AUTOGRAD    1

/*
 * Smooths small STEP timing irregularities before the driver acts on them.
 * Keep enabled for normal robot motion; disable only while debugging exact high
 * speed STEP timing.
 */
#define TMC2209_DEFAULT_MULTISTEP_FILT  1

/*
 * 0..255: time to power down after standstill. 0 disables automatic powerdown.
 * Increase if the axis loses holding force too soon after a move. Decrease if
 * idle motors stay unnecessarily warm.
 */
#define TMC2209_DEFAULT_TPOWERDOWN      20u

/*
 * Upper velocity threshold for StealthChop. 0 keeps StealthChop active over the
 * full speed range when enabled. Increase if the motor should switch earlier to
 * the louder but more forceful SpreadCycle behavior at speed.
 */
#define TMC2209_DEFAULT_TPWMTHRS        0u

/*
 * 0..255. StallGuard sensitivity threshold; tune per mechanics if sensorless
 * load/stall detection is used. Not needed for ordinary step/dir operation.
 */
#define TMC2209_DEFAULT_SGTHRS          0u

/*
 * Standstill mode: 0 normal hold, 1 freewheel, 2 coil short LS, 3 coil short HS.
 * Use normal hold for axes that must keep position. Freewheel keeps the motor
 * cool and movable by hand, but the position can drift.
 */
#define TMC2209_DEFAULT_FREEWHEEL       0u

#endif /* CONFIG_TMC2209_CONFIG_H_ */
