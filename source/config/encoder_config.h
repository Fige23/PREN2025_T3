/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_))
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|
encoder_config.h   Created on: 24.03.2026      Author: Fige23 Team 3
*/

#ifndef CONFIG_ENCODER_CONFIG_H_
#define CONFIG_ENCODER_CONFIG_H_

/* ============================================================================
 * POSITION FEEDBACK (AS5311)
 * ========================================================================== */

/* AS5311 quadrature x4:
 * 1024 counts per 2.0 mm  => 512 counts/mm
 */
#define ENC_X_COUNTS_PER_MM                  512u
#define ENC_Y_COUNTS_PER_MM                  512u

/* Invert encoder sign if needed */
#define ENC_X_INVERT                         0
#define ENC_Y_INVERT                         0

/* ============================================================================
 * CLOSED-LOOP TARGET TOLERANCE
 * unit: scaled mm (SCALE_MM = 1000 => 1 == 0.001 mm)
 * ========================================================================== */

#define POS_TOL_X_SCALED                     20      /* 0.020 mm */
#define POS_TOL_Y_SCALED                     20      /* 0.020 mm */

/* ============================================================================
 * CORRECTION CONTROLLER
 * ========================================================================== */

/* P gain = numerator / denominator */
#define POS_CORR_P_ZAEHLER                   1
#define POS_CORR_P_NENNER                    1

/* Max correction command per correction segment */
#define POS_CORR_MAX_STEP_SCALED            2000     /* 2.000 mm */

/* Hard stop after this many correction segments */
#define POS_CORR_MAX_ITERATIONS              6u

/* ============================================================================
 * CORRECTION MOTION PROFILE
 * Use a slower profile than normal travel moves
 * ========================================================================== */

#define POS_CORR_START_STEP_RATE_SPS         100u
#define POS_CORR_MAX_STEP_RATE_SPS           400u
#define POS_CORR_ACCEL_SPS2                 1000u

/* ============================================================================
 * PROGRESS / STALL DETECTION
 * ========================================================================== */

/* Minimal error improvement that counts as real progress */
#define POS_CORR_PROGRESS_MIN_DELTA_SCALED   5       /* 0.005 mm */

/* Abort if this many correction cycles in a row show no measurable progress */
#define POS_CORR_NO_PROGRESS_LIMIT           2u

/* ============================================================================
 * INTERNAL VS MEASURED POSITION CHECK
 * These are NOT the fine target tolerances.
 * They are sanity checks for "something is seriously off".
 * ========================================================================== */

/* Warning threshold: log clearly, but still allow correction */
#define POS_INT_MEAS_WARN_X_SCALED          1000     /* 1.000 mm */
#define POS_INT_MEAS_WARN_Y_SCALED          1000     /* 1.000 mm */

/* Abort threshold: too far apart -> likely lost steps, wrong scale, wrong sign,
 * slipping mechanics, or another major problem.
 */
#define POS_INT_MEAS_ABORT_X_SCALED         5000     /* 5.000 mm */
#define POS_INT_MEAS_ABORT_Y_SCALED         5000     /* 5.000 mm */

#endif /* CONFIG_ENCODER_CONFIG_H_ */