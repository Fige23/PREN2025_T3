/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_))
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|

job_home.c
Created on: 17.03.2026
Author: Fige23
Team 3
*/

#include "job_home.h"

#include "motion.h"
#include "limit_switch.h"
#include "protocol.h"
#include "robot_config.h"


#include "position.h"


typedef enum {
    HOME_STATE_IDLE = 0,

#if HOME_ENABLE_Z
    HOME_STATE_Z_RELEASE,
    HOME_STATE_Z_SEEK_FAST,
    HOME_STATE_Z_BACKOFF,
    HOME_STATE_Z_SEEK_SLOW,
#endif

#if HOME_ENABLE_X
    HOME_STATE_X_RELEASE,
    HOME_STATE_X_SEEK_FAST,
    HOME_STATE_X_BACKOFF,
    HOME_STATE_X_SEEK_SLOW,
#endif

#if HOME_ENABLE_Y
    HOME_STATE_Y_RELEASE,
    HOME_STATE_Y_SEEK_FAST,
    HOME_STATE_Y_BACKOFF,
    HOME_STATE_Y_SEEK_SLOW,
#endif

    HOME_STATE_DONE,
    HOME_STATE_ERROR
} home_state_e;


typedef struct {
    home_state_e state;
} job_home_s;

static job_home_s jh;


#if HOME_ENABLE_X
static const motion_profile_s g_home_x_release_profile = HOME_X_RELEASE_PROFILE;
static const motion_profile_s g_home_x_fast_profile = HOME_X_FAST_PROFILE;
static const motion_profile_s g_home_x_backoff_profile = HOME_X_BACKOFF_PROFILE;
static const motion_profile_s g_home_x_slow_profile = HOME_X_SLOW_PROFILE;
#endif

#if HOME_ENABLE_Y
static const motion_profile_s g_home_y_release_profile = HOME_Y_RELEASE_PROFILE;
static const motion_profile_s g_home_y_fast_profile = HOME_Y_FAST_PROFILE;
static const motion_profile_s g_home_y_backoff_profile = HOME_Y_BACKOFF_PROFILE;
static const motion_profile_s g_home_y_slow_profile = HOME_Y_SLOW_PROFILE;
#endif

#if HOME_ENABLE_Z
static const motion_profile_s g_home_z_release_profile = HOME_Z_RELEASE_PROFILE;
static const motion_profile_s g_home_z_fast_profile = HOME_Z_FAST_PROFILE;
static const motion_profile_s g_home_z_backoff_profile = HOME_Z_BACKOFF_PROFILE;
static const motion_profile_s g_home_z_slow_profile = HOME_Z_SLOW_PROFILE;
#endif


// -----------------------------------------------------------------------------
// Local helpers: Referenz anwenden
// -----------------------------------------------------------------------------
#if HOME_ENABLE_X
void home_apply_x_reference(void){
    g_status.pos_internal.x_mm_scaled = HOME_X_ZERO_OFFSET_MM_SCALED;
    position_set_x_mm_scaled(HOME_X_ZERO_OFFSET_MM_SCALED);
}
#endif
#if HOME_ENABLE_Y
void home_apply_y_reference(void){
    g_status.pos_internal.y_mm_scaled = HOME_Y_ZERO_OFFSET_MM_SCALED;
    position_set_y_mm_scaled(HOME_Y_ZERO_OFFSET_MM_SCALED);
}
#endif
#if HOME_ENABLE_Z
static void home_apply_z_reference(void){
    g_status.pos_internal.z_mm_scaled = HOME_Z_ZERO_OFFSET_MM_SCALED;
    g_status.pos_measured.z_mm_scaled = HOME_Z_ZERO_OFFSET_MM_SCALED;
}
#endif

// -----------------------------------------------------------------------------
// Local helpers: Limitzustände
// -----------------------------------------------------------------------------
#if HOME_ENABLE_X
static bool home_x_pressed(void){
    poll_limit_switch();
    return g_status.limits.x_now;
}
#endif

#if HOME_ENABLE_Y
static bool home_y_pressed(void){
    poll_limit_switch();
    return g_status.limits.y_now;
}
#endif

#if HOME_ENABLE_Z
static bool home_z_pressed(void){
    poll_limit_switch();
    return g_status.limits.z_now;
}
#endif


// -----------------------------------------------------------------------------
// Local helpers: Motion-Start pro Homing-Segment
// -----------------------------------------------------------------------------
#if HOME_ENABLE_X
static err_e home_start_x_release(void){
    bot_action_s a = { 0 };
    a.type = ACT_HOME;
    a.target_pos = g_status.pos_internal;
    a.target_pos.x_mm_scaled += HOME_X_RELEASE_MM_SCALED;
    return motion_start(&a, limit_none, &g_home_x_release_profile, MOTION_PROFILE_KIND_HOME);
}

