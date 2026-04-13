/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
poll.c	Created on: 25.03.2026	   Author: Fige23	Team 3                                                                
*/
#include "robot_config.h"

#include "cmd.h"
#include "io.h"
#include "protocol.h"
#include "position.h"
#include "console_uart_sim.h"

// polling ESTOP
void estop_poll(void)
{
#if ESTOP_POLL_MODE == ESTOP_POLL_MODE_EDGE
    static bool last = false;
    bool now = estop_button_pressed();

    // Original behavior: latch only on press edge.
    if (now && !last) {
        g_status.estop = true;
    }

    last = now;
#else
    // Robust behavior: latch on active level.
    if (estop_button_pressed()) {
        g_status.estop = true;
    }
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

void poll_all(void){

    frontend_poll();

#if !ENABLE_CONSOLE_UART_SIM
    // ESTOP und Position werden in main loop gepollt
    // Bei Console-Sim werden sie in motion_tick_isr() gepollt (wegen blockierendem fgets)
    estop_poll();

#if POSITION_ENABLE
    position_poll();
#endif
#endif

}
