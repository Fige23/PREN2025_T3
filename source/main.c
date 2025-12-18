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



// MCFUN artefact:
// calulate nr of TOF count for a given number of milliseconds

#define TOFS_MS(x)   ((uint16_t)(((FTM3_CLOCK / 1000) * x) / (FTM3_MODULO + 1)))

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


//Main function of Puzzle Robot
int main(void){


	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitBootPeripherals();
	/*
	 * FTM3 Konfiguration:
	 * -Im ConficTool gemacht, wird durch InitBootPeripherals initialisiert.
	 * -Modus: Output compare, toggle auf channel 0,1,2,3 für die 4 stepper
	 * -channel interrupts im config tool aktiviert
	 *
	 * TODO:
	 * -eigenes stepper file
	 * -FTM3_IRQHandler anlegen und unterscheiden welcher channel interrupt ausgelöst hat
	 * -Wrapper funktionen erstellen für verwendung.
	 * -Aktuell wird ftm noch nicht verwendet.
	 *
	 *
	 */

	serial_init(115200);
	//magnet_init();
	cmd_init(); //Macht nichts, sendet nur CMD_READY über die UART

	//Init FTM3
	ftm3_tick_init(STEP_TICK_HZ);
	motion_init();
	ftm3_tick_start();
	job_init();

	for(;;){
		cmd_poll();
		bot_step();
		__asm volatile("nop");
	}

}









