/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
motion.h	Created on: 24.11.2025	   Author: Fige23	Team 3                                                                
*/

#ifndef MOTION_MOTION_H_
#define MOTION_MOTION_H_

#include <stdint.h>
#include <stdbool.h>
#include "protocol.h"
#include "bot.h"

//nicht fix
err_e motion_start(bot_action_s *cur);
//err_e home_start(bot_action_s *cur);

//err_e job_start_pick(bot_action_s *cur);
//err_e job_start_place(bot_action_s *cur);
void motion_init(void);
bool motion_is_done(void);
err_e motion_last_err(void);



#endif /* MOTION_MOTION_H_ */
