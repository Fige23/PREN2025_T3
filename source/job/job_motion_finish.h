/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
job_motion_finish.h	Created on: 17.03.2026	   Author: Fige23	Team 3                                                                
*/

#ifndef JOB_JOB_MOTION_FINISH_H_
#define JOB_JOB_MOTION_FINISH_H_

#include <stdbool.h>
#include "protocol.h"

typedef struct{
	robot_pos_s final_target;
	uint8_t corr_iter;
}job_motion_finish_s;

// Kontext für eine Bewegung initialisieren.
// Wird einmal beim Start des Jobs aufgerufen.
void job_motion_finish_init(job_motion_finish_s *ctx, const robot_pos_s *final_target);

// Muss zyklisch aufgerufen werden, nachdem die erste motion_start(...)-Bewegung
// bereits gestartet wurde.
//
// Rückgabe:
// false -> noch nicht fertig, entweder Motion läuft noch oder ein Korrektursegment wurde gestartet
// true  -> gesamte Bewegung ist fertig (OK oder Fehler)
//
// out_err:
// ERR_NONE  -> Ziel erfolgreich erreicht
// anderer err_e -> Fehler / Abbruch
bool job_motion_finish_step(job_motion_finish_s *ctx, err_e *out_err);
#endif /* JOB_JOB_MOTION_FINISH_H_ */
