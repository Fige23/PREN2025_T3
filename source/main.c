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
#include "SEGGER_SYSVIEW.h"

int main(void)
{

	init_all();						//initialisiert alles

    SEGGER_SYSVIEW_Conf();
    SEGGER_SYSVIEW_Start();


    ftm3_tick_start();              // startet periodischen Interrupt

    test_tools_run();				//führt alle aktivierten Testfunktionen aus

    for (;;) {
        //SEGGER_SYSVIEW_OnUserStart(1);
    	poll_all();					//pollt alle schalter und sensoren
        //SEGGER_SYSVIEW_OnUserStop(1);

        //SEGGER_SYSVIEW_OnUserStart(2);
        bot_step();                	// führt Bot-/Bewegungslogik aus
        //SEGGER_SYSVIEW_OnUserStop(2);
        __asm volatile("nop");
    }
}
