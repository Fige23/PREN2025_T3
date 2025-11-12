/*Project: ${project_name}
 (   (          )   (            )   ) (               )
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_))
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|
${file_name}	Created on: ${date}	   Author: Fige23	Team 3
*/



// MCFUN artefact:
// calulate nr of TOF count for a given number of milliseconds

#define TOFS_MS(x)   ((uint16_t)(((FTM3_CLOCK / 1000) * x) / (FTM3_MODULO + 1)))

#include "platform.h"
#include "uart.h"
#include "ftm0.h"
#include "ftm3.h"
#include "led.h"
#include "serial_port.h"
#include "cmd.h"




//Main function of Puzzle Robot
int main(void){

	serial_init(115200);

	cmd_init();

	for(;;){
		cmd_poll();
		__asm volatile("nop");
	}

}