static err_e home_start_x_seek_fast(void){
    bot_action_s a = { 0 };
    a.type = ACT_HOME;
    a.target_pos = g_status.pos_internal;
    a.target_pos.x_mm_scaled -= HOME_X_SEEK_MM_SCALED;
    return motion_start(&a, limit_x, &g_home_x_fast_profile, MOTION_PROFILE_KIND_HOME);
}

static err_e home_start_x_backoff(void){
    bot_action_s a = { 0 };
    a.type = ACT_HOME;
    a.target_pos = g_status.pos_internal;
    a.target_pos.x_mm_scaled += HOME_X_BACKOFF_MM_SCALED;
    return motion_start(&a, limit_none, &g_home_x_backoff_profile, MOTION_PROFILE_KIND_HOME);
}

static err_e home_start_x_seek_slow(void){
    bot_action_s a = { 0 };
    a.type = ACT_HOME;
    a.target_pos = g_status.pos_internal;
    a.target_pos.x_mm_scaled -= HOME_X_SEEK_MM_SCALED;
    return motion_start(&a, limit_x, &g_home_x_slow_profile, MOTION_PROFILE_KIND_HOME);
}
#endif

#if HOME_ENABLE_Y
static err_e home_start_y_release(void){
    bot_action_s a = { 0 };
    a.type = ACT_HOME;
    a.target_pos = g_status.pos_internal;
    a.target_pos.y_mm_scaled += HOME_Y_RELEASE_MM_SCALED;
    return motion_start(&a, limit_none, &g_home_y_release_profile, MOTION_PROFILE_KIND_HOME);
}

static err_e home_start_y_seek_fast(void){
    bot_action_s a = { 0 };
    a.type = ACT_HOME;
    a.target_pos = g_status.pos_internal;
    a.target_pos.y_mm_scaled -= HOME_Y_SEEK_MM_SCALED;
    return motion_start(&a, limit_y, &g_home_y_fast_profile, MOTION_PROFILE_KIND_HOME);
}

static err_e home_start_y_backoff(void){
    bot_action_s a = { 0 };
    a.type = ACT_HOME;
    a.target_pos = g_status.pos_internal;
    a.target_pos.y_mm_scaled += HOME_Y_BACKOFF_MM_SCALED;
    return motion_start(&a, limit_none, &g_home_y_backoff_profile, MOTION_PROFILE_KIND_HOME);
}

static err_e home_start_y_seek_slow(void){
    bot_action_s a = { 0 };
    a.type = ACT_HOME;
    a.target_pos = g_status.pos_internal;
    a.target_pos.y_mm_scaled -= HOME_Y_SEEK_MM_SCALED;
    return motion_start(&a, limit_y, &g_home_y_slow_profile, MOTION_PROFILE_KIND_HOME);
}
#endif

#if HOME_ENABLE_Z
static err_e home_start_z_release(void){
    bot_action_s a = { 0 };
    a.type = ACT_HOME;
    a.target_pos = g_status.pos_internal;
    a.target_pos.z_mm_scaled += HOME_Z_RELEASE_MM_SCALED;
    return motion_start(&a, limit_none, &g_home_z_release_profile, MOTION_PROFILE_KIND_HOME);
}

static err_e home_start_z_seek_fast(void){
    bot_action_s a = { 0 };
    a.type = ACT_HOME;
    a.target_pos = g_status.pos_internal;
    a.target_pos.z_mm_scaled -= HOME_Z_SEEK_MM_SCALED;
    return motion_start(&a, limit_z, &g_home_z_fast_profile, MOTION_PROFILE_KIND_HOME);
}

static err_e home_start_z_backoff(void){
    bot_action_s a = { 0 };
    a.type = ACT_HOME;
    a.target_pos = g_status.pos_internal;
    a.target_pos.z_mm_scaled += HOME_Z_BACKOFF_MM_SCALED;
    return motion_start(&a, limit_none, &g_home_z_backoff_profile, MOTION_PROFILE_KIND_HOME);
}

static err_e home_start_z_seek_slow(void){
    bot_action_s a = { 0 };
    a.type = ACT_HOME;
    a.target_pos = g_status.pos_internal;
    a.target_pos.z_mm_scaled -= HOME_Z_SEEK_MM_SCALED;
    return motion_start(&a, limit_z, &g_home_z_slow_profile, MOTION_PROFILE_KIND_HOME);
}
#endif


