/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_))
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|
job_place.c
*/

#include "job_place.h"

#include "motion.h"
#include "place_config.h"
#include "io.h"
#include "bot_engine.h"
#include "job_motion_finish.h"

typedef enum {
    PLACE_STATE_IDLE = 0,
    PLACE_STATE_RAISE_TO_SAFE_Z,
    PLACE_STATE_XY_PHI_MOVE,
    PLACE_STATE_XY_PHI_SETTLE,
    PLACE_STATE_Z_DOWN,
    PLACE_STATE_Z_DOWN_SETTLE,
    PLACE_STATE_MAGNET_RELEASE_WAIT,
    PLACE_STATE_Z_UP,
    PLACE_STATE_Z_UP_SETTLE,
    PLACE_STATE_DONE,
    PLACE_STATE_ERROR
} place_state_e;

typedef struct {
    place_state_e state;
    err_e last_err;
    uint32_t wait_start_tick;

    robot_pos_s place_target;        /* original ACT_PLACE target */
    robot_pos_s xy_phi_safe_target;  /* actual target used for motion_finish */

    job_motion_finish_s xy_phi_finish;
} job_place_s;

static job_place_s jp;

/* Z profiles */
static const motion_profile_s g_place_z_down_profile = PLACE_Z_DOWN_PROFILE;
static const motion_profile_s g_place_z_up_profile   = PLACE_Z_UP_PROFILE;

/* -------------------------------------------------------------------------- */
/* Helpers                                                                    */
/* -------------------------------------------------------------------------- */

static bool wait_elapsed(uint32_t start_tick, uint32_t wait_ticks)
{
    return (motion_get_isr_tick_count() - start_tick) >= wait_ticks;
}

static void place_enter_wait(place_state_e next_state)
{
    jp.state = next_state;
    jp.wait_start_tick = motion_get_isr_tick_count();
}

static bool place_fail(err_e e, err_e *out_err)
{
    jp.state = PLACE_STATE_ERROR;
    jp.last_err = e;

    if (out_err != 0) {
        *out_err = e;
    }
    return true;
}

/*
 * Current convention:
 *   larger z  => further down
 *   smaller z => higher up
 *
 * Therefore:
 *   current_z > safe_z => tool is too low, so first move only Z to safe.
 */
static bool place_needs_raise_to_safe(void)
{
    return g_status.pos_internal.z_mm_scaled > PLACE_Z_SAFE_POS_MM_SCALED;
}

static err_e place_start_raise_to_safe_z(void)
{
    bot_action_s cmd = {0};

    cmd.type = ACT_PLACE;
    cmd.target_pos = g_status.pos_internal;
    cmd.target_pos.z_mm_scaled = PLACE_Z_SAFE_POS_MM_SCALED;

    return motion_start(&cmd, limit_none, &g_place_z_up_profile, MOTION_PROFILE_KIND_PLACE);
}

static err_e place_start_xy_phi_move(const robot_pos_s *target_pos)
{
    bot_action_s cmd = {0};

    cmd.type = ACT_PLACE;
    cmd.target_pos = g_status.pos_internal;

    /* Move only in XY+phi, keep Z at safe height */
    cmd.target_pos.x_mm_scaled    = target_pos->x_mm_scaled;
    cmd.target_pos.y_mm_scaled    = target_pos->y_mm_scaled;
    cmd.target_pos.z_mm_scaled    = PLACE_Z_SAFE_POS_MM_SCALED;
    cmd.target_pos.phi_deg_scaled = target_pos->phi_deg_scaled;

    jp.xy_phi_safe_target = cmd.target_pos;
    job_motion_finish_init(&jp.xy_phi_finish, &jp.xy_phi_safe_target);

    return motion_start(&cmd, limit_none, 0, MOTION_PROFILE_KIND_PLACE);
}

static err_e place_start_z_down(void)
{
    bot_action_s cmd = {0};

    cmd.type = ACT_PLACE;
    cmd.target_pos = g_status.pos_internal;
    cmd.target_pos.z_mm_scaled = PLACE_Z_DROP_POS_MM_SCALED;

    return motion_start(&cmd, limit_none, &g_place_z_down_profile, MOTION_PROFILE_KIND_PLACE);
}

static err_e place_start_z_up(void)
{
    bot_action_s cmd = {0};

    cmd.type = ACT_PLACE;
    cmd.target_pos = g_status.pos_internal;
    cmd.target_pos.z_mm_scaled = PLACE_Z_SAFE_POS_MM_SCALED;

    return motion_start(&cmd, limit_none, &g_place_z_up_profile, MOTION_PROFILE_KIND_PLACE);
}

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

