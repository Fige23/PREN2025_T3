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

// 1 = MOVE nur erlaubt nach HOME, 0 = MOVE auch ohne HOME erlauben
#define REQUIRE_HOME_FOR_MOVE  0



//Hier umschalten ob UART demonstriert/getestet oder die stepper angesteuert werden sollen.
//wenn UART_DEMO nicht definiert -> alles ausser MOVE liefert aktuell ERR_NOT_IMPL 19.12.2025

//#define UART_DEMO
#ifndef UART_DEMO
#define IMPLEMENTATION_STEPPER
#endif

// === Steps/mm oder Steps/µm (je Achse) ===
//
//aus mechanik ableiten:
#define STEPS_PER_MM_X 80
#define STEPS_PER_MM_Y 80
#define STEPS_PER_MM_Z 400
#define STEPS_PER_DEG_PHI 10

// Pulse-Tick (ISR-Rate)
#define STEP_TICK_HZ       12000    // Startwert, später tunen

// Max-Geschwindigkeiten in mm/s bzw deg/s (für Planner)
#define VMAX_X_MM_S        50.0f
#define VMAX_Y_MM_S        50.0f
#define VMAX_Z_MM_S        20.0f
#define VMAX_PHI_DEG_S     90.0f

// Auflösung (Fixed-Point)
#define SCALE_MM    1000   // 0.001 mm
#define SCALE_DEG    100   // 0.01 °

// Physikalische Limits (UNskaliert)
#define LIMIT_X_MIN      0
#define LIMIT_X_MAX    300
#define LIMIT_Y_MIN      0
#define LIMIT_Y_MAX    300
#define LIMIT_Z_MIN      0
#define LIMIT_Z_MAX    150
#define LIMIT_PHI_MIN -180
#define LIMIT_PHI_MAX  180

// Max. Z-Höhe (ab Top=0 nach unten positiv), bei der XY/PHI-Bewegungen erlaubt sind.
// Wenn z > SAFE_Z_MAX_DURING_XY: erst Z nach oben (zurück) bis zur Safe-Höhe, dann XY/PHI.
// Wenn Ziel-Z > SAFE-Z: XY/PHI bei Safe-Z, danach Z-only auf Ziel.
#define SAFE_Z_MAX_DURING_XY   (50 * SCALE_MM)   // 50 mm unterhalb Top sind "safe"

// Skaliert
#define LIM_X_MIN_S  ((int32_t)(LIMIT_X_MIN  * SCALE_MM))
#define LIM_X_MAX_S  ((int32_t)(LIMIT_X_MAX  * SCALE_MM))
#define LIM_Y_MIN_S  ((int32_t)(LIMIT_Y_MIN  * SCALE_MM))
#define LIM_Y_MAX_S  ((int32_t)(LIMIT_Y_MAX  * SCALE_MM))
#define LIM_Z_MIN_S  ((int32_t)(LIMIT_Z_MIN  * SCALE_MM))
#define LIM_Z_MAX_S  ((int32_t)(LIMIT_Z_MAX  * SCALE_MM))
#define LIM_P_MIN_S  ((int32_t)(LIMIT_PHI_MIN* SCALE_DEG))
#define LIM_P_MAX_S  ((int32_t)(LIMIT_PHI_MAX* SCALE_DEG))



#endif /* CONFIG_ROBOT_CONFIG_H_ */
