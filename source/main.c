/*Project: ${project_name}
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

#include <job/job.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "platform.h"
#include "board.h"
#include "pin_mux.h"
#include "peripherals.h"
#include "clock_config.h"

#include "robot_config.h"
#include "uart.h"
#include "serial_port.h"
#include "cmd.h"
#include "bot_engine.h"
#include "io.h"
#include "ftm3.h"
#include "motion.h"
#include "util.h"
#include "position.h"
#include "console_uart_sim.h"
#include "calibration.h"


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
        position_poll();           // pollt Encoder
        bot_step();                // führt Bot-/Bewegungslogik aus

        __asm volatile("nop");
    }
}
