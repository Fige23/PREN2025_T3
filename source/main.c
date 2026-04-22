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
#if SYSTEMVIEW
#include "SEGGER_SYSVIEW.h"
#endif
#include "io.h"
int main(void){
    init_all();
    enable_pin(true);						//initialisiert alles
#if SYSTEMVIEW
    SEGGER_SYSVIEW_Conf();
    if(g_systrack.sysview_track){
        SEGGER_SYSVIEW_Start();
        g_systrack.sysview_starts = 1;
    }
    else if(g_systrack.sysview_starts == 1){
        SEGGER_SYSVIEW_Stop();
    }
#endif

    ftm3_tick_start();              // startet periodischen Interrupt

    test_tools_run();				//führt alle aktivierten Testfunktionen aus

    for(;;){
#if SYSTEMVIEW
        SEGGER_SYSVIEW_OnUserStart(1);
#endif
        poll_all();					//pollt alle schalter und sensoren
#if SYSTEMVIEW
        SEGGER_SYSVIEW_OnUserStop(1);


        SEGGER_SYSVIEW_OnUserStart(2);
#endif
        bot_step();                	// führt Bot-/Bewegungslogik aus
#if SYSTEMVIEW
        SEGGER_SYSVIEW_OnUserStop(2);
#endif
        __asm volatile("nop");
    }
}