// -----------------------------------------------------------------------------
// Local helpers: Start nächste Achse / Abschluss
// -----------------------------------------------------------------------------
#if HOME_ENABLE_Z
static bool home_finish_or_start_next_after_z(err_e* out_err){
#if HOME_ENABLE_X
    err_e me;

    if(home_x_pressed()){
        jh.state = HOME_STATE_X_RELEASE;
        me = home_start_x_release();
    }
    else{
        jh.state = HOME_STATE_X_SEEK_FAST;
        me = home_start_x_seek_fast();
    }

    if(me != ERR_NONE){
        jh.state = HOME_STATE_ERROR;
        if(out_err) *out_err = me;
        return true;
    }
    return false;

#elif HOME_ENABLE_Y
    err_e me;

    if(home_y_pressed()){
        jh.state = HOME_STATE_Y_RELEASE;
        me = home_start_y_release();
    }
    else{
        jh.state = HOME_STATE_Y_SEEK_FAST;
        me = home_start_y_seek_fast();
    }

    if(me != ERR_NONE){
        jh.state = HOME_STATE_ERROR;
        if(out_err) *out_err = me;
        return true;
    }
    return false;

#else
    g_status.homed = true;
    jh.state = HOME_STATE_DONE;
    if(out_err) *out_err = ERR_NONE;
    return true;
#endif
}
#endif
#if HOME_ENABLE_X
static bool home_finish_or_start_next_after_x(err_e* out_err){
#if HOME_ENABLE_Y
    err_e me;

    if(home_y_pressed()){
        jh.state = HOME_STATE_Y_RELEASE;
        me = home_start_y_release();
    }
    else{
        jh.state = HOME_STATE_Y_SEEK_FAST;
        me = home_start_y_seek_fast();
    }

    if(me != ERR_NONE){
        jh.state = HOME_STATE_ERROR;
        if(out_err) *out_err = me;
        return true;
    }
    return false;

#else
    g_status.homed = true;
    jh.state = HOME_STATE_DONE;
    if(out_err) *out_err = ERR_NONE;
    return true;
#endif
}
#endif

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------
err_e job_home_start(const bot_action_s* a){
    (void)a;

    jh.state = HOME_STATE_IDLE;

#if HOME_ENABLE_Z
    if(home_z_pressed()){
        jh.state = HOME_STATE_Z_RELEASE;
        return home_start_z_release();
    }
    else{
        jh.state = HOME_STATE_Z_SEEK_FAST;
        return home_start_z_seek_fast();
    }

#elif HOME_ENABLE_X
    if(home_x_pressed()){
        jh.state = HOME_STATE_X_RELEASE;
        return home_start_x_release();
    }
    else{
        jh.state = HOME_STATE_X_SEEK_FAST;
        return home_start_x_seek_fast();
    }

#elif HOME_ENABLE_Y
    if(home_y_pressed()){
        jh.state = HOME_STATE_Y_RELEASE;
        return home_start_y_release();
    }
    else{
        jh.state = HOME_STATE_Y_SEEK_FAST;
        return home_start_y_seek_fast();
    }

#else
    g_status.homed = true;
    jh.state = HOME_STATE_DONE;
    return ERR_NONE;
#endif
}

