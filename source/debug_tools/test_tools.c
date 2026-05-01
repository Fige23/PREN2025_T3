/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\()))` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
test_tools.c	Created on: 25.03.2026	   Author: Fige23	Team 3                                                                
*/

#ifndef APP_TEST_TOOLS_C_
#define APP_TEST_TOOLS_C_

#include "robot_config.h"

#include "calibration.h"
#include "demo_draw.h"
#include "position_debug.h"
#include "tmc2209_uart_test.h"

// Test/Debug Features Coordinator
// This file triggers various debug features based on build_config.h settings

void test_tools_run(void) {

#if POSITION_DEBUG
    // WARNING: This is a blocking loop that never returns!
    // Only enable for encoder debugging.
    position_debug_live_loop();
#endif

#if CALIBRATION_MODE
    // Run automatic calibration on startup
    calibrate_n_iterations(CAL_AXIS_X, 5);   // blocking
#endif

#if TMC2209_UART_TEST_MODE && TMC2209_UART_ENABLE
    // Probe every configured TMC2209 address once on startup.
    tmc2209_uart_test_run();
#endif

#if DEMO_DRAW_MODE
    // Queue demo pattern once
    static bool demo_loaded = false;
    if (!demo_loaded) {
        demo_loaded = demo_enqueue_pattern(DEMO_PATTERN_STAR);
    }
#endif

}
#endif /* APP_TEST_TOOLS_C_ */
