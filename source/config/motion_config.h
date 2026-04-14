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
 * AXIS MOTION PROFILES (MOVE Commands)
 * ========================================================================== */


// X axis
#define X_MAX_STEP_RATE_SPS             12000u
#define X_START_STEP_RATE_SPS             400u
#define X_ACCEL_SPS2                     20000u

// Y axis
#define Y_MAX_STEP_RATE_SPS             12000u
#define Y_START_STEP_RATE_SPS             400u
#define Y_ACCEL_SPS2                     20000u

// Z axis
#define Z_MAX_STEP_RATE_SPS              2500u
#define Z_START_STEP_RATE_SPS             400u
#define Z_ACCEL_SPS2                     10000u

// PHI axis
#define PHI_MAX_STEP_RATE_SPS            1800u
#define PHI_START_STEP_RATE_SPS           300u
#define PHI_ACCEL_SPS2                    7000u


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
