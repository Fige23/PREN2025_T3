/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
geometry_config.h	Created on: 24.03.2026	   Author: Fige23	Team 3                                                                
*/

#ifndef CONFIG_GEOMETRY_CONFIG_H_
#define CONFIG_GEOMETRY_CONFIG_H_

/* ============================================================================
 * SCALING / GEOMETRY / AXIS CONVERSION
 * ========================================================================== */

// Fixed-point Auflösung
#define SCALE_MM                        1000   // 0.001 mm
#define SCALE_DEG                       100    // 0.01 °

// Achsumrechnung
// mechanikabhängig, kalibrieren
// Angaben in steps pro 1000 mm bzw. 1000 deg
#define STEPS_PER_MM_X_Q1000            42848
#define STEPS_PER_MM_Y_Q1000            39848
#define STEPS_PER_MM_Z_Q1000           400000
#define STEPS_PER_DEG_PHI_Q1000         10000

// Drehrichtungs-Invertierung der Motoren
#define INVERT_ROT_X                    0
#define INVERT_ROT_Y                    0
#define INVERT_ROT_Z                    0
#define INVERT_ROT_PHI                  0


/* ============================================================================
 * WORKSPACE / SAFETY LIMITS
 * ========================================================================== */

// Physikalische Limits (unskaliert)
#define LIMIT_X_MIN                      0
#define LIMIT_X_MAX                      350
#define LIMIT_Y_MIN                      0
#define LIMIT_Y_MAX                      255
#define LIMIT_Z_MIN                      0
#define LIMIT_Z_MAX                      150
#define LIMIT_PHI_MIN                   -180
#define LIMIT_PHI_MAX                    180

// Safe-Z Höhe für PICK/PLACE Sequenzen
// MOVE ignoriert Safe-Z bewusst
#define SAFE_Z_MAX_DURING_XY             (50 * SCALE_MM)


/* ============================================================================
 * DERIVED / HELPER MACROS
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


#endif /* CONFIG_GEOMETRY_CONFIG_H_ */
