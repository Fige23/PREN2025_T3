/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
ftm3.h	Created on: 18.12.2025	   Author: Fige23	Team 3                                                                
*/

#ifndef UTILS_FTM3_H_
#define UTILS_FTM3_H_

#include <stdint.h>

typedef void (*ftm3_tick_cb_t)(void);

// Initialisiert FTM3 für Overflow-Ticks mit tick_hz (z.B. 50000)
void ftm3_tick_init(uint32_t tick_hz);

// Start/Stop
void ftm3_tick_start(void);
void ftm3_tick_stop(void);

// Callback, der bei jedem Tick aus dem ISR aufgerufen wird
void ftm3_tick_set_callback(ftm3_tick_cb_t cb);

#endif /* UTILS_FTM3_H_ */
