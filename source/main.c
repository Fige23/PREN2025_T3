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
#include "bot.h"
#include "io.h"
#include "ftm3.h"
#include "motion.h"
#include "job.h"
#include "console_goto.h"
#include "util.h"
#include "position.h"
#include "console_uart_sim.h"
#include "calibration.h"



//polling ESTOP
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

int main(void) {
	/*
	 * PINS AUF UART GEMUXT
	 */
	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitBootPeripherals(); //FTM3

#if CALIBRATION_MODE
	calibrate_n_iterations(5); //blocking!
#endif

#if !CALIBRATION_MODE
	serial_init(115200);	//init uart

	#if ENABLE_CONSOLE_GOTO
	    console_goto_init();	//debug
	#elif ENABLE_CONSOLE_UART_SIM
	    cmd_init();
	    console_uart_sim_init();
	#else
	    cmd_init();	//macht nix, gibt "CMD READY" aus.
	#endif

	ftm3_tick_init(STEP_TICK_HZ); //konfiguriert period. interrupt
	position_init();
	motion_init();	//setzt callback funktion für isr
	ftm3_tick_start();	//startet period. interrupt
	job_init();	//reset j.active = false, j.last_err = none, j.corr_iter = 0

	for (;;) {
	#if ENABLE_CONSOLE_GOTO
	    console_goto_poll();	//debug
	#elif ENABLE_CONSOLE_UART_SIM
	    console_uart_sim_poll();	//debug: uart schnittstelle auf semihost-konsole
	#else
	    cmd_poll(); //pollt uart
	#endif
	    estop_poll();	//pollt not-aus
	    position_poll();	//pollt encoder
	    bot_step();		// führt bewegung aus
	    __asm volatile("nop");
	}
#endif
}


