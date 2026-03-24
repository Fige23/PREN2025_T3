/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
calibration_config.h	Created on: 24.03.2026	   Author: Fige23	Team 3                                                                
*/

#ifndef CONFIG_CALIBRATION_CONFIG_H_
#define CONFIG_CALIBRATION_CONFIG_H_

/* ============================================================================
 * CALIBRATION
 * ========================================================================== */

// Kalibrierachsen
#define CAL_AXIS_X                      0
#define CAL_AXIS_Y                      1
#define CAL_AXIS_Z                      2

// Such-/Fahrparameter
#define CAL_STEP_WAIT_TIME_US           500U
#define CAL_MAX_SEARCH_STEPS            200000ULL
#define CAL_MAX_TRAVEL_STEPS            400000ULL

// Entprellung / Re-Trigger-Schutz
#define CAL_SWITCH_DEBOUNCE_US          3000U
#define CAL_SWITCH_LOCKOUT_STEPS        20U

// Gemessene Achslängen zwischen den gültigen Triggerpunkten
#define CAL_AXIS_X_LENGTH_MM            523.7f
#define CAL_AXIS_Y_LENGTH_MM            0.0f
#define CAL_AXIS_Z_LENGTH_MM            0.0f


#endif /* CONFIG_CALIBRATION_CONFIG_H_ */
