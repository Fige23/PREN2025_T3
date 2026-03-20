/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )
 (()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_))
 (_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
 | _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|
 |  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |
 |_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|
 main.c
 Created on: 13.11.2025
 Author: Fige23
 Team 3
 */

/*
 *
 * MAGNET UMKONFIGURIEREN VON PTA1 auf PTA13
 *
 *
 */

#include "debug.h"
#include "fsl_common.h"
#include "fsl_ftm.h"
#include "position.h"

#include "platform.h"
#include "board.h"
#include "pin_mux.h"
#include "peripherals.h"
#include "clock_config.h"

#include "robot_config.h"
#include "serial_port.h"
#include "cmd.h"
#include "bot_engine.h"
#include "io.h"
#include "ftm3.h"
#include "motion.h"
#include "position.h"
#include "console_uart_sim.h"
#include "calibration.h"
#include "job.h"
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

// polling ESTOP
static void estop_poll(void)
{
    static bool last = false;
    bool now = estop_button_pressed();

    // Flankenerkennung: nur beim Drücken latchen
    if (now && !last) {
        g_status.estop = true;
    }

    last = now;
}

static void frontend_init(void)
{
    cmd_init();

#if ENABLE_CONSOLE_UART_SIM
    console_uart_sim_init();
#endif
}

static void frontend_poll(void)
{
#if ENABLE_CONSOLE_UART_SIM
    console_uart_sim_poll();
#else
    cmd_poll();
#endif
}

int main(void)
{
    /*
     * Grundinitialisierung
     */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();   // FTM3 etc.

    serial_init(115200);




    ftm3_tick_init(STEP_TICK_HZ);  // konfiguriert periodischen Interrupt
    position_init();
#if POSITION_DEBUG
    debug_encoder_live_loop();
#endif

    motion_init();                 // setzt Callback-Funktion für ISR
    job_init();                    // reset j.active / j.last_err / j.corr_iter

    frontend_init();

#if CALIBRATION_MODE
    calibrate_n_iterations(CAL_AXIS_X, 5);   // blocking
#endif

    ftm3_tick_start();             // startet periodischen Interrupt

    for (;;) {
        frontend_poll();

        estop_poll();              // pollt Not-Aus
#if !ENABLE_CONSOLE_UART_SIM
        position_poll();
#endif

        bot_step();                // führt Bot-/Bewegungslogik aus

        __asm volatile("nop");
    }
}
