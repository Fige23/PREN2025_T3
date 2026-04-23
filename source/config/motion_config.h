/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_))
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|
motion_config.h	Created on: 24.03.2026	   Author: Fige23	Team 3
*/

#ifndef CONFIG_MOTION_CONFIG_H_
#define CONFIG_MOTION_CONFIG_H_

/* ============================================================================
 * MOTION ENGINE / STEPPER CONTROL
 * ========================================================================== */

// Motion-Timer / Pulse Engine
#define STEP_TICK_HZ                    50000u
#define STEP_PULSE_WIDTH_TICKS          2u
#define STEP_MIN_PERIOD_TICKS           2u

// Bewegungsprofil
// kurze Wege -> Dreiecksprofil
// lange Wege -> durch VMAX gedeckelt
#define MOTION_PROFILE_ENABLE           1
#define MOTION_PROFILE_SYMMETRIC        1


/* ============================================================================
 * E-STOP POLLING BEHAVIOR
 * ========================================================================== */

// E-STOP polling mode:
// - EDGE: latch only on rising edge (original behavior)
// - LEVEL_LATCH: latch whenever pin is active (more robust against missed edges)
#define ESTOP_POLL_MODE_EDGE            0
#define ESTOP_POLL_MODE_LEVEL_LATCH     1

#define ESTOP_POLL_MODE                 ESTOP_POLL_MODE_LEVEL_LATCH

// Optional additional E-STOP polling inside motion tick ISR dispatch.
// Keep disabled by default to minimize ISR work.
#define ESTOP_POLL_IN_MOTION_ISR        1

// If ISR polling is enabled, sample only every N timer ticks to reduce ISR cost.
// 240000 Hz / 24 = 10000 polls/s, still very fast for a button.
#define ESTOP_POLL_ISR_DIVIDER          24u


/* ============================================================================
 * LIMIT SWITCH FILTERING
 * ========================================================================== */

// Limit switch inputs can pick up noise from nearby stepper wiring.
// Require the raw input to stay changed for multiple consecutive
// poll_limit_switch() calls before the stable state changes.
#define LIMIT_SWITCH_FILTER_ENABLE      1

// Consecutive polls required to accept a changed state.
// Keep pressed stricter against noise, but release faster so homing backoff
// recognizes that the switch has opened again.
// With STEP_TICK_HZ=50000 and ISR polling this is roughly:
// 150 -> 3 ms pressed confirm, 50 -> 1 ms release confirm.
#define LIMIT_SWITCH_PRESS_POLLS        150u
#define LIMIT_SWITCH_RELEASE_POLLS      50u


/* ============================================================================
 * AXIS MOTION PROFILES (MOVE Commands)
 * ========================================================================== */

// Configure desired real-world motion here.
// Values are intentionally kept round and easy to reason about.

// X axis
#define X_MAX_SPEED_MM_S                600.0f
#define X_START_SPEED_MM_S               20.0f
#define X_ACCEL_MM_S2                  1000.0f

// Y axis
#define Y_MAX_SPEED_MM_S                600.0f
#define Y_START_SPEED_MM_S               20.0f
#define Y_ACCEL_MM_S2                  1000.0f

// Z axis
#define Z_MAX_SPEED_MM_S                 12.5f
#define Z_START_SPEED_MM_S                2.0f
#define Z_ACCEL_MM_S2                    50.0f

// PHI axis
#define PHI_MAX_SPEED_DEG_S             800.0f
#define PHI_START_SPEED_DEG_S            25.0f
#define PHI_ACCEL_DEG_S2                400.0f


/* ============================================================================
 * DERIVED STEP-RATE VALUES
 * Normally do not edit below this line.
 * ========================================================================== */

#define MM_S_TO_SPS(mm_s, steps_per_mm_q1000) \
    ((uint32_t)(((mm_s) * (float)(steps_per_mm_q1000) / 1000.0f) + 0.5f))

#define DEG_S_TO_SPS(deg_s, steps_per_deg_q1000) \
    ((uint32_t)(((deg_s) * (float)(steps_per_deg_q1000) / 1000.0f) + 0.5f))

