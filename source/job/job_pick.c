/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_))
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|
job_pick.c   Created on: 01.04.2026      Author: Fige23 Team 3
*/

#include "job_pick.h"

#include "motion.h"
#include "pick_config.h"
#include "io.h"
#include "bot_engine.h"
#include "job_motion_finish.h"

typedef enum {
    PICK_STATE_IDLE = 0,
    PICK_STATE_RAISE_TO_SAFE_Z,
    PICK_STATE_XY_MOVE,
    PICK_STATE_XY_SETTLE,
    PICK_STATE_Z_DOWN,
    PICK_STATE_Z_DOWN_SETTLE,
    PICK_STATE_MAGNET_WAIT,
    PICK_STATE_Z_UP,
    PICK_STATE_Z_UP_SETTLE,
    PICK_STATE_DONE,
    PICK_STATE_ERROR
} pick_state_e;

typedef struct {
    pick_state_e state;
    err_e last_err;
    uint32_t wait_start_tick;

    robot_pos_s pick_target;      /* original target from ACT_PICK */
    robot_pos_s xy_safe_target;   /* actual XY move target used for motion_finish */

    job_motion_finish_s xy_finish;
} job_pick_s;

static job_pick_s jp;

/* Z profiles */
static const motion_profile_s g_pick_z_down_profile = PICK_Z_DOWN_PROFILE;
static const motion_profile_s g_pick_z_up_profile = PICK_Z_UP_PROFILE;

/* -------------------------------------------------------------------------- */
/* Helpers                                                                    */
/* -------------------------------------------------------------------------- */

static bool wait_elapsed(uint32_t start_tick, uint32_t wait_ticks){
    return (uint32_t)(motion_get_isr_tick_count() - start_tick) >= wait_ticks;
}

static void pick_enter_wait(pick_state_e next_state){
    jp.state = next_state;
    jp.wait_start_tick = motion_get_isr_tick_count();
}

static bool pick_fail(err_e e, err_e* out_err){
    jp.state = PICK_STATE_ERROR;
    jp.last_err = e;

    if(out_err != 0){
        *out_err = e;
    }
    return true;
}

/*
 * With your current convention:
 *   larger z  => further down
 *   smaller z => higher up
 *
 * Therefore:
 *   current_z > safe_z  => tool is too low, so first go up to safe z.
 */
static bool pick_needs_raise_to_safe(void){
    return g_status.pos_internal.z_mm_scaled > PICK_Z_SAFE_POS_MM_SCALED;
}

static err_e pick_start_raise_to_safe_z(void){
    bot_action_s cmd = { 0 };

    cmd.type = ACT_PICK;
    cmd.target_pos = g_status.pos_internal;
    cmd.target_pos.z_mm_scaled = PICK_Z_SAFE_POS_MM_SCALED;

    /* Reuse Z-up profile for this upward move */
    return motion_start(&cmd, limit_none, &g_pick_z_up_profile);
}

static err_e pick_start_xy_move(const robot_pos_s* target_xy){
    bot_action_s cmd = { 0 };

    cmd.type = ACT_PICK;
    cmd.target_pos = g_status.pos_internal;

    /* Move only in XY to target, keep Z at safe height, phi forced to 0 for pick */
    cmd.target_pos.x_mm_scaled = target_xy->x_mm_scaled;
    cmd.target_pos.y_mm_scaled = target_xy->y_mm_scaled;
    cmd.target_pos.z_mm_scaled = PICK_Z_SAFE_POS_MM_SCALED;
    cmd.target_pos.phi_deg_scaled = 0;

    jp.xy_safe_target = cmd.target_pos;
    job_motion_finish_init(&jp.xy_finish, &jp.xy_safe_target);

    return motion_start(&cmd, limit_none, 0);
}

static err_e pick_start_z_down(void){
    bot_action_s cmd = { 0 };

    cmd.type = ACT_PICK;
    cmd.target_pos = g_status.pos_internal;
    cmd.target_pos.z_mm_scaled = PICK_Z_GRIP_POS_MM_SCALED;

    return motion_start(&cmd, limit_none, &g_pick_z_down_profile);
}

static err_e pick_start_z_up(void){
    bot_action_s cmd = { 0 };

    cmd.type = ACT_PICK;
    cmd.target_pos = g_status.pos_internal;
    cmd.target_pos.z_mm_scaled = PICK_Z_SAFE_POS_MM_SCALED;

    return motion_start(&cmd, limit_none, &g_pick_z_up_profile);
}

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

