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
/*
 ===============================================================================
 main.c

 Aufgabe:
 - Initialisiert Board/Clocks/Pins, UART/Serial-Port und alle Robot-Module.
 - Startet den Motion-Tick (FTM3) und ruft danach zyklisch:
 cmd_poll();  -> nimmt Kommandos entgegen und queued Actions
 bot_step();  -> arbeitet Actions ab und sendet final OK/ERR

 Wichtige Module:
 - cmd.c:  Parsing/Validierung, erstellt bot_action_s und queued via bot_enqueue()
 - bot.c:  Scheduler/State-Machine, startet Jobs/Motion, sendet OK/ERR
 - job.c:  Sequenzen (später PICK/PLACE), MOVE ist 1 Segment
 - motion.c: Stepper-Puls-Generator im Timer-ISR, updatet pose_cmd (und aktuell auch pose_meas)
 ===============================================================================
 */
int main(void) {
	/*
	 *
	 * PINS AUF UART GEMUXT
	 *
	 *
	 */
	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitBootPeripherals();
	/*
	 * FTM3 Konfiguration:
	 * -Im ConficTool gemacht, wird durch InitBootPeripherals initialisiert.
	 * -Modus: Output compare, toggle auf channel 0,1,2,3 für die 4 stepper
	 * -channel interrupts im config tool aktiviert
	 */

	serial_init(115200);
#if ENABLE_CONSOLE_GOTO
	console_goto_init();
#else
    cmd_init();	//macht nichts sendet nur CMD_READY per UART
#endif

	//Init FTM3
	ftm3_tick_init(STEP_TICK_HZ);
	position_init();
	motion_init();
	ftm3_tick_start();
	job_init();



	for (;;) {
#if ENABLE_CONSOLE_GOTO
		console_goto_poll();
#else
		cmd_poll();
#endif
		position_poll();
		bot_step();
		__asm volatile("nop");
	}

}