#define MM_S2_TO_SPS2(mm_s2, steps_per_mm_q1000) \
    ((uint32_t)(((mm_s2) * (float)(steps_per_mm_q1000) / 1000.0f) + 0.5f))

#define DEG_S2_TO_SPS2(deg_s2, steps_per_deg_q1000) \
    ((uint32_t)(((deg_s2) * (float)(steps_per_deg_q1000) / 1000.0f) + 0.5f))

#define X_MAX_STEP_RATE_SPS             MM_S_TO_SPS(X_MAX_SPEED_MM_S, STEPS_PER_MM_X_Q1000)
#define X_START_STEP_RATE_SPS           MM_S_TO_SPS(X_START_SPEED_MM_S, STEPS_PER_MM_X_Q1000)
#define X_ACCEL_SPS2                    MM_S2_TO_SPS2(X_ACCEL_MM_S2, STEPS_PER_MM_X_Q1000)

#define Y_MAX_STEP_RATE_SPS             MM_S_TO_SPS(Y_MAX_SPEED_MM_S, STEPS_PER_MM_Y_Q1000)
#define Y_START_STEP_RATE_SPS           MM_S_TO_SPS(Y_START_SPEED_MM_S, STEPS_PER_MM_Y_Q1000)
#define Y_ACCEL_SPS2                    MM_S2_TO_SPS2(Y_ACCEL_MM_S2, STEPS_PER_MM_Y_Q1000)

#define Z_MAX_STEP_RATE_SPS             MM_S_TO_SPS(Z_MAX_SPEED_MM_S, STEPS_PER_MM_Z_Q1000)
#define Z_START_STEP_RATE_SPS           MM_S_TO_SPS(Z_START_SPEED_MM_S, STEPS_PER_MM_Z_Q1000)
#define Z_ACCEL_SPS2                    MM_S2_TO_SPS2(Z_ACCEL_MM_S2, STEPS_PER_MM_Z_Q1000)

#define PHI_MAX_STEP_RATE_SPS           DEG_S_TO_SPS(PHI_MAX_SPEED_DEG_S, STEPS_PER_DEG_PHI_Q1000)
#define PHI_START_STEP_RATE_SPS         DEG_S_TO_SPS(PHI_START_SPEED_DEG_S, STEPS_PER_DEG_PHI_Q1000)
#define PHI_ACCEL_SPS2                  DEG_S2_TO_SPS2(PHI_ACCEL_DEG_S2, STEPS_PER_DEG_PHI_Q1000)


/* ============================================================================
 * PERIOD SMOOTHING (optional)
 * ========================================================================== */

// Optionales weiches Nachführen der Periodendauer
#define ENABLE_PERIOD_SMOOTHING          0

// Filterstärke:
// 1 => stark direkt
// 2 => 1/4 pro Update
// 3 => 1/8 pro Update
// 4 => 1/16 pro Update
#define PERIOD_SMOOTH_SHIFT              2

// minimale Tick-Änderung bei aktivem Smoothing
#define PERIOD_SMOOTH_MINSTEP            1


/* ============================================================================
 * DERIVED VALUES (physical units)
 * ========================================================================== */

// Abgeleitete Maximalgeschwindigkeiten in physikalischen Einheiten
#define VMAX_X_MM_S     ((float)X_MAX_STEP_RATE_SPS    / ((float)STEPS_PER_MM_X_Q1000    / 1000.0f))
#define VMAX_Y_MM_S     ((float)Y_MAX_STEP_RATE_SPS    / ((float)STEPS_PER_MM_Y_Q1000    / 1000.0f))
#define VMAX_Z_MM_S     ((float)Z_MAX_STEP_RATE_SPS    / ((float)STEPS_PER_MM_Z_Q1000    / 1000.0f))
#define VMAX_PHI_DEG_S  ((float)PHI_MAX_STEP_RATE_SPS  / ((float)STEPS_PER_DEG_PHI_Q1000 / 1000.0f))


#endif /* CONFIG_MOTION_CONFIG_H_ */
