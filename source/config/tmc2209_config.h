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
#define TMC2209_UART_RX_BUF_SIZE        64u
#define TMC2209_UART_TIMEOUT_LOOPS      200000u

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

/* Set to your board's actual sense resistor. Common modules are 0.110 Ohm. */
#define TMC2209_RSENSE_OHM              0.110f

/* 1 => lower current-scale voltage, better resolution for moderate currents. */
#define TMC2209_GLOBAL_SCALER           0u
#define TMC2209_VSENSE_LOW_CURRENT      1

#define TMC2209_HOLDCURR_X_A            0.35f
#define TMC2209_HOLDCURR_Y_A            0.35f
#define TMC2209_HOLDCURR_Z_A            0.25f
#define TMC2209_HOLDCURR_PHI_A          0.25f

#define TMC2209_RUNCURR_X_A             0.80f
#define TMC2209_RUNCURR_Y_A             0.80f
#define TMC2209_RUNCURR_Z_A             0.60f
#define TMC2209_RUNCURR_PHI_A           0.60f

/* 0..15. Delay before switching from run current to hold current. */
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

#define TMC2209_DEFAULT_STEALTHCHOP     1
#define TMC2209_DEFAULT_SPREADCYCLE     0
#define TMC2209_DEFAULT_PWM_AUTOSCALE   1

#endif /* CONFIG_TMC2209_CONFIG_H_ */
