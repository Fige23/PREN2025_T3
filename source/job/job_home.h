/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_))
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|
job_home.h	Created on: 17.03.2026	   Author: Fige23	Team 3
*/

#ifndef JOB_JOB_HOME_H_
#define JOB_JOB_HOME_H_

#include <stdbool.h>
#include "protocol.h"
#include "bot_engine.h"

err_e job_home_start(const bot_action_s* a);
bool job_home_step(err_e* out_err);
void home_apply_x_reference(void);
void home_apply_y_reference(void);


#endif /* JOB_JOB_HOME_H_ */
