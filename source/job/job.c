/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_))
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|
job.c	Created on: 18.12.2025	   Author: Fige23	Team 3
*/
#include "job.h"

#include "job_move.h"
#include "job_home.h"
#include "job_pick.h"
#include "job_place.h"
#include "io.h"

typedef enum {
    JOB_TYPE_NONE = 0,
    JOB_TYPE_MOVE,
    JOB_TYPE_HOME,
    JOB_TYPE_PICK,
    JOB_TYPE_PLACE
}job_type_e;

typedef struct{
    bool active;
    err_e last_err;
    job_type_e type;
}job_s;

static job_s j;

void job_init(void){
    j.active = false;
    j.last_err = ERR_NONE;
    j.type = JOB_TYPE_NONE;
}

void job_abort(void){
    job_init();
}

bool job_is_active(void){
    return j.active;
}

err_e job_last_err(void){
    return j.last_err;
}

err_e job_start(const bot_action_s* a){
    if(a == 0) return ERR_INTERNAL;
    if(j.active) return ERR_INTERNAL;

    err_e e = ERR_INTERNAL;
    switch(a->type){
    case ACT_MOVE:
        e = job_move_start(a);
        if(e == ERR_NONE){
            j.active = true;
            j.last_err = ERR_NONE;
            j.type = JOB_TYPE_MOVE;
        }
        return e;
    case ACT_HOME:
        e = job_home_start(a);
        if(e == ERR_NONE){
            j.active = true;
            j.last_err = ERR_NONE;
            j.type = JOB_TYPE_HOME;
        }
        return e;
    case ACT_PICK:
        e = job_pick_start(a);
        if(e == ERR_NONE){
            j.active = true;
            j.last_err = ERR_NONE;
            j.type = JOB_TYPE_PICK;
        }
        return e;
    case ACT_PLACE:
        e = job_place_start(a);
        if(e == ERR_NONE){
            j.active = true;
            j.last_err = ERR_NONE;
            j.type = JOB_TYPE_PLACE;
        }
        return e;
    default:
        return ERR_INTERNAL;
    }
}

bool job_step(err_e* out_err){

    if(!j.active){
        if(out_err) *out_err = j.last_err;
        return true;
    }
    bool done = false;
    err_e e = ERR_NONE;

    switch(j.type){
    case JOB_TYPE_MOVE:
        done = job_move_step(&e);
        break;
    case JOB_TYPE_HOME:
        done = job_home_step(&e);
        break;
    case JOB_TYPE_PICK:
        done = job_pick_step(&e);
        break;
    case JOB_TYPE_PLACE:
        done = job_place_step(&e);
        break;

    case JOB_TYPE_NONE:
    default:
        done = true;
        e = ERR_INTERNAL;
        break;
    }
    if(done){
        j.active = false;
        j.last_err = e;
        j.type = JOB_TYPE_NONE;

        if(out_err) *out_err = j.last_err;
        return true;
    }
    return false;
}