bool job_home_step(err_e* out_err){
    if(!motion_is_done()){
        return false;
    }

    err_e me = motion_last_err();
    if(me != ERR_NONE){
        jh.state = HOME_STATE_ERROR;
        if(out_err) *out_err = me;
        return true;
    }

    switch(jh.state){

#if HOME_ENABLE_Z
    case HOME_STATE_Z_RELEASE:
        if(home_z_pressed()){
            me = home_start_z_release();
            if(me != ERR_NONE){
                jh.state = HOME_STATE_ERROR;
                if(out_err) *out_err = me;
                return true;
            }
            return false;
        }

        jh.state = HOME_STATE_Z_SEEK_FAST;
        me = home_start_z_seek_fast();
        if(me != ERR_NONE){
            jh.state = HOME_STATE_ERROR;
            if(out_err) *out_err = me;
            return true;
        }
        return false;

    case HOME_STATE_Z_SEEK_FAST:
        if(!motion_stopped_by_limit() || !(motion_limit_hit() & limit_z)){
            jh.state = HOME_STATE_ERROR;
            if(out_err) *out_err = ERR_MOTOR;
            return true;
        }

        jh.state = HOME_STATE_Z_BACKOFF;
        me = home_start_z_backoff();
        if(me != ERR_NONE){
            jh.state = HOME_STATE_ERROR;
            if(out_err) *out_err = me;
            return true;
        }
        return false;

    case HOME_STATE_Z_BACKOFF:
        if(home_z_pressed()){
            me = home_start_z_backoff();
            if(me != ERR_NONE){
                jh.state = HOME_STATE_ERROR;
                if(out_err) *out_err = me;
                return true;
            }
            return false;
        }

        jh.state = HOME_STATE_Z_SEEK_SLOW;
        me = home_start_z_seek_slow();
        if(me != ERR_NONE){
            jh.state = HOME_STATE_ERROR;
            if(out_err) *out_err = me;
            return true;
        }
        return false;

    case HOME_STATE_Z_SEEK_SLOW:
        if(!motion_stopped_by_limit() || !(motion_limit_hit() & limit_z)){
            jh.state = HOME_STATE_ERROR;
            if(out_err) *out_err = ERR_MOTOR;
            return true;
        }

        home_apply_z_reference();
        return home_finish_or_start_next_after_z(out_err);
#endif

#if HOME_ENABLE_X
    case HOME_STATE_X_RELEASE:
        if(home_x_pressed()){
            me = home_start_x_release();
            if(me != ERR_NONE){
                jh.state = HOME_STATE_ERROR;
                if(out_err) *out_err = me;
                return true;
            }
            return false;
        }

        jh.state = HOME_STATE_X_SEEK_FAST;
        me = home_start_x_seek_fast();
        if(me != ERR_NONE){
            jh.state = HOME_STATE_ERROR;
            if(out_err) *out_err = me;
            return true;
        }
        return false;

    case HOME_STATE_X_SEEK_FAST:
        if(!motion_stopped_by_limit() || !(motion_limit_hit() & limit_x)){
            jh.state = HOME_STATE_ERROR;
            if(out_err) *out_err = ERR_MOTOR;
            return true;
        }

        jh.state = HOME_STATE_X_BACKOFF;
        me = home_start_x_backoff();
        if(me != ERR_NONE){
            jh.state = HOME_STATE_ERROR;
            if(out_err) *out_err = me;
            return true;
        }
        return false;

    case HOME_STATE_X_BACKOFF:
        if(home_x_pressed()){
            me = home_start_x_backoff();
            if(me != ERR_NONE){
                jh.state = HOME_STATE_ERROR;
                if(out_err) *out_err = me;
                return true;
            }
            return false;
        }

        jh.state = HOME_STATE_X_SEEK_SLOW;
        me = home_start_x_seek_slow();
        if(me != ERR_NONE){
            jh.state = HOME_STATE_ERROR;
            if(out_err) *out_err = me;
            return true;
        }
        return false;

    case HOME_STATE_X_SEEK_SLOW:
        if(!motion_stopped_by_limit() || !(motion_limit_hit() & limit_x)){
            jh.state = HOME_STATE_ERROR;
            if(out_err) *out_err = ERR_MOTOR;
            return true;
        }

        home_apply_x_reference();
        return home_finish_or_start_next_after_x(out_err);
#endif

#if HOME_ENABLE_Y
    case HOME_STATE_Y_RELEASE:
        if(home_y_pressed()){
            me = home_start_y_release();
            if(me != ERR_NONE){
                jh.state = HOME_STATE_ERROR;
                if(out_err) *out_err = me;
                return true;
            }
            return false;
        }

        jh.state = HOME_STATE_Y_SEEK_FAST;
        me = home_start_y_seek_fast();
        if(me != ERR_NONE){
            jh.state = HOME_STATE_ERROR;
            if(out_err) *out_err = me;
            return true;
        }
        return false;

    case HOME_STATE_Y_SEEK_FAST:
        if(!motion_stopped_by_limit() || !(motion_limit_hit() & limit_y)){
            jh.state = HOME_STATE_ERROR;
            if(out_err) *out_err = ERR_MOTOR;
            return true;
        }

        jh.state = HOME_STATE_Y_BACKOFF;
        me = home_start_y_backoff();
        if(me != ERR_NONE){
            jh.state = HOME_STATE_ERROR;
            if(out_err) *out_err = me;
            return true;
        }
        return false;

    case HOME_STATE_Y_BACKOFF:
        if(home_y_pressed()){
            me = home_start_y_backoff();
            if(me != ERR_NONE){
                jh.state = HOME_STATE_ERROR;
                if(out_err) *out_err = me;
                return true;
            }
            return false;
        }

        jh.state = HOME_STATE_Y_SEEK_SLOW;
        me = home_start_y_seek_slow();
        if(me != ERR_NONE){
            jh.state = HOME_STATE_ERROR;
            if(out_err) *out_err = me;
            return true;
        }
        return false;

    case HOME_STATE_Y_SEEK_SLOW:
        if(!motion_stopped_by_limit() || !(motion_limit_hit() & limit_y)){
            jh.state = HOME_STATE_ERROR;
            if(out_err) *out_err = ERR_MOTOR;
            return true;
        }

        home_apply_y_reference();

        g_status.homed = true;
        jh.state = HOME_STATE_DONE;
        if(out_err) *out_err = ERR_NONE;
        return true;
#endif

    case HOME_STATE_DONE:
        if(out_err) *out_err = ERR_NONE;
        return true;

    case HOME_STATE_IDLE:
    case HOME_STATE_ERROR:
    default:
        if(out_err) *out_err = ERR_INTERNAL;
        return true;
    }
}
