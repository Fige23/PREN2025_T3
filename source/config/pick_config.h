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

/* ============================================================================
 * SYSTEM TIMING CONSTANT
 * ========================================================================== */
// Assumed cycle interval for state machine waits (in milliseconds)
#define SYSTEM_STEP_INTERVAL_MS         10u

/* ============================================================================
 * Z POSITIONS FOR PICK OPERATION
 * ========================================================================== */

// Z position during XY movement (safe height to avoid collisions)
#define PICK_Z_SAFE_POS_MM_SCALED        (SCALE_MM*100)

// Z position when picking up object (down at the piece)
#define PICK_Z_PICK_POS_MM_SCALED        (SCALE_MM*0)

// Optional: Z position during XY movement for "show off" effect
// (higher than safe, lowers just before pick starts)
// Set to same as SAFE if not needed
#define PICK_Z_TOP_POS_MM_SCALED         (SCALE_MM*100)


/* ============================================================================
 * PICK OPERATION TIMINGS (in milliseconds)
 * ========================================================================== */

// Time to wait after magnet activates for it to grab the piece
#define PICK_MAGNET_WAIT_MS              200u

// Time to wait for Z motor to move from safe to pick position
#define PICK_Z_DOWN_TIMEOUT_MS           5000u

// Time to wait for XY motors to reach target
#define PICK_XY_MOVE_TIMEOUT_MS          10000u

// Time to wait for Z motor to move from pick back to safe
#define PICK_Z_UP_TIMEOUT_MS             5000u


/* ============================================================================
 * WAIT CYCLES (derived from timings above)
 * ========================================================================== */

// Helper: Convert milliseconds to step cycles
#define CYCLES_FROM_MS(ms) \
    ((ms) / SYSTEM_STEP_INTERVAL_MS)

// Wait cycles for various pick phases (assuming SYSTEM_STEP_INTERVAL_MS per call)
#define PICK_WAIT_CYCLES_XY_SETTLE       CYCLES_FROM_MS(50u)     // 50ms settle time
#define PICK_WAIT_CYCLES_Z_SETTLE        CYCLES_FROM_MS(50u)     // 50ms settle time
#define PICK_WAIT_CYCLES_MAGNET_GRAB     CYCLES_FROM_MS(PICK_MAGNET_WAIT_MS)


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
