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

//Neues makro machen, welches UART pin konfiguration ausschaötet
#include "robot_config.h"

#include "debug.h"
#include "ftm3.h"
#include "init.h"
#include "poll.h"
#include "test_tools.h"
#include "bot_engine.h"

int main(void)
{

	init_all();						//initialisiert alles
    ftm3_tick_start();              // startet periodischen Interrupt

    test_tools_run();				//führt alle aktivierten Testfunktionen aus

    for (;;) {

    	poll_all();					//pollt alle schalter und sensoren

        bot_step();                	// führt Bot-/Bewegungslogik aus

        __asm volatile("nop");
    }
}
