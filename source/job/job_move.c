/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
job_move.c	Created on: 17.03.2026	   Author: Fige23	Team 3                                                                
*/

#include "job_move.h"

#include "motion.h"
#include "limit_switch.h"
#include "job_motion_finish.h"

typedef struct {
	job_motion_finish_s finish;
}job_move_s;

static job_move_s jm;

err_e job_move_start(const bot_action_s *a){

	if(a == 0) return ERR_INTERNAL;

	//Endziel für abschluss/korrekturteil merken
	job_motion_finish_init(&jm.finish, &a->target_pos);

	//Erste bewegung starten
	return motion_start(a, limit_none, 0);
}

//delegiert alles an job_motion_finish_step
//weil hier keine logik nötig!
bool job_move_step(err_e *out_err){
	return job_motion_finish_step(&jm.finish, out_err);
}


