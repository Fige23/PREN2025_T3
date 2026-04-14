/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
debug.h	Created on: 15.03.2026	   Author: Fige23	Team 3                                                                
*/

#ifndef APP_DEBUG_H_
#define APP_DEBUG_H_
#include "robot_config.h"
#include <stdint.h>
#include "stdbool.h"
#if SYSTEMVIEW
typedef struct{
    bool sysview_track;
    uint32_t isr_cycles;
    uint32_t motion_steps;
    uint32_t sysview_starts;
}tSystemview_s;
#endif

extern volatile tSystemview_s g_systrack;

void debug_printf(const char *fmt, ...);




#endif /* APP_DEBUG_H_ */
