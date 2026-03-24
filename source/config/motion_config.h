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
 * 5) MOTION ENGINE / PROFILE
 * ========================================================================== */

// Motion-Timer / Pulse Engine
#define STEP_TICK_HZ                    240000u
#define STEP_PULSE_WIDTH_TICKS          2u
#define STEP_MIN_PERIOD_TICKS           2u

// Bewegungsprofil
// kurze Wege -> Dreiecksprofil
// lange Wege -> durch VMAX gedeckelt
#define MOTION_PROFILE_ENABLE           1
#define MOTION_PROFILE_SYMMETRIC        1

// X axis
#define X_MAX_STEP_RATE_SPS             12000u
#define X_START_STEP_RATE_SPS             400u
#define X_ACCEL_SPS2                     30000u

// Y axis
#define Y_MAX_STEP_RATE_SPS             12000u
#define Y_START_STEP_RATE_SPS             400u
#define Y_ACCEL_SPS2                     30000u

// Z axis
#define Z_MAX_STEP_RATE_SPS              2500u
#define Z_START_STEP_RATE_SPS             400u
#define Z_ACCEL_SPS2                     10000u

// PHI axis
#define PHI_MAX_STEP_RATE_SPS            1800u
#define PHI_START_STEP_RATE_SPS           300u
#define PHI_ACCEL_SPS2                    7000u

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
 * 8) DERIVED / HELPER MACROS
 * ========================================================================== */

// Skaliertes Arbeitsfenster
#define LIM_X_MIN_S   ((int32_t)(LIMIT_X_MIN   * SCALE_MM))
#define LIM_X_MAX_S   ((int32_t)(LIMIT_X_MAX   * SCALE_MM))
#define LIM_Y_MIN_S   ((int32_t)(LIMIT_Y_MIN   * SCALE_MM))
#define LIM_Y_MAX_S   ((int32_t)(LIMIT_Y_MAX   * SCALE_MM))
#define LIM_Z_MIN_S   ((int32_t)(LIMIT_Z_MIN   * SCALE_MM))
#define LIM_Z_MAX_S   ((int32_t)(LIMIT_Z_MAX   * SCALE_MM))
#define LIM_P_MIN_S   ((int32_t)(LIMIT_PHI_MIN * SCALE_DEG))
#define LIM_P_MAX_S   ((int32_t)(LIMIT_PHI_MAX * SCALE_DEG))

// Abgeleitete Maximalgeschwindigkeiten in physikalischen Einheiten
#define VMAX_X_MM_S     ((float)X_MAX_STEP_RATE_SPS    / ((float)STEPS_PER_MM_X_Q1000    / 1000.0f))
#define VMAX_Y_MM_S     ((float)Y_MAX_STEP_RATE_SPS    / ((float)STEPS_PER_MM_Y_Q1000    / 1000.0f))
#define VMAX_Z_MM_S     ((float)Z_MAX_STEP_RATE_SPS    / ((float)STEPS_PER_MM_Z_Q1000    / 1000.0f))
#define VMAX_PHI_DEG_S  ((float)PHI_MAX_STEP_RATE_SPS  / ((float)STEPS_PER_DEG_PHI_Q1000 / 1000.0f))


/* ============================================================================
 * 9a) MOTION PROFILES
 * ========================================================================== */

// Helper-Makro für motion_profile_s Initializer
#define MOTION_PROFILE_INIT(start_sps, max_sps, accel_sps2) \
    { (start_sps), (max_sps), (accel_sps2) }

// X axis homing profiles
#define HOME_X_RELEASE_START_STEP_RATE_SPS    300u
#define HOME_X_RELEASE_MAX_STEP_RATE_SPS     1200u
#define HOME_X_RELEASE_ACCEL_SPS2            6000u

#define HOME_X_FAST_START_STEP_RATE_SPS       400u
#define HOME_X_FAST_MAX_STEP_RATE_SPS        4000u
#define HOME_X_FAST_ACCEL_SPS2              12000u

#define HOME_X_BACKOFF_START_STEP_RATE_SPS    250u
#define HOME_X_BACKOFF_MAX_STEP_RATE_SPS     1000u
#define HOME_X_BACKOFF_ACCEL_SPS2            5000u

#define HOME_X_SLOW_START_STEP_RATE_SPS       120u
#define HOME_X_SLOW_MAX_STEP_RATE_SPS         500u
#define HOME_X_SLOW_ACCEL_SPS2               1500u

// Y axis homing profiles
#define HOME_Y_RELEASE_START_STEP_RATE_SPS    300u
#define HOME_Y_RELEASE_MAX_STEP_RATE_SPS     1200u
#define HOME_Y_RELEASE_ACCEL_SPS2            6000u

#define HOME_Y_FAST_START_STEP_RATE_SPS       400u
#define HOME_Y_FAST_MAX_STEP_RATE_SPS        4000u
#define HOME_Y_FAST_ACCEL_SPS2              12000u

#define HOME_Y_BACKOFF_START_STEP_RATE_SPS    250u
#define HOME_Y_BACKOFF_MAX_STEP_RATE_SPS     1000u
#define HOME_Y_BACKOFF_ACCEL_SPS2            5000u

