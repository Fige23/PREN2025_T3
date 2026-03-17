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

#ifndef JOB_JOB_H_
#define JOB_JOB_H_

#include "bot_engine.h"
#include <stdbool.h>
#include "protocol.h"

void job_init(void);
err_e job_start(const bot_action_s *a);
bool job_step(err_e *out_err);

bool job_is_active(void);
err_e job_last_err(void);

#endif /* JOB_JOB_H_ */
