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


#include "position.h"
#include "robot_config.h"

#if POSITION_ENABLE

#include "fsl_ftm.h"
#include "fsl_clock.h"
#include "fsl_common.h"
#include "MK22F51212.h"

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

#ifndef ENC_PHASE_FILTER_ENABLE
#define ENC_PHASE_FILTER_ENABLE 1
#endif
#ifndef ENC_PHASE_FILTER_VAL
#define ENC_PHASE_FILTER_VAL 5u
#endif

typedef struct {
    FTM_Type *ftm;
    uint16_t last_cnt;
    int32_t  acc_counts;      // integrierte Counts (x4)
    int32_t  offset_counts;   // Offset (Koordinatensystem)
    uint32_t counts_per_mm;
    bool     invert;
} enc_axis_s;

static enc_axis_s ex;
static enc_axis_s ey;

static int32_t div_round_s64(int64_t num, int64_t den)
{
    if (den == 0) return 0;
    if (num >= 0) return (int32_t)((num + den/2) / den);
    else          return (int32_t)((num - den/2) / den);
}

static int32_t counts_to_mm_scaled(int32_t counts, uint32_t counts_per_mm)
{
    int64_t num = (int64_t)counts * (int64_t)SCALE_MM;
    return div_round_s64(num, (int64_t)counts_per_mm);
}

static int32_t mm_scaled_to_counts(int32_t mm_scaled, uint32_t counts_per_mm)
{
    int64_t num = (int64_t)mm_scaled * (int64_t)counts_per_mm;
    return div_round_s64(num, (int64_t)SCALE_MM);
}

static void enc_update(enc_axis_s *e)
{
    uint16_t now = (uint16_t)FTM_GetQuadDecoderCounterValue(e->ftm);

    // wrap-around-safe, solange zwischen reads |delta| < 32768
    int16_t delta = (int16_t)(now - e->last_cnt);
    e->last_cnt = now;

    int32_t d = (int32_t)delta;
    if (e->invert) d = -d;

    e->acc_counts += d;
}

static void enc_hw_init(enc_axis_s *e, FTM_Type *ftm)
{
    ftm_config_t cfg;
    FTM_GetDefaultConfig(&cfg);
    cfg.prescale = kFTM_Prescale_Divide_1;

    FTM_Init(ftm, &cfg);

    ftm_phase_params_t phA = {
        .enablePhaseFilter = (ENC_PHASE_FILTER_ENABLE != 0),
        .phaseFilterVal    = ENC_PHASE_FILTER_VAL,
        .phasePolarity     = kFTM_QuadPhaseNormal,
    };

    ftm_phase_params_t phB = {
        .enablePhaseFilter = (ENC_PHASE_FILTER_ENABLE != 0),
        .phaseFilterVal    = ENC_PHASE_FILTER_VAL,
        .phasePolarity     = kFTM_QuadPhaseNormal,
    };

    FTM_SetupQuadDecode(ftm, &phA, &phB, kFTM_QuadPhaseEncode);
    FTM_SetQuadDecoderModuloValue(ftm, 0u, 0xFFFFu);
    FTM_ClearQuadDecoderCounterValue(ftm);
    FTM_StartTimer(ftm, kFTM_SystemClock);

    e->ftm = ftm;
    e->last_cnt = (uint16_t)FTM_GetQuadDecoderCounterValue(ftm);
    e->acc_counts = 0;
    e->offset_counts = 0;
}

void position_init(void)
{


	CLOCK_EnableClock(kCLOCK_Ftm1);
	CLOCK_EnableClock(kCLOCK_Ftm2);


    ex.counts_per_mm = ENC_X_COUNTS_PER_MM;
    ex.invert        = (ENC_X_INVERT != 0);
    enc_hw_init(&ex, FTM1);

    ey.counts_per_mm = ENC_Y_COUNTS_PER_MM;
    ey.invert        = (ENC_Y_INVERT != 0);
    enc_hw_init(&ey, FTM2);

    position_sync_measured_to_cmd();
}

void position_poll(void)
{
    enc_update(&ex);
    enc_update(&ey);

    int32_t x_counts = ex.acc_counts + ex.offset_counts;
    int32_t y_counts = ey.acc_counts + ey.offset_counts;

    g_status.pos_measured.x_mm_scaled = counts_to_mm_scaled(x_counts, ex.counts_per_mm);
    g_status.pos_measured.y_mm_scaled = counts_to_mm_scaled(y_counts, ey.counts_per_mm);

    // Z/Phi (noch) open loop
    g_status.pos_measured.z_mm_scaled    = g_status.pos_cmd.z_mm_scaled;
    g_status.pos_measured.phi_deg_scaled = g_status.pos_cmd.phi_deg_scaled;
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

void position_sync_measured_to_cmd(void)
{
    position_set_xy_mm_scaled(g_status.pos_cmd.x_mm_scaled, g_status.pos_cmd.y_mm_scaled);
}

void position_get_measured(robot_pos_s *out)
{
    if (!out) return;
    position_poll();
    *out = g_status.pos_measured;
}

#else

void position_init(void) {}
void position_poll(void) {}
int32_t position_get_x_mm_scaled(void) { return g_status.pos_cmd.x_mm_scaled; }
int32_t position_get_y_mm_scaled(void) { return g_status.pos_cmd.y_mm_scaled; }
int32_t position_get_x_counts(void) { return 0; }
int32_t position_get_y_counts(void) { return 0; }
void position_set_xy_mm_scaled(int32_t x_mm_scaled, int32_t y_mm_scaled) { (void)x_mm_scaled; (void)y_mm_scaled; }
void position_sync_measured_to_cmd(void) {}
void position_get_measured(robot_pos_s *out) { if (!out) return; *out = g_status.pos_cmd; }

#endif
