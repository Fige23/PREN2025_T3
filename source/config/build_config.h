/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
build_config.h	Created on: 24.03.2026	   Author: Fige23	Team 3                                                                
*/

#ifndef CONFIG_BUILD_CONFIG_H_
#define CONFIG_BUILD_CONFIG_H_

/* ============================================================================
 * BUILD / FEATURE MODES
 * ========================================================================== */
#define RELEASE							0	//automatically disables all debug functionality
// Build mode selection
// Genau EIN Mode soll 1 sein.
#if !RELEASE
#define DEBUG_MODE						1
#define IMPLEMENTATION_STEPPER          1	//
#define UART_DEMO                       0	//UART Responds to all Commands, Hardware does nothing

#if (IMPLEMENTATION_STEPPER + UART_DEMO) != 1
#error "Select exactly one mode: IMPLEMENTATION_STEPPER or UART_DEMO"
#endif



// 1 = beim Start Kalibrier-Routine ausführen
// 0 = normaler Start ohne Kalibrierung
#define CALIBRATION_MODE                0
#define DEMO_DRAW_MODE					0	// zeichnet aus test_tools_run() etwas auf die Arbeitsfläche

// Position feedback / closed loop
#define POSITION_ENABLE                 1   // 0=kein Encoder, 1=Encoder aktiv
#define POSITION_CLOSED_LOOP_ENABLE     1   // 0=nur messen, 1=MOVE korrigiert nach
#define POSITION_DEBUG					0	// 1=blocking loop printing encoder values
#endif


#if RELEASE
#define DEBUG_MODE						0
#define IMPLEMENTATION_STEPPER          1
#define UART_DEMO                       0

// 1 = beim Start Kalibrier-Routine ausführen
// 0 = normaler Start ohne Kalibrierung
#define CALIBRATION_MODE                0
#define DEMO_DRAW_MODE					0

// Position feedback / closed loop
#define POSITION_ENABLE                 1   // 0=kein Encoder, 1=Encoder aktiv
#define POSITION_CLOSED_LOOP_ENABLE     0   // 0=nur messen, 1=MOVE korrigiert nach
#define POSITION_DEBUG					0	//1=blocking loop printing encoder values
#endif

#endif /* CONFIG_BUILD_CONFIG_H_ */
