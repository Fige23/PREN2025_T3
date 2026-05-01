/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_))
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|
place_config.h	Created on: 01.04.2026	   Author: Fige23	Team 3
*/

#ifndef CONFIG_PLACE_CONFIG_H_
#define CONFIG_PLACE_CONFIG_H_
#include "geometry_config.h"
#include "motion_config.h"

/* ============================================================================
 * ISR TIMING (based on actual motion timer frequency)
 * ========================================================================== */

// Convert milliseconds to ISR ticks (based on STEP_TICK_HZ from motion_config.h)
#define ISR_TICKS_FROM_MS(ms) \
    (((uint32_t)(ms) * (STEP_TICK_HZ) + 500u) / 1000u)

/* ============================================================================
 * Z POSITIONS FOR PLACE OPERATION
 * ========================================================================== */

/* Z position during XY+phi movement (safe height to avoid collisions) */
#define PLACE_Z_SAFE_POS_MM_SCALED        (SCALE_MM*50)

/* Z position when actually dropping / placing the object. Larger Z is further down. */
#define PLACE_Z_DROP_POS_MM_SCALED        (SCALE_MM*90)

/* Compatibility alias, falls irgendwo noch der alte Name verwendet wird */
#define PLACE_Z_PLACE_POS_MM_SCALED       PLACE_Z_DROP_POS_MM_SCALED


/* ============================================================================
 * PLACE OPERATION TIMINGS (in milliseconds)
 * ========================================================================== */

// Time to wait after magnet deactivates for release
#define PLACE_MAGNET_RELEASE_WAIT_MS      200u


/* ============================================================================
 * WAIT TICKS (derived from timings using ISR frequency)
 * ========================================================================== */

// Wait ticks for various place phases (based on ISR ticks, not loop cycles!)
#define PLACE_WAIT_TICKS_XY_PHI_SETTLE    ISR_TICKS_FROM_MS(50u)      // 50ms settle time
#define PLACE_WAIT_TICKS_Z_SETTLE         ISR_TICKS_FROM_MS(50u)      // 50ms settle time
#define PLACE_WAIT_TICKS_MAGNET_RELEASE   ISR_TICKS_FROM_MS(PLACE_MAGNET_RELEASE_WAIT_MS)


/* ============================================================================
 * MOTION PROFILES FOR PLACE OPERATION
 * ========================================================================== */

// Helper macro for motion_profile_s initializer
#define MOTION_PROFILE_INIT(start_sps, max_sps, accel_sps2) \
    { (start_sps), (max_sps), (accel_sps2) }

// Z axis motion profiles for place operation
#define PLACE_Z_DOWN_START_STEP_RATE_SPS   150u
#define PLACE_Z_DOWN_MAX_STEP_RATE_SPS     800u
#define PLACE_Z_DOWN_ACCEL_SPS2            2000u

#define PLACE_Z_UP_START_STEP_RATE_SPS     150u
#define PLACE_Z_UP_MAX_STEP_RATE_SPS       800u
#define PLACE_Z_UP_ACCEL_SPS2              2000u

// Profile initializers
#define PLACE_Z_DOWN_PROFILE \
    MOTION_PROFILE_INIT(PLACE_Z_DOWN_START_STEP_RATE_SPS, \
                        PLACE_Z_DOWN_MAX_STEP_RATE_SPS, \
                        PLACE_Z_DOWN_ACCEL_SPS2)

#define PLACE_Z_UP_PROFILE \
    MOTION_PROFILE_INIT(PLACE_Z_UP_START_STEP_RATE_SPS, \
                        PLACE_Z_UP_MAX_STEP_RATE_SPS, \
                        PLACE_Z_UP_ACCEL_SPS2)


#endif /* CONFIG_PLACE_CONFIG_H_ */
