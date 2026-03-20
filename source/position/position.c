/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
position.c	Created on: 25.02.2026	   Author: Fige23	Team 3                                                                
*/

/*
 *
 * MAGNET UMKONFIGURIEREN VON PTA1 auf PTA13
 *
 *
 */






#include "position.h"
#include "robot_config.h"

#ifndef POSITION_ENABLE
#define POSITION_ENABLE 0
#endif

#if POSITION_ENABLE

#include "fsl_ftm.h"
#include "fsl_clock.h"

#ifndef ENC_X_COUNTS_PER_MM
#define ENC_X_COUNTS_PER_MM 512u
#endif
#ifndef ENC_Y_COUNTS_PER_MM
#define ENC_Y_COUNTS_PER_MM 512u
#endif
#ifndef ENC_X_INVERT
#define ENC_X_INVERT 0
#endif
#ifndef ENC_Y_INVERT
#define ENC_Y_INVERT 0
#endif

typedef struct {
    FTM_Type *ftm;
    uint16_t last_cnt;
    int32_t  acc_counts;      // integrierte Counts (wrap-safe)
    int32_t  offset_counts;   // Koordinaten-Offset (Nullpunkt)
    uint32_t counts_per_mm;
    uint32_t counter_span;    // echter Hardware-Zählbereich
    bool     invert;
} enc_axis_s;

static enc_axis_s ex;
static enc_axis_s ey;

static inline int32_t iabs32(int32_t v) { return (v < 0) ? -v : v; }

static int32_t div_round_s64(int64_t num, int64_t den)
{
    if (den == 0) return 0;
    if (num >= 0) return (int32_t)((num + den/2) / den);
    else          return (int32_t)((num - den/2) / den);
}

static int32_t counts_to_mm_scaled(int32_t counts, uint32_t counts_per_mm)
{
    // mm_scaled = counts * SCALE_MM / counts_per_mm
    int64_t num = (int64_t)counts * (int64_t)SCALE_MM;
    return div_round_s64(num, (int64_t)counts_per_mm);
}

static int32_t mm_scaled_to_counts(int32_t mm_scaled, uint32_t counts_per_mm)
{
    // counts = mm_scaled * counts_per_mm / SCALE_MM
    int64_t num = (int64_t)mm_scaled * (int64_t)counts_per_mm;
    return div_round_s64(num, (int64_t)SCALE_MM);
}

static void enc_update(enc_axis_s *e)
{
    uint16_t now = (uint16_t)FTM_GetQuadDecoderCounterValue(e->ftm);

    int32_t diff = (int32_t)now - (int32_t)e->last_cnt;
    e->last_cnt = now;

    int32_t span = (int32_t)e->counter_span;
    int32_t half = span / 2;

    if (diff > half) {
        diff -= span;
    } else if (diff < -half) {
        diff += span;
    }

    if (e->invert) {
        diff = -diff;
    }

    e->acc_counts += diff;
}

static void enc_hw_init(enc_axis_s *e, FTM_Type *ftm)
{
    e->ftm = ftm;
    e->last_cnt = (uint16_t)FTM_GetQuadDecoderCounterValue(ftm);
    e->acc_counts = 0;
    e->offset_counts = 0;

    e->counter_span = (uint32_t)ftm->MOD - (uint32_t)ftm->CNTIN + 1u;
    if (e->counter_span == 0u) {
        e->counter_span = 65536u;
    }
}

void position_init(void)
{
    ex.counts_per_mm = ENC_X_COUNTS_PER_MM;
    ex.invert        = (ENC_X_INVERT != 0);
    enc_hw_init(&ex, FTM1);

    ey.counts_per_mm = ENC_Y_COUNTS_PER_MM;
    ey.invert        = (ENC_Y_INVERT != 0);
    enc_hw_init(&ey, FTM2);

    position_sync_measured_to_internal();
}

void position_poll(void)
{
    enc_update(&ex);
    enc_update(&ey);

    int32_t x_counts = ex.acc_counts + ex.offset_counts;
    int32_t y_counts = ey.acc_counts + ey.offset_counts;

    g_status.pos_measured.x_mm_scaled = counts_to_mm_scaled(x_counts, ex.counts_per_mm);
    g_status.pos_measured.y_mm_scaled = counts_to_mm_scaled(y_counts, ey.counts_per_mm);

    // Z/Phi vorerst "unge-messen" -> aus cmd übernehmen
    g_status.pos_measured.z_mm_scaled    = g_status.pos_internal.z_mm_scaled;
    g_status.pos_measured.phi_deg_scaled = g_status.pos_internal.phi_deg_scaled;
}

int32_t position_get_x_mm_scaled(void)
{
    return counts_to_mm_scaled(ex.acc_counts + ex.offset_counts, ex.counts_per_mm);
}

int32_t position_get_y_mm_scaled(void)
{
    return counts_to_mm_scaled(ey.acc_counts + ey.offset_counts, ey.counts_per_mm);
}

int32_t position_get_x_counts(void) { return ex.acc_counts + ex.offset_counts; }
int32_t position_get_y_counts(void) { return ey.acc_counts + ey.offset_counts; }

void position_set_xy_mm_scaled(int32_t x_mm_scaled, int32_t y_mm_scaled)
{
    int32_t x_des_counts = mm_scaled_to_counts(x_mm_scaled, ex.counts_per_mm);
    int32_t y_des_counts = mm_scaled_to_counts(y_mm_scaled, ey.counts_per_mm);

    ex.offset_counts = x_des_counts - ex.acc_counts;
    ey.offset_counts = y_des_counts - ey.acc_counts;

    position_poll();
}
void position_set_x_mm_scaled(int32_t x_mm_scaled)
{
    int32_t x_des_counts = mm_scaled_to_counts(x_mm_scaled, ex.counts_per_mm);
    ex.offset_counts = x_des_counts - ex.acc_counts;
    position_poll();
}

void position_set_y_mm_scaled(int32_t y_mm_scaled)
{
    int32_t y_des_counts = mm_scaled_to_counts(y_mm_scaled, ey.counts_per_mm);
    ey.offset_counts = y_des_counts - ey.acc_counts;
    position_poll();
}


void position_sync_measured_to_internal(void)
{
    position_set_xy_mm_scaled(g_status.pos_internal.x_mm_scaled, g_status.pos_internal.y_mm_scaled);
}

#else

void position_init(void) {}
void position_poll(void) {}
int32_t position_get_x_mm_scaled(void) { return g_status.pos_internal.x_mm_scaled; }
int32_t position_get_y_mm_scaled(void) { return g_status.pos_internal.y_mm_scaled; }
int32_t position_get_x_counts(void) { return 0; }
int32_t position_get_y_counts(void) { return 0; }
void position_set_xy_mm_scaled(int32_t x_mm_scaled, int32_t y_mm_scaled) { (void)x_mm_scaled; (void)y_mm_scaled; }
void position_set_x_mm_scaled(int32_t x_mm_scaled) { (void)x_mm_scaled; }
void position_set_y_mm_scaled(int32_t y_mm_scaled) { (void)y_mm_scaled; }
void position_sync_measured_to_internal(void) {}

#endif
