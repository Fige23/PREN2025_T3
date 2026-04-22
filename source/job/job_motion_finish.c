/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_))
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|
job_motion_finish.c	Created on: 17.03.2026	   Author: Fige23	Team 3
*/
#include "job_motion_finish.h"

#include "motion.h"
#include "protocol.h"
#include "robot_config.h"
#include "position.h"
#include "debug.h"
#include "io.h"

#if POSITION_ENABLE

static inline int32_t iabs32(int32_t v){
    return (v < 0) ? -v : v;
}

static inline int32_t sign32(int32_t v){
    return (v < 0) ? -1 : +1;
}

static int32_t div_round_s64(int64_t num, int64_t den){
    if(den == 0){
        return 0;
    }

    if(num >= 0){
        return (int32_t)((num + den / 2) / den);
    }
    else{
        return (int32_t)((num - den / 2) / den);
    }
}

static int32_t clamp_i32(int32_t v, int32_t lo, int32_t hi){
    if(v < lo){
        return lo;
    }
    if(v > hi){
        return hi;
    }
    return v;
}

static int64_t err_abs_sum_s64(int32_t ex, int32_t ey){
    return (int64_t)iabs32(ex) + (int64_t)iabs32(ey);
}

static const motion_profile_s g_pos_corr_profile = {
    POS_CORR_START_STEP_RATE_SPS,
    POS_CORR_MAX_STEP_RATE_SPS,
    POS_CORR_ACCEL_SPS2
};

static int32_t calc_min_corr_step_scaled(uint32_t steps_per_mm_q1000){
    int32_t v = div_round_s64(
        (int64_t)SCALE_MM * 1000LL,
        (int64_t)steps_per_mm_q1000);

    if(v < 1){
        v = 1;
    }

    if(v > (int32_t)POS_CORR_MAX_STEP_SCALED){
        v = (int32_t)POS_CORR_MAX_STEP_SCALED;
    }

    return v;
}

static int32_t calc_axis_corr_cmd(int32_t err_scaled,
    int32_t tol_scaled,
    int32_t min_step_scaled){
    int32_t c;

    if(iabs32(err_scaled) <= tol_scaled){
        return 0;
    }

    c = div_round_s64(
        (int64_t)err_scaled * (int64_t)POS_CORR_P_ZAEHLER,
        (int64_t)POS_CORR_P_NENNER);

    c = clamp_i32(c,
        -(int32_t)POS_CORR_MAX_STEP_SCALED,
        (int32_t)POS_CORR_MAX_STEP_SCALED);

    if(iabs32(c) < min_step_scaled){
        c = sign32(err_scaled) * min_step_scaled;
    }

    return c;
}

#endif /* POSITION_ENABLE */

static void set_internal_pos_exact(const robot_pos_s* p){
    if(p == 0){
        return;
    }

    g_status.pos_internal.x_mm_scaled = p->x_mm_scaled;
    g_status.pos_internal.y_mm_scaled = p->y_mm_scaled;
    g_status.pos_internal.z_mm_scaled = p->z_mm_scaled;
    g_status.pos_internal.phi_deg_scaled = p->phi_deg_scaled;
}

void job_motion_finish_init(job_motion_finish_s* ctx, const robot_pos_s* final_target){
    if(ctx == 0 || final_target == 0){
        return;
    }

    ctx->final_target = *final_target;
    ctx->corr_iter = 0u;

    ctx->prev_ex = 0;
    ctx->prev_ey = 0;
    ctx->no_progress_count = 0u;
    ctx->prev_valid = false;
}

