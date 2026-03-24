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
 * HOMING
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
#define HOME_ENABLE_Y                   0
#define HOME_ENABLE_Z                   0
#endif


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


#endif /* CONFIG_HOME_CONFIG_H_ */
