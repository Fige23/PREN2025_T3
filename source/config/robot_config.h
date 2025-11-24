/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
robot_config.h	Created on: 24.11.2025	   Author: Fige23	Team 3                                                                
*/

#ifndef CONFIG_ROBOT_CONFIG_H_
#define CONFIG_ROBOT_CONFIG_H_

// === Steps/mm oder Steps/µm (je Achse) ===
//
// steps_per_um = steps_per_mm / 1000.
#define STEPS_PER_MM_X     80.0f
#define STEPS_PER_MM_Y     80.0f
#define STEPS_PER_MM_Z     400.0f
#define STEPS_PER_DEG_PHI  10.0f

// Pulse-Tick (ISR-Rate)
#define STEP_TICK_HZ       12000    // Startwert, später tunen

// Max-Geschwindigkeiten in mm/s bzw deg/s (für Planner)
#define VMAX_X_MM_S        50.0f
#define VMAX_Y_MM_S        50.0f
#define VMAX_Z_MM_S        20.0f
#define VMAX_PHI_DEG_S     90.0f

#endif





#endif /* CONFIG_ROBOT_CONFIG_H_ */
