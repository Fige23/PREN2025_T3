/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_))
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|
job_motion_finish.h
*/

#ifndef JOB_JOB_MOTION_FINISH_H_
#define JOB_JOB_MOTION_FINISH_H_

#include <stdbool.h>
#include "protocol.h"

typedef struct {
    robot_pos_s final_target;
    uint8_t corr_iter;

    /* Debug / convergence tracking */
    int32_t prev_ex;
    int32_t prev_ey;
    uint8_t no_progress_count;
    bool prev_valid;
    bool correction_microsteps_active;
} job_motion_finish_s;

void job_motion_finish_init(job_motion_finish_s *ctx, const robot_pos_s *final_target);
bool job_motion_finish_step(job_motion_finish_s *ctx, err_e *out_err);

#endif /* JOB_JOB_MOTION_FINISH_H_ */
