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

/* ============================================================================
 * Z POSITIONS FOR PLACE OPERATION
 * ========================================================================== */

// Z position during XY+phi movement (safe height to avoid collisions)
#define PLACE_Z_SAFE_POS_MM_SCALED        (SCALE_MM*100)

// Z position when placing object (down at the destination)
#define PLACE_Z_PLACE_POS_MM_SCALED       (SCALE_MM*0)


/* ============================================================================
 * PLACE OPERATION TIMINGS (in milliseconds)
 * ========================================================================== */

// Time to wait after magnet deactivates for release
#define PLACE_MAGNET_RELEASE_WAIT_MS      200u

// Time to wait for Z motor to move from safe to place position
#define PLACE_Z_DOWN_TIMEOUT_MS           5000u

// Time to wait for XY+phi motors to reach target
#define PLACE_XY_MOVE_TIMEOUT_MS          10000u

// Time to wait for Z motor to move from place back to safe
#define PLACE_Z_UP_TIMEOUT_MS             5000u


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
