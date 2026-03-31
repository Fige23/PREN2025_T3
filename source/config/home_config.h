/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
home_config.h	Created on: 24.03.2026	   Author: Fige23	Team 3                                                                
*/

#ifndef CONFIG_HOME_CONFIG_H_
#define CONFIG_HOME_CONFIG_H_
#include "build_config.h"

/* ============================================================================
 * HOMING ENABLE/DISABLE
 * ========================================================================== */

#if RELEASE
#define REQUIRE_HOME_FOR_MOVE 			1
#define HOME_ENABLE_X					1
#define HOME_ENABLE_Y					1
#define HOME_ENABLE_Z					1
#endif

// 1 = MOVE nur erlaubt nach HOME
// 0 = MOVE auch ohne HOME erlauben
#if !RELEASE
#define REQUIRE_HOME_FOR_MOVE           0

#define HOME_ENABLE_X                   1
#define HOME_ENABLE_Y                   1
#define HOME_ENABLE_Z                   0
#endif


/* ============================================================================
 * HOMING DISTANCES
 * ========================================================================== */

// X-Achse
#define HOME_X_RELEASE_MM_SCALED         (SCALE_MM*5)
#define HOME_X_SEEK_MM_SCALED            (SCALE_MM*400)
#define HOME_X_BACKOFF_MM_SCALED         (SCALE_MM*2)
#define HOME_X_ZERO_OFFSET_MM_SCALED     (SCALE_MM*0)

// Y-Achse
#define HOME_Y_RELEASE_MM_SCALED         (SCALE_MM*5)
#define HOME_Y_SEEK_MM_SCALED            (SCALE_MM*400)
#define HOME_Y_BACKOFF_MM_SCALED         (SCALE_MM*2)
#define HOME_Y_ZERO_OFFSET_MM_SCALED     (SCALE_MM*0)

// Z-Achse
#define HOME_Z_RELEASE_MM_SCALED         (SCALE_MM*5)
#define HOME_Z_SEEK_MM_SCALED            (SCALE_MM*200)
#define HOME_Z_BACKOFF_MM_SCALED         (SCALE_MM*2)
#define HOME_Z_ZERO_OFFSET_MM_SCALED     (SCALE_MM*0)


/* ============================================================================
 * HOMING MOTION PROFILES
 * ========================================================================== */

// Helper-Makro für motion_profile_s Initializer
#define MOTION_PROFILE_INIT(start_sps, max_sps, accel_sps2) \
    { (start_sps), (max_sps), (accel_sps2) }

// X axis homing profiles
#define HOME_X_RELEASE_START_STEP_RATE_SPS    300u
#define HOME_X_RELEASE_MAX_STEP_RATE_SPS     1000u
#define HOME_X_RELEASE_ACCEL_SPS2            3000u

#define HOME_X_FAST_START_STEP_RATE_SPS       400u
#define HOME_X_FAST_MAX_STEP_RATE_SPS        1500u
#define HOME_X_FAST_ACCEL_SPS2               3000u

#define HOME_X_BACKOFF_START_STEP_RATE_SPS    250u
#define HOME_X_BACKOFF_MAX_STEP_RATE_SPS      800u
#define HOME_X_BACKOFF_ACCEL_SPS2            2000u

#define HOME_X_SLOW_START_STEP_RATE_SPS       120u
#define HOME_X_SLOW_MAX_STEP_RATE_SPS         300u
#define HOME_X_SLOW_ACCEL_SPS2               1000u

// Y axis homing profiles
#define HOME_Y_RELEASE_START_STEP_RATE_SPS    300u
#define HOME_Y_RELEASE_MAX_STEP_RATE_SPS     1000u
#define HOME_Y_RELEASE_ACCEL_SPS2            3000u

#define HOME_Y_FAST_START_STEP_RATE_SPS       400u
#define HOME_Y_FAST_MAX_STEP_RATE_SPS        1500u
#define HOME_Y_FAST_ACCEL_SPS2               3000u

#define HOME_Y_BACKOFF_START_STEP_RATE_SPS    250u
#define HOME_Y_BACKOFF_MAX_STEP_RATE_SPS      800u
#define HOME_Y_BACKOFF_ACCEL_SPS2            3000u

#define HOME_Y_SLOW_START_STEP_RATE_SPS       120u
#define HOME_Y_SLOW_MAX_STEP_RATE_SPS         300u
#define HOME_Y_SLOW_ACCEL_SPS2               1000u

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


/* ============================================================================
 * HOMING PROFILE INITIALIZER MACROS
 * ========================================================================== */

// X axis
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

// Y axis
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

// Z axis
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


#endif /* CONFIG_HOME_CONFIG_H_ */