err_e job_place_start(const bot_action_s *a)
{
    err_e e;

    if (a == 0) {
        return ERR_INTERNAL;
    }

    jp = (job_place_s){0};
    jp.state = PLACE_STATE_IDLE;
    jp.last_err = ERR_NONE;
    jp.place_target = a->target_pos;

    if (place_needs_raise_to_safe()) {
        e = place_start_raise_to_safe_z();
        if (e != ERR_NONE) {
            jp.state = PLACE_STATE_ERROR;
            jp.last_err = e;
            return e;
        }

        jp.state = PLACE_STATE_RAISE_TO_SAFE_Z;
        return ERR_NONE;
    }

    e = place_start_xy_phi_move(&jp.place_target);
    if (e != ERR_NONE) {
        jp.state = PLACE_STATE_ERROR;
        jp.last_err = e;
        return e;
    }

    jp.state = PLACE_STATE_XY_PHI_MOVE;
    return ERR_NONE;
}

bool job_place_step(err_e *out_err)
{
    err_e e = ERR_NONE;
    bool done = false;

    switch (jp.state) {
    case PLACE_STATE_RAISE_TO_SAFE_Z:
        if (!motion_is_done()) {
            return false;
        }

        e = motion_last_err();
        if (e != ERR_NONE) {
            return place_fail(e, out_err);
        }

        e = place_start_xy_phi_move(&jp.place_target);
        if (e != ERR_NONE) {
            return place_fail(e, out_err);
        }

        jp.state = PLACE_STATE_XY_PHI_MOVE;
        return false;

    case PLACE_STATE_XY_PHI_MOVE:
        done = job_motion_finish_step(&jp.xy_phi_finish, &e);
        if (!done) {
            return false;
        }

        if (e != ERR_NONE) {
            return place_fail(e, out_err);
        }

        place_enter_wait(PLACE_STATE_XY_PHI_SETTLE);
        return false;

    case PLACE_STATE_XY_PHI_SETTLE:
        if (!wait_elapsed(jp.wait_start_tick, PLACE_WAIT_TICKS_XY_PHI_SETTLE)) {
            return false;
        }

        e = place_start_z_down();
        if (e != ERR_NONE) {
            return place_fail(e, out_err);
        }

        jp.state = PLACE_STATE_Z_DOWN;
        return false;

    case PLACE_STATE_Z_DOWN:
        if (!motion_is_done()) {
            return false;
        }

        e = motion_last_err();
        if (e != ERR_NONE) {
            return place_fail(e, out_err);
        }

        place_enter_wait(PLACE_STATE_Z_DOWN_SETTLE);
        return false;

    case PLACE_STATE_Z_DOWN_SETTLE:
        if (!wait_elapsed(jp.wait_start_tick, PLACE_WAIT_TICKS_Z_SETTLE)) {
            return false;
        }

        magnet_on_off(false);
        place_enter_wait(PLACE_STATE_MAGNET_RELEASE_WAIT);
        return false;

    case PLACE_STATE_MAGNET_RELEASE_WAIT:
        if (!wait_elapsed(jp.wait_start_tick, PLACE_WAIT_TICKS_MAGNET_RELEASE)) {
            return false;
        }

        e = place_start_z_up();
        if (e != ERR_NONE) {
            return place_fail(e, out_err);
        }

        jp.state = PLACE_STATE_Z_UP;
        return false;

    case PLACE_STATE_Z_UP:
        if (!motion_is_done()) {
            return false;
        }

        e = motion_last_err();
        if (e != ERR_NONE) {
            return place_fail(e, out_err);
        }

        place_enter_wait(PLACE_STATE_Z_UP_SETTLE);
        return false;

    case PLACE_STATE_Z_UP_SETTLE:
        if (!wait_elapsed(jp.wait_start_tick, PLACE_WAIT_TICKS_Z_SETTLE)) {
            return false;
        }

        jp.state = PLACE_STATE_DONE;
        jp.last_err = ERR_NONE;

        if (out_err != 0) {
            *out_err = ERR_NONE;
        }
        return true;

    case PLACE_STATE_DONE:
        if (out_err != 0) {
            *out_err = ERR_NONE;
        }
        return true;

    case PLACE_STATE_ERROR:
        if (out_err != 0) {
            *out_err = jp.last_err;
        }
        return true;

    case PLACE_STATE_IDLE:
    default:
        if (out_err != 0) {
            *out_err = ERR_INTERNAL;
        }
        return true;
    }
}
