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

#include "motion.h"
#include "protocol.h"
#include "robot_config.h"

#ifndef POSITION_ENABLE
#define POSITION_ENABLE 0
#endif
#ifndef POSITION_CLOSED_LOOP_ENABLE
#define POSITION_CLOSED_LOOP_ENABLE 0
#endif

#if POSITION_ENABLE
#include "position/position.h"
#endif

// Defaults, falls Makros nicht gesetzt
#ifndef POS_TOL_X_SCALED
#define POS_TOL_X_SCALED  (20)   // 0.020 mm
#endif
#ifndef POS_TOL_Y_SCALED
#define POS_TOL_Y_SCALED  (20)
#endif
#ifndef POS_CORR_P_ZAEHLER
#define POS_CORR_P_ZAEHLER     1
#endif
#ifndef POS_CORR_P_NENNER
#define POS_CORR_P_NENNER     1
#endif
#ifndef POS_CORR_MAX_STEP_SCALED
#define POS_CORR_MAX_STEP_SCALED  (2000) // 2.0 mm
#endif
#ifndef POS_CORR_MAX_ITERATIONS
#define POS_CORR_MAX_ITERATIONS  6
#endif

static inline int32_t iabs32(int32_t v) { return (v < 0) ? -v : v; }
static inline int32_t sign32(int32_t v) { return (v < 0) ? -1 : +1; }

static int32_t div_round_s64(int64_t num, int64_t den)
{
    if (den == 0) return 0;
    if (num >= 0) return (int32_t)((num + den/2) / den);
    else          return (int32_t)((num - den/2) / den);
}

static int32_t clamp_i32(int32_t v, int32_t lo, int32_t hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

typedef struct {
    bool active;
    err_e last_err;

    // Move sequencing
    robot_pos_s final_target;
    uint8_t corr_iter;
} job_s;

static job_s j = {0};

void job_init(void)
{
    j.active = false;
    j.last_err = ERR_NONE;
    j.corr_iter = 0;
}

bool job_is_active(void)
{
    return j.active;
}

err_e job_start_move(const bot_action_s *a)
{
    if (j.active) return ERR_INTERNAL;

    j.last_err = ERR_NONE;
    j.final_target = a->target_pos;
    j.corr_iter = 0;

    // Start: 1. Segment direkt auf final_target (open-loop)
    err_e e = motion_start(a);
    if (e != ERR_NONE) {
        j.active = false;
        j.last_err = e;
        return e;
    }

    j.active = true;
    return ERR_NONE;
}

bool job_step(err_e *out_err)
{
    if (!j.active) {
        if (out_err) *out_err = j.last_err;
        return true;
    }

    if (!motion_is_done()) {
        return false;
    }

    // Segment fertig -> Fehler prüfen
    err_e me = motion_last_err();
    if (me != ERR_NONE) {
        j.last_err = me;
        j.active = false;
        if (out_err) *out_err = j.last_err;
        return true;
    }

#if (POSITION_ENABLE && POSITION_CLOSED_LOOP_ENABLE)
    // Messung aktualisieren (wichtig: direkt vor Error-Berechnung)
    position_poll();

    int32_t ex = j.final_target.x_mm_scaled - g_status.pos_measured.x_mm_scaled;
    int32_t ey = j.final_target.y_mm_scaled - g_status.pos_measured.y_mm_scaled;

    bool okx = (iabs32(ex) <= POS_TOL_X_SCALED);
    bool oky = (iabs32(ey) <= POS_TOL_Y_SCALED);

    if (okx && oky) {
        // Ziel erreicht (innerhalb Toleranz) -> Job fertig
        // Optional: pos_cmd exakt auf Soll setzen (saubere "Soll-Welt")
        g_status.pos_cmd.x_mm_scaled = j.final_target.x_mm_scaled;
        g_status.pos_cmd.y_mm_scaled = j.final_target.y_mm_scaled;
        g_status.pos_cmd.z_mm_scaled = j.final_target.z_mm_scaled;
        g_status.pos_cmd.phi_deg_scaled = j.final_target.phi_deg_scaled;

        j.last_err = ERR_NONE;
        j.active = false;
        if (out_err) *out_err = j.last_err;
        return true;
    }

    if (j.corr_iter >= POS_CORR_MAX_ITERATIONS) {
        // Konnte nicht sauber treffen
        j.last_err = ERR_MOTOR;
        j.active = false;
        if (out_err) *out_err = j.last_err;
        return true;
    }

    // --- Korrektursegment planen ---
    // P-Korrektur: corr = P * error
    int32_t cx = div_round_s64((int64_t)ex * (int64_t)POS_CORR_P_ZAEHLER, (int64_t)POS_CORR_P_NENNER);
    int32_t cy = div_round_s64((int64_t)ey * (int64_t)POS_CORR_P_ZAEHLER, (int64_t)POS_CORR_P_NENNER);

    // Clamp max step
    cx = clamp_i32(cx, -(int32_t)POS_CORR_MAX_STEP_SCALED, (int32_t)POS_CORR_MAX_STEP_SCALED);
    cy = clamp_i32(cy, -(int32_t)POS_CORR_MAX_STEP_SCALED, (int32_t)POS_CORR_MAX_STEP_SCALED);

    // Mindestkorrektur: mindestens 1 Step (sonst kann’s wegen Rundung bei steps=0 hängen bleiben)
    // 1 Step in mm_scaled: SCALE_MM / STEPS_PER_MM_*
    int32_t min_x = div_round_s64((int64_t)SCALE_MM, (int64_t)STEPS_PER_MM_X);
    int32_t min_y = div_round_s64((int64_t)SCALE_MM, (int64_t)STEPS_PER_MM_Y);

    if (iabs32(ex) > POS_TOL_X_SCALED && iabs32(cx) < min_x) cx = sign32(ex) * min_x;
    if (iabs32(ey) > POS_TOL_Y_SCALED && iabs32(cy) < min_y) cy = sign32(ey) * min_y;

    // Wir bewegen relativ zur "commanded Welt".
    // pos_cmd sagt "wir sind schon am Soll", aber measured sagt "nein".
    // => wir müssen command weiter verschieben, damit physisch nachgezogen wird.
    bot_action_s corr = {0};
    corr.type = ACT_MOVE;
    corr.target_pos = g_status.pos_cmd; // Startpunkt für Delta ist pos_cmd in motion_start()
    corr.target_pos.x_mm_scaled += cx;
    corr.target_pos.y_mm_scaled += cy;

    // Z/Phi auf final halten
    corr.target_pos.z_mm_scaled = j.final_target.z_mm_scaled;
    corr.target_pos.phi_deg_scaled = j.final_target.phi_deg_scaled;

    err_e e = motion_start(&corr);
    if (e != ERR_NONE) {
        j.last_err = e;
        j.active = false;
        if (out_err) *out_err = j.last_err;
        return true;
    }

    j.corr_iter++;
    return false;

#else
    // Ohne closed-loop: Job ist nach 1 Segment fertig
    j.last_err = ERR_NONE;
    j.active = false;
    if (out_err) *out_err = j.last_err;
    return true;
#endif
}
