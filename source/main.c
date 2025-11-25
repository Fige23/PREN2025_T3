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
#include "fsl_gpio.h"

#include "uart.h"
#include "ftm0.h"
#include "ftm3.h"
#include "led.h"
#include "serial_port.h"
#include "cmd.h"
#include "bot.h"
//#include "init.h"


//Main function of Puzzle Robot
int main(void){
	//Diese Funktionen auslagern in init file:
	//Wrapper Funktionen schreiben für GPIO ein aus, nur ein file inkludieren.

	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitBootPeripherals();
	GPIO_PinWrite(BOARD_INITPINS_Magnet_GPIO, BOARD_INITPINS_Magnet_PIN, true);	//Schaltet pin High
	GPIO_PinWrite(BOARD_INITPINS_Magnet_GPIO, BOARD_INITPINS_Magnet_PIN, false);//Schaltet pin Low



	serial_init(115200);
	//magnet_init();
	//cmd_init();


	for(;;){
		cmd_poll();
		bot_step();
		__asm volatile("nop");
	}

}









