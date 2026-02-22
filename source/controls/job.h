/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
job.h	Created on: 18.12.2025	   Author: Fige23	Team 3                                                                
*/

#ifndef CONTROLS_JOB_H_
#define CONTROLS_JOB_H_

#include <stdbool.h>
#include "bot.h"
#include "protocol.h"

void job_init(void);

//Startet einen MOVE-Job (ohne Z-Safety/Phasen) -> fährt direkt auf Zielpose via motion_start()
err_e job_start_move(const bot_action_s *a);

//Muss zyklisch aufgerufen werden -> gibt true zurück wenn job fertig (OK oder ERR)
bool job_step(err_e *out_err);

bool job_is_active(void);




#endif /* CONTROLS_JOB_H_ */