#define HOME_Y_SLOW_START_STEP_RATE_SPS       120u
#define HOME_Y_SLOW_MAX_STEP_RATE_SPS         500u
#define HOME_Y_SLOW_ACCEL_SPS2               1500u

// Z axis homing profiles
#define HOME_Z_RELEASE_START_STEP_RATE_SPS    200u
#define HOME_Z_RELEASE_MAX_STEP_RATE_SPS      700u
#define HOME_Z_RELEASE_ACCEL_SPS2            3000u

#define HOME_Z_FAST_START_STEP_RATE_SPS       250u
#define HOME_Z_FAST_MAX_STEP_RATE_SPS        1000u
#define HOME_Z_FAST_ACCEL_SPS2               4000u

#define HOME_Z_BACKOFF_START_STEP_RATE_SPS    180u
#define HOME_Z_BACKOFF_MAX_STEP_RATE_SPS      500u
#define HOME_Z_BACKOFF_ACCEL_SPS2            2000u

#define HOME_Z_SLOW_START_STEP_RATE_SPS       100u
#define HOME_Z_SLOW_MAX_STEP_RATE_SPS         250u
#define HOME_Z_SLOW_ACCEL_SPS2               1000u

// Struct-Initializer-Makros
#define HOME_X_RELEASE_PROFILE \
    MOTION_PROFILE_INIT(HOME_X_RELEASE_START_STEP_RATE_SPS, \
                        HOME_X_RELEASE_MAX_STEP_RATE_SPS, \
                        HOME_X_RELEASE_ACCEL_SPS2)

#define HOME_X_FAST_PROFILE \
    MOTION_PROFILE_INIT(HOME_X_FAST_START_STEP_RATE_SPS, \
                        HOME_X_FAST_MAX_STEP_RATE_SPS, \
                        HOME_X_FAST_ACCEL_SPS2)

#define HOME_X_BACKOFF_PROFILE \
    MOTION_PROFILE_INIT(HOME_X_BACKOFF_START_STEP_RATE_SPS, \
                        HOME_X_BACKOFF_MAX_STEP_RATE_SPS, \
                        HOME_X_BACKOFF_ACCEL_SPS2)

#define HOME_X_SLOW_PROFILE \
    MOTION_PROFILE_INIT(HOME_X_SLOW_START_STEP_RATE_SPS, \
                        HOME_X_SLOW_MAX_STEP_RATE_SPS, \
                        HOME_X_SLOW_ACCEL_SPS2)

#define HOME_Y_RELEASE_PROFILE \
    MOTION_PROFILE_INIT(HOME_Y_RELEASE_START_STEP_RATE_SPS, \
                        HOME_Y_RELEASE_MAX_STEP_RATE_SPS, \
                        HOME_Y_RELEASE_ACCEL_SPS2)

#define HOME_Y_FAST_PROFILE \
    MOTION_PROFILE_INIT(HOME_Y_FAST_START_STEP_RATE_SPS, \
                        HOME_Y_FAST_MAX_STEP_RATE_SPS, \
                        HOME_Y_FAST_ACCEL_SPS2)

#define HOME_Y_BACKOFF_PROFILE \
    MOTION_PROFILE_INIT(HOME_Y_BACKOFF_START_STEP_RATE_SPS, \
                        HOME_Y_BACKOFF_MAX_STEP_RATE_SPS, \
                        HOME_Y_BACKOFF_ACCEL_SPS2)

#define HOME_Y_SLOW_PROFILE \
    MOTION_PROFILE_INIT(HOME_Y_SLOW_START_STEP_RATE_SPS, \
                        HOME_Y_SLOW_MAX_STEP_RATE_SPS, \
                        HOME_Y_SLOW_ACCEL_SPS2)

#define HOME_Z_RELEASE_PROFILE \
    MOTION_PROFILE_INIT(HOME_Z_RELEASE_START_STEP_RATE_SPS, \
                        HOME_Z_RELEASE_MAX_STEP_RATE_SPS, \
                        HOME_Z_RELEASE_ACCEL_SPS2)

#define HOME_Z_FAST_PROFILE \
    MOTION_PROFILE_INIT(HOME_Z_FAST_START_STEP_RATE_SPS, \
                        HOME_Z_FAST_MAX_STEP_RATE_SPS, \
                        HOME_Z_FAST_ACCEL_SPS2)

#define HOME_Z_BACKOFF_PROFILE \
    MOTION_PROFILE_INIT(HOME_Z_BACKOFF_START_STEP_RATE_SPS, \
                        HOME_Z_BACKOFF_MAX_STEP_RATE_SPS, \
                        HOME_Z_BACKOFF_ACCEL_SPS2)

#define HOME_Z_SLOW_PROFILE \
    MOTION_PROFILE_INIT(HOME_Z_SLOW_START_STEP_RATE_SPS, \
                        HOME_Z_SLOW_MAX_STEP_RATE_SPS, \
                        HOME_Z_SLOW_ACCEL_SPS2)


#endif /* CONFIG_MOTION_CONFIG_H_ */
