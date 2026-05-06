/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_))
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|
pick_config.h	Created on: 01.04.2026	   Author: Fige23	Team 3
*/

#ifndef CONFIG_PICK_CONFIG_H_
#define CONFIG_PICK_CONFIG_H_
#include "build_config.h"
#include "geometry_config.h"
#include "motion_config.h"

/* ============================================================================
 * ISR TIMING (based on actual motion timer frequency)
 * ========================================================================== */

// Convert milliseconds to ISR ticks (based on STEP_TICK_HZ from motion_config.h)
#define ISR_TICKS_FROM_MS(ms) \
    (((uint32_t)(ms) * (STEP_TICK_HZ) + 500u) / 1000u)

/* ============================================================================
 * Z POSITIONS FOR PICK OPERATION
 * ========================================================================== */

/* Z position during XY movement (safe height to avoid collisions) */
#define PICK_Z_SAFE_POS_MM_SCALED        (SCALE_MM*20)

/* Z position when actually gripping the object. Larger Z is further down. */
#define PICK_Z_GRIP_POS_MM_SCALED        (SCALE_MM*40)

/* Optional compatibility alias, falls irgendwo noch der alte Name verwendet wird */
#define PICK_Z_PICK_POS_MM_SCALED        PICK_Z_GRIP_POS_MM_SCALED

/* ============================================================================
 * PICK OPERATION TIMINGS (in milliseconds)
 * ========================================================================== */

// Time to wait after magnet activates for it to grab the piece
#define PICK_MAGNET_WAIT_MS              200u


/* ============================================================================
 * WAIT TICKS (derived from timings using ISR frequency)
 * ========================================================================== */

// Wait ticks for various pick phases (based on ISR ticks, not loop cycles!)
#define PICK_WAIT_TICKS_XY_SETTLE       ISR_TICKS_FROM_MS(50u)      // 50ms settle time
#define PICK_WAIT_TICKS_Z_SETTLE        ISR_TICKS_FROM_MS(50u)      // 50ms settle time
#define PICK_WAIT_TICKS_MAGNET_GRAB     ISR_TICKS_FROM_MS(PICK_MAGNET_WAIT_MS)


/* ============================================================================
 * MOTION PROFILES FOR PICK OPERATION
 * ========================================================================== */

// Helper macro for motion_profile_s initializer
#define MOTION_PROFILE_INIT(start_sps, max_sps, accel_sps2) \
    { (start_sps), (max_sps), (accel_sps2) }

// Z axis motion profiles for pick operation
#define PICK_Z_DOWN_START_STEP_RATE_SPS   150u
#define PICK_Z_DOWN_MAX_STEP_RATE_SPS     800u
#define PICK_Z_DOWN_ACCEL_SPS2            2000u

#define PICK_Z_UP_START_STEP_RATE_SPS     150u
#define PICK_Z_UP_MAX_STEP_RATE_SPS       800u
#define PICK_Z_UP_ACCEL_SPS2              2000u

// Profile initializers
#define PICK_Z_DOWN_PROFILE \
    MOTION_PROFILE_INIT(PICK_Z_DOWN_START_STEP_RATE_SPS, \
                        PICK_Z_DOWN_MAX_STEP_RATE_SPS, \
                        PICK_Z_DOWN_ACCEL_SPS2)

#define PICK_Z_UP_PROFILE \
    MOTION_PROFILE_INIT(PICK_Z_UP_START_STEP_RATE_SPS, \
                        PICK_Z_UP_MAX_STEP_RATE_SPS, \
                        PICK_Z_UP_ACCEL_SPS2)


#endif /* CONFIG_PICK_CONFIG_H_ */
