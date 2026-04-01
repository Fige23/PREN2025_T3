/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_))
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|
job_place.h	Created on: 01.04.2026	   Author: Fige23	Team 3
*/

#ifndef JOB_JOB_PLACE_H_
#define JOB_JOB_PLACE_H_

#include <stdbool.h>
#include "protocol.h"
#include "bot_engine.h"

// Start a place operation:
// XY+phi movement to target position at SAFE_Z height,
// then Z down to place position, magnet off, wait, and Z back up
err_e job_place_start(const bot_action_s *a);

// Step function - call cyclically from main loop
// Returns true when operation is complete (success or error)
bool job_place_step(err_e *out_err);

#endif /* JOB_JOB_PLACE_H_ */