bool job_motion_finish_step(job_motion_finish_s* ctx, err_e* out_err){
    if(ctx == 0){
        if(out_err){
            *out_err = ERR_INTERNAL;
        }
        return true;
    }

    /* Solange Motion noch läuft: noch nicht fertig */
    if(!motion_is_done()){
        return false;
    }

    /* Erst prüfen, ob die Motion selbst mit Fehler beendet wurde */
    err_e me = motion_last_err();
    if(me != ERR_NONE){
        debug_printf("CORR motion finished with motion error=%d\r\n", (int)me);
        if(out_err){
            *out_err = me;
        }
        return true;
    }

#if (POSITION_ENABLE && POSITION_CLOSED_LOOP_ENABLE)

    /* Bei normalem Zielanfahren darf ein Limit nicht stillschweigend als OK gelten */
    if(motion_stopped_by_limit()){
        debug_printf("CORR abort: unexpected limit during finish/correction\r\n");
        if(out_err){
            *out_err = ERR_UNEXPECTED_LIMIT;
        }
        return true;
    }

    /* Frische Messwerte holen */
    position_poll();

    /* Sanity check: interne Soll-Welt vs. gemessene Welt */
    int32_t dix = g_status.pos_internal.x_mm_scaled - g_status.pos_measured.x_mm_scaled;
    int32_t diy = g_status.pos_internal.y_mm_scaled - g_status.pos_measured.y_mm_scaled;

    bool warn_mismatch_x = (iabs32(dix) > POS_INT_MEAS_WARN_X_SCALED);
    bool warn_mismatch_y = (iabs32(diy) > POS_INT_MEAS_WARN_Y_SCALED);
    bool abort_mismatch_x = (iabs32(dix) > POS_INT_MEAS_ABORT_X_SCALED);
    bool abort_mismatch_y = (iabs32(diy) > POS_INT_MEAS_ABORT_Y_SCALED);

    if(warn_mismatch_x || warn_mismatch_y){
        debug_printf(
            "CORR warn: int/meas mismatch int=(%ld,%ld) meas=(%ld,%ld) delta=(%ld,%ld)\r\n",
            (long)g_status.pos_internal.x_mm_scaled,
            (long)g_status.pos_internal.y_mm_scaled,
            (long)g_status.pos_measured.x_mm_scaled,
            (long)g_status.pos_measured.y_mm_scaled,
            (long)dix,
            (long)diy);
    }

    if(abort_mismatch_x || abort_mismatch_y){
        debug_printf(
            "CORR abort: int/meas mismatch too large int=(%ld,%ld) meas=(%ld,%ld) delta=(%ld,%ld)\r\n",
            (long)g_status.pos_internal.x_mm_scaled,
            (long)g_status.pos_internal.y_mm_scaled,
            (long)g_status.pos_measured.x_mm_scaled,
            (long)g_status.pos_measured.y_mm_scaled,
            (long)dix,
            (long)diy);

        if(out_err){
            *out_err = ERR_ENCODER_POSITION_MISMATCH;
        }
        return true;
    }

    /* Fehler zum finalen Ziel in der gemessenen Welt */
    int32_t ex = ctx->final_target.x_mm_scaled - g_status.pos_measured.x_mm_scaled;
    int32_t ey = ctx->final_target.y_mm_scaled - g_status.pos_measured.y_mm_scaled;

    bool okx = (iabs32(ex) <= POS_TOL_X_SCALED);
    bool oky = (iabs32(ey) <= POS_TOL_Y_SCALED);

    debug_printf(
        "CORR state iter=%u tgt=(%ld,%ld) int=(%ld,%ld) meas=(%ld,%ld) err=(%ld,%ld) ok=(%u,%u)\r\n",
        (unsigned)ctx->corr_iter,
        (long)ctx->final_target.x_mm_scaled,
        (long)ctx->final_target.y_mm_scaled,
        (long)g_status.pos_internal.x_mm_scaled,
        (long)g_status.pos_internal.y_mm_scaled,
        (long)g_status.pos_measured.x_mm_scaled,
        (long)g_status.pos_measured.y_mm_scaled,
        (long)ex,
        (long)ey,
        (unsigned)okx,
        (unsigned)oky);

    if(okx && oky){
        debug_printf("CORR success: target reached within tolerance\r\n");

        /* commanded/internal sauber exakt auf Ziel setzen */
        set_internal_pos_exact(&ctx->final_target);

        if(out_err){
            *out_err = ERR_NONE;
        }
        return true;
    }

    /*
     * Fortschritt prüfen über den Gesamtfehler |ex| + |ey|.
     * So vermeiden wir, dass eine Achse etwas besser wird,
     * während die andere deutlich schlechter wird, und das
     * trotzdem fälschlich als "Fortschritt" gilt.
     */
    if(ctx->prev_valid){
        int64_t prev_abs_sum = err_abs_sum_s64(ctx->prev_ex, ctx->prev_ey);
        int64_t curr_abs_sum = err_abs_sum_s64(ex, ey);

        bool total_progress =
            (prev_abs_sum - curr_abs_sum) >= (int64_t)POS_CORR_PROGRESS_MIN_DELTA_SCALED;

        bool total_worse =
            (curr_abs_sum - prev_abs_sum) >= (int64_t)POS_CORR_PROGRESS_MIN_DELTA_SCALED;

        if(total_progress){
            if(ctx->no_progress_count != 0u){
                debug_printf(
                    "CORR progress recovered prev_sum=%ld curr_sum=%ld prev_err=(%ld,%ld) curr_err=(%ld,%ld)\r\n",
                    (long)prev_abs_sum,
                    (long)curr_abs_sum,
                    (long)ctx->prev_ex,
                    (long)ctx->prev_ey,
                    (long)ex,
                    (long)ey);
            }
            ctx->no_progress_count = 0u;
        }
        else{
            ctx->no_progress_count++;

            debug_printf(
                "CORR no progress count=%u prev_sum=%ld curr_sum=%ld prev_err=(%ld,%ld) curr_err=(%ld,%ld) worse=%u\r\n",
                (unsigned)ctx->no_progress_count,
                (long)prev_abs_sum,
                (long)curr_abs_sum,
                (long)ctx->prev_ex,
                (long)ctx->prev_ey,
                (long)ex,
                (long)ey,
                (unsigned)total_worse);

            if(ctx->no_progress_count >= POS_CORR_NO_PROGRESS_LIMIT){
                debug_printf(
                    "CORR abort: no measurable total progress after %u correction cycles\r\n",
                    (unsigned)ctx->no_progress_count);
                if(out_err){
                    *out_err = ERR_ENCODER_NO_PROGRESS;
                }
                return true;
            }
        }
    }

    if(ctx->corr_iter >= POS_CORR_MAX_ITERATIONS){
        debug_printf(
            "CORR abort: max iterations reached (%u), remaining err=(%ld,%ld)\r\n",
            (unsigned)POS_CORR_MAX_ITERATIONS,
            (long)ex,
            (long)ey);
        if(out_err){
            *out_err = ERR_ENCODER_MAX_ITERATIONS;
        }
        return true;
    }

    /* Kleinste sinnvolle Korrektur pro Achse, ungefähr 1 Motorschritt */
    int32_t min_x = calc_min_corr_step_scaled(STEPS_PER_MM_X_Q1000);
    int32_t min_y = calc_min_corr_step_scaled(STEPS_PER_MM_Y_Q1000);

    /*
     * Nur Achsen korrigieren, die wirklich ausserhalb der Toleranz sind.
     * Eine bereits gute Achse bleibt auf 0 Korrektur.
     */
    int32_t cx = calc_axis_corr_cmd(ex, POS_TOL_X_SCALED, min_x);
    int32_t cy = calc_axis_corr_cmd(ey, POS_TOL_Y_SCALED, min_y);

    debug_printf(
        "CORR plan min_step=(%ld,%ld) cmd=(%ld,%ld)\r\n",
        (long)min_x,
        (long)min_y,
        (long)cx,
        (long)cy);

    /*
     * Sollte praktisch nicht mehr passieren, aber falls Konfiguration oder
     * Rundung komisch sind, klar abbrechen statt sinnlos zu loopen.
     */
    if(cx == 0 && cy == 0){
        debug_printf(
            "CORR abort: correction command became zero while still out of tolerance, err=(%ld,%ld)\r\n",
            (long)ex,
            (long)ey);
        if(out_err){
            *out_err = ERR_ENCODER_NO_PROGRESS;
        }
        return true;
    }

    /* Neues relatives Korrektursegment auf Basis der commanded/internal Welt */
    bot_action_s corr = { 0 };
    corr.type = ACT_MOVE;
    corr.target_pos = g_status.pos_internal;

    corr.target_pos.x_mm_scaled += cx;
    corr.target_pos.y_mm_scaled += cy;

    /* Z und Phi am finalen Soll halten */
    corr.target_pos.z_mm_scaled = ctx->final_target.z_mm_scaled;
    corr.target_pos.phi_deg_scaled = ctx->final_target.phi_deg_scaled;

    debug_printf(
        "CORR start iter=%u new_target=(%ld,%ld,%ld,%ld)\r\n",
        (unsigned)(ctx->corr_iter + 1u),
        (long)corr.target_pos.x_mm_scaled,
        (long)corr.target_pos.y_mm_scaled,
        (long)corr.target_pos.z_mm_scaled,
        (long)corr.target_pos.phi_deg_scaled);

    err_e e = motion_start(&corr, limit_none, &g_pos_corr_profile, MOTION_PROFILE_KIND_CORR);
    if(e != ERR_NONE){
        debug_printf("CORR abort: motion_start for correction failed with err=%d\r\n", (int)e);
        if(out_err){
            *out_err = e;
        }
        return true;
    }

    ctx->corr_iter++;
    ctx->prev_ex = ex;
    ctx->prev_ey = ey;
    ctx->prev_valid = true;

    return false;

#else
    /* Ohne closed-loop ist nach sauberem Motion-Ende alles fertig */
    set_internal_pos_exact(&ctx->final_target);

    if(out_err){
        *out_err = ERR_NONE;
    }

    return true;
#endif
}
