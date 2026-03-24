/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
encoder_config.h	Created on: 24.03.2026	   Author: Fige23	Team 3                                                                
*/

#ifndef CONFIG_ENCODER_CONFIG_H_
#define CONFIG_ENCODER_CONFIG_H_

/* ============================================================================
 * POSITION FEEDBACK (AS5311)
 * ========================================================================== */

// AS5311 (Quadratur, x4): 1024 edges pro 2.0 mm => 512 counts/mm
#define ENC_X_COUNTS_PER_MM              512u
#define ENC_Y_COUNTS_PER_MM              512u

// Falls Encoder-Richtung nicht mit +mm übereinstimmt
#define ENC_X_INVERT                     0
#define ENC_Y_INVERT                     0

// Toleranzen (Fixed-point mm * 1000)
#define POS_TOL_X_SCALED                 20      // 0.020 mm
#define POS_TOL_Y_SCALED                 20      // 0.020 mm

// Korrektur-Regelung
#define POS_CORR_P_ZAEHLER               1       // gain = NUM / DEN
#define POS_CORR_P_NENNER                1
#define POS_CORR_MAX_STEP_SCALED         2000    // max 2.0 mm pro Korrektur
#define POS_CORR_MAX_ITERATIONS          6


#endif /* CONFIG_ENCODER_CONFIG_H_ */
