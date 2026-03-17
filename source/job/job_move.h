/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
job_move.h	Created on: 17.03.2026	   Author: Fige23	Team 3                                                                
*/

#ifndef JOB_JOB_MOVE_H_
#define JOB_JOB_MOVE_H_


#include <stdbool.h>
#include "protocol.h"
#include "bot_engine.h"

err_e job_move_start(const bot_action_s *a);
bool job_move_step(err_e *out_err);



#endif /* JOB_JOB_MOVE_H_ */
