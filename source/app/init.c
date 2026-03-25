/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
init.c	Created on: 25.03.2026	   Author: Fige23	Team 3                                                                
*/
#include "robot_config.h"

#include "cmd.h"
#include "pin_mux.h"
#include "platform.h"
#include "peripherals.h"
#include "clock_config.h"
#include "console_uart_sim.h"
#include "serial_port.h"
#include "ftm3.h"
#include "position.h"
#include "motion.h"
#include "job.h"

#if !POSITION_DEBUG
static void frontend_init(void)
{
    cmd_init();

#if ENABLE_CONSOLE_UART_SIM
    console_uart_sim_init();
#endif
}
#endif




void init_all(void){

    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();   // FTM3 etc.

    serial_init(115200);


    ftm3_tick_init(STEP_TICK_HZ);  // konfiguriert periodischen Interrupt
    position_init();



#if !POSITION_DEBUG
    motion_init();                 // setzt Callback-Funktion für ISR
    job_init();                    // reset j.active / j.last_err / j.corr_iter

    frontend_init();
#endif
}