err_e job_pick_start(const bot_action_s* a){
    err_e e;

    if(a == 0){
        return ERR_INTERNAL;
    }

    jp = (job_pick_s){ 0 };
    jp.state = PICK_STATE_IDLE;
    jp.last_err = ERR_NONE;
    jp.pick_target = a->target_pos;

    if(pick_needs_raise_to_safe()){
        e = pick_start_raise_to_safe_z();
        if(e != ERR_NONE){
            jp.state = PICK_STATE_ERROR;
            jp.last_err = e;
            return e;
        }

        jp.state = PICK_STATE_RAISE_TO_SAFE_Z;
        return ERR_NONE;
    }

    e = pick_start_xy_move(&jp.pick_target);
    if(e != ERR_NONE){
        jp.state = PICK_STATE_ERROR;
        jp.last_err = e;
        return e;
    }

    jp.state = PICK_STATE_XY_MOVE;
    return ERR_NONE;
}

bool job_pick_step(err_e* out_err){
    err_e e = ERR_NONE;
    bool done = false;

    switch(jp.state){
    case PICK_STATE_RAISE_TO_SAFE_Z:
        if(!motion_is_done()){
            return false;
        }

        e = motion_last_err();
        if(e != ERR_NONE){
            return pick_fail(e, out_err);
        }

        e = pick_start_xy_move(&jp.pick_target);
        if(e != ERR_NONE){
            return pick_fail(e, out_err);
        }

        jp.state = PICK_STATE_XY_MOVE;
        return false;

    case PICK_STATE_XY_MOVE:
        done = job_motion_finish_step(&jp.xy_finish, &e);
        if(!done){
            return false;
        }

        if(e != ERR_NONE){
            return pick_fail(e, out_err);
        }

        pick_enter_wait(PICK_STATE_XY_SETTLE);
        return false;

    case PICK_STATE_XY_SETTLE:
        if(!wait_elapsed(jp.wait_start_tick, PICK_WAIT_TICKS_XY_SETTLE)){
            return false;
        }

        e = pick_start_z_down();
        if(e != ERR_NONE){
            return pick_fail(e, out_err);
        }

        jp.state = PICK_STATE_Z_DOWN;
        return false;

    case PICK_STATE_Z_DOWN:
        if(!motion_is_done()){
            return false;
        }

        e = motion_last_err();
        if(e != ERR_NONE){
            return pick_fail(e, out_err);
        }

        pick_enter_wait(PICK_STATE_Z_DOWN_SETTLE);
        return false;

    case PICK_STATE_Z_DOWN_SETTLE:
        if(!wait_elapsed(jp.wait_start_tick, PICK_WAIT_TICKS_Z_SETTLE)){
            return false;
        }

        magnet_on_off(true);
        pick_enter_wait(PICK_STATE_MAGNET_WAIT);
        return false;

    case PICK_STATE_MAGNET_WAIT:
        if(!wait_elapsed(jp.wait_start_tick, PICK_WAIT_TICKS_MAGNET_GRAB)){
            return false;
        }

        e = pick_start_z_up();
        if(e != ERR_NONE){
            return pick_fail(e, out_err);
        }

        jp.state = PICK_STATE_Z_UP;
        return false;

    case PICK_STATE_Z_UP:
        if(!motion_is_done()){
            return false;
        }

        e = motion_last_err();
        if(e != ERR_NONE){
            return pick_fail(e, out_err);
        }

        pick_enter_wait(PICK_STATE_Z_UP_SETTLE);
        return false;

    case PICK_STATE_Z_UP_SETTLE:
        if(!wait_elapsed(jp.wait_start_tick, PICK_WAIT_TICKS_Z_SETTLE)){
            return false;
        }

        jp.state = PICK_STATE_DONE;
        jp.last_err = ERR_NONE;

        if(out_err != 0){
            *out_err = ERR_NONE;
        }
        return true;

    case PICK_STATE_DONE:
        if(out_err != 0){
            *out_err = ERR_NONE;
        }
        return true;

    case PICK_STATE_ERROR:
        if(out_err != 0){
            *out_err = jp.last_err;
        }
        return true;

    case PICK_STATE_IDLE:
    default:
        if(out_err != 0){
            *out_err = ERR_INTERNAL;
        }
        return true;
    }
}