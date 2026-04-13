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

#if POSITION_ENABLE

static inline int32_t iabs32(int32_t v)
{
    return (v < 0) ? -v : v;
}

static inline int32_t sign32(int32_t v)
{
    return (v < 0) ? -1 : +1;
}

static int32_t div_round_s64(int64_t num, int64_t den)
{
    if (den == 0) return 0;

    if (num >= 0) {
        return (int32_t)((num + den / 2) / den);
    } else {
        return (int32_t)((num - den / 2) / den);
    }
}

static int32_t clamp_i32(int32_t v, int32_t lo, int32_t hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}
#endif

static void set_internal_pos_exact(const robot_pos_s *p)
{
    if (p == 0) return;

    g_status.pos_internal.x_mm_scaled    = p->x_mm_scaled;
    g_status.pos_internal.y_mm_scaled    = p->y_mm_scaled;
    g_status.pos_internal.z_mm_scaled    = p->z_mm_scaled;
    g_status.pos_internal.phi_deg_scaled = p->phi_deg_scaled;
}

void job_motion_finish_init(job_motion_finish_s *ctx, const robot_pos_s *final_target)
{
    if (ctx == 0 || final_target == 0) return;

    ctx->final_target = *final_target;
    ctx->corr_iter = 0;
}

bool job_motion_finish_step(job_motion_finish_s *ctx, err_e *out_err)
{
    if (ctx == 0) {
        if (out_err) *out_err = ERR_INTERNAL;
        return true;
    }

    // Solange Motion noch läuft: nicht fertig
    if (!motion_is_done()) {
        return false;
    }

    // Erst prüfen, ob die Motion selbst mit Fehler beendet wurde
    err_e me = motion_last_err();
    if (me != ERR_NONE) {
        if (out_err) *out_err = me;
        return true;
    }

#if (POSITION_ENABLE && POSITION_CLOSED_LOOP_ENABLE)

    // Sicherheitscheck:
    // Für normale präzise Zielbewegungen darf hier kein Limit-Abbruch als "OK" durchgehen.
    if (motion_stopped_by_limit()) {
        if (out_err) *out_err = ERR_MOTOR;
        return true;
    }

    // Messung direkt vor der Fehlerberechnung aktualisieren
    position_poll();

    int32_t ex = ctx->final_target.x_mm_scaled - g_status.pos_measured.x_mm_scaled;
    int32_t ey = ctx->final_target.y_mm_scaled - g_status.pos_measured.y_mm_scaled;

    bool okx = (iabs32(ex) <= POS_TOL_X_SCALED);
    bool oky = (iabs32(ey) <= POS_TOL_Y_SCALED);

    if (okx && oky) {
        // Ziel innerhalb Toleranz erreicht.
        // Interne Soll-Welt sauber exakt auf final_target setzen.
        set_internal_pos_exact(&ctx->final_target);

        if (out_err) *out_err = ERR_NONE;
        return true;
    }

    if (ctx->corr_iter >= POS_CORR_MAX_ITERATIONS) {
        if (out_err) *out_err = ERR_NOT_IMPLEMENTED;
        return true;
    }

    // --- Korrektursegment planen ---
    // P-Korrektur: corr = P * error
    int32_t cx = div_round_s64(
        (int64_t)ex * (int64_t)POS_CORR_P_ZAEHLER,
        (int64_t)POS_CORR_P_NENNER
    );

    int32_t cy = div_round_s64(
        (int64_t)ey * (int64_t)POS_CORR_P_ZAEHLER,
        (int64_t)POS_CORR_P_NENNER
    );

    // Maximalen Korrekturschritt begrenzen
    cx = clamp_i32(cx,
                   -(int32_t)POS_CORR_MAX_STEP_SCALED,
                    (int32_t)POS_CORR_MAX_STEP_SCALED);

    cy = clamp_i32(cy,
                   -(int32_t)POS_CORR_MAX_STEP_SCALED,
                    (int32_t)POS_CORR_MAX_STEP_SCALED);

    // Mindestkorrektur:
    // Wenn Error ausserhalb Toleranz liegt, Korrektur aber wegen Rundung zu klein wird,
    // erzwingen wir mindestens ca. 1 Schritt in physikalischer Einheit.
    int32_t min_x = div_round_s64(
        (int64_t)SCALE_MM * 1000LL,
        (int64_t)STEPS_PER_MM_X_Q1000
    );

    int32_t min_y = div_round_s64(
        (int64_t)SCALE_MM * 1000LL,
        (int64_t)STEPS_PER_MM_Y_Q1000
    );

    if (iabs32(ex) > POS_TOL_X_SCALED && iabs32(cx) < min_x) {
        cx = sign32(ex) * min_x;
    }

    if (iabs32(ey) > POS_TOL_Y_SCALED && iabs32(cy) < min_y) {
        cy = sign32(ey) * min_y;
    }

    // Neues relatives Korrektursegment auf Basis der commanded/internal Welt
    bot_action_s corr = {0};
    corr.type = ACT_MOVE;
    corr.target_pos = g_status.pos_internal;

    corr.target_pos.x_mm_scaled += cx;
    corr.target_pos.y_mm_scaled += cy;

    // Z und Phi auf finalem Soll halten
    corr.target_pos.z_mm_scaled    = ctx->final_target.z_mm_scaled;
    corr.target_pos.phi_deg_scaled = ctx->final_target.phi_deg_scaled;

    err_e e = motion_start(&corr, limit_none, 0);
    if (e != ERR_NONE) {
        if (out_err) *out_err = e;
        return true;
    }

    ctx->corr_iter++;
    return false;

#else
    // Ohne closed-loop ist die Bewegung nach einem sauberen Motion-Ende fertig
    set_internal_pos_exact(&ctx->final_target);

    if (out_err) *out_err = ERR_NONE;
    return true;
#endif
}
