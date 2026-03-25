/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
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
#include "clock_config.h"
#include "debug.h"
#include "fsl_ftm.h"
#include "position.h"

//debug encoder
#if POSITION_DEBUG
static void debug_encoder_live_loop(void)
{
    debug_printf("\r\n=== DEBUG ENCODER LIVE LOOP ===\r\n");
    debug_printf("Move encoder by hand.\r\n");
    debug_printf("Showing raw FTM counts, accumulated counts and measured mm.\r\n\r\n");

    for (;;) {
        position_poll();

        uint16_t ftm1_raw = (uint16_t)FTM_GetQuadDecoderCounterValue(FTM1);
        uint16_t ftm2_raw = (uint16_t)FTM_GetQuadDecoderCounterValue(FTM2);

        int32_t x_cnt = position_get_x_counts();
        int32_t y_cnt = position_get_y_counts();

        int32_t x_mm = position_get_x_mm_scaled();
        int32_t y_mm = position_get_y_mm_scaled();

        debug_printf(
            "FTM1=%u FTM2=%u   "
            "X_CNT=%ld Y_CNT=%ld   "
            "X_MM=%ld.%03ld Y_MM=%ld.%03ld\r\n",
            (unsigned)ftm1_raw,
            (unsigned)ftm2_raw,
            (long)x_cnt,
            (long)y_cnt,
            (long)(x_mm / 1000), (long)((x_mm >= 0 ? x_mm : -x_mm) % 1000),
            (long)(y_mm / 1000), (long)((y_mm >= 0 ? y_mm : -y_mm) % 1000)
        );

        SDK_DelayAtLeastUs(100000u, CLOCK_GetFreq(kCLOCK_CoreSysClk)); // 100 ms
    }
}

#endif
void test_tools_run(void) {

#if POSITION_DEBUG
    debug_encoder_live_loop();
#endif

#if CALIBRATION_MODE
    calibrate_n_iterations(CAL_AXIS_X, 5);   // blocking
#endif

#if DEMO_DRAW_MODE
    static bool demo_loaded = false;

    if (!demo_loaded) {
        demo_loaded = demo_enqueue_pattern(DEMO_PATTERN_STAR);
        }
#endif

}
#endif /* APP_TEST_TOOLS_C_ */
