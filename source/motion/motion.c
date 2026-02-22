/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )
 (()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_))
 (_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
 | _ \ _ \ __| \| | | _ \ | | |_  /_
...
===============================================================================
motion_start(...) startet eine Bewegung.
- motion_is_done()/motion_last_err() melden Completion.
- IO ist gekapselt in io.c (Pins toggeln).
===============================================================================
*/
#include <stdint.h>
#include <stdbool.h>

#include "MK22F51212.h"
#include "platform.h"

#include "io.h"
#include "protocol.h"
#include "bot.h"
#include "robot_config.h"
#include "ftm3.h"

// ---------- motion tick / pulse ----------
#define PULSE_WIDTH_TICKS      1u
#define MIN_STEP_PERIOD_TICKS  2u

typedef enum {
    AX_X = 0, AX_Y = 1, AX_Z = 2, AX_PHI = 3, AX_N = 4
} axis_e;

static inline int32_t iabs32(int32_t v) { return (v < 0) ? -v : v; }

// d_scaled * steps_per_unit / scale  (rounded)
static int32_t scaled_to_steps(int32_t d_scaled, int32_t steps_per_unit, int32_t scale)
{
    int64_t num = (int64_t)d_scaled * (int64_t)steps_per_unit;
    if (num >= 0) return (int32_t)((num + scale/2) / scale);
    else          return (int32_t)((num - scale/2) / scale);
}

// period_ticks = ceil( STEP_TICK_HZ / v_sps )
static inline uint32_t ticks_from_v_q16(uint32_t v_sps_q16)
{
    if (v_sps_q16 == 0u) v_sps_q16 = 1u;
    uint64_t num = ((uint64_t)STEP_TICK_HZ << 16);
    uint32_t period = (uint32_t)((num + (uint64_t)v_sps_q16 - 1u) / (uint64_t)v_sps_q16);
    if (period < MIN_STEP_PERIOD_TICKS) period = MIN_STEP_PERIOD_TICKS;
    return period;
}

// stop_steps = (v^2 - vmin^2)/(2a)  (Q16.16), return steps (floor)
static inline uint32_t stop_steps_from_vmin_q16(uint32_t v_q16, uint32_t vmin_q16, uint32_t a_q16)
{
    if (a_q16 == 0u) return 0xFFFFFFFFu;
    if (v_q16 <= vmin_q16) return 0u;

    uint64_t v2    = (uint64_t)v_q16    * (uint64_t)v_q16;    // Q32
    uint64_t vmin2 = (uint64_t)vmin_q16 * (uint64_t)vmin_q16; // Q32
    uint64_t diff  = v2 - vmin2;                               // Q32

    uint64_t denom = 2ull * (uint64_t)a_q16;                   // Q16
    uint32_t stop_q16 = (uint32_t)(diff / denom);              // Q16
    return (stop_q16 >> 16);
}

// dv per MAJOR STEP (konstante Beschleunigung in step-domain):
// dt = 1/v, dv = a*dt = a/v
static inline uint32_t dv_per_step_q16(uint32_t a_q16, uint32_t v_q16)
{
    if (v_q16 == 0u) v_q16 = 1u;
    uint64_t num = ((uint64_t)a_q16 << 16);
    uint32_t dv = (uint32_t)(num / (uint64_t)v_q16);
    if (dv == 0u) dv = 1u;
    return dv;
}

// --- pin wrappers ---
static void step_set(axis_e ax, bool level)
{
    switch (ax) {
    case AX_X:   stepper_x_step(level);   break;
    case AX_Y:   stepper_y_step(level);   break;
    case AX_Z:   stepper_z_step(level);   break;
    case AX_PHI: stepper_phi_step(level); break;
    default: break;
    }
}

static void dir_set(axis_e ax, bool dir_pos)
{
    switch (ax) {
    case AX_X:   stepper_x_dir(dir_pos);   break;
    case AX_Y:   stepper_y_dir(dir_pos);   break;
    case AX_Z:   stepper_z_dir(dir_pos);   break;
    case AX_PHI: stepper_phi_dir(dir_pos); break;
    default: break;
    }
}

typedef struct {
    volatile bool active;
    volatile bool done;
    volatile err_e err;

    uint32_t tick_div;
    uint32_t step_period_ticks;         // aktueller Period (wird geglättet)
    uint32_t step_period_target_ticks;  // Ziel-Period aus Profil (optional)

    uint32_t major_total;
    uint32_t major_left;

    // --- speed profile on MAJOR axis (Q16.16 steps/s, steps/s^2) ---
    uint32_t v_q16;        // current speed
    uint32_t v_start_q16;  // minimum/start/end speed
    uint32_t v_max_q16;    // speed cap
    uint32_t a_q16;        // accel cap

    uint32_t steps_abs[AX_N];
    uint32_t acc[AX_N];
    uint8_t  pulse_left[AX_N];

    // position tracking
    int64_t pos_num[AX_N];         // scaled * steps_per_unit
    int32_t steps_per_unit[AX_N];
    int32_t scale[AX_N];
    int8_t  sign[AX_N];
} motion_s;

static motion_s m = {0};

static void motion_tick_isr(void);

bool motion_is_done(void) { return m.done; }
err_e motion_last_err(void){ return m.err; }

static void motion_finish(err_e e)
{
    m.active = false;
    m.done = true;
    m.err = e;

    for (int i=0;i<AX_N;i++){
        m.pulse_left[i] = 0;
        step_set((axis_e)i, false);
    }
}

static void motion_tick_dispatch(void)
{
    if (!m.active) return;
    motion_tick_isr();
}

void motion_init(void)
{
    ftm3_tick_set_callback(motion_tick_dispatch);
}

err_e motion_start(const bot_action_s *cur)
{
    if (m.active) return ERR_INTERNAL;

    const int32_t spx   = (int32_t)(STEPS_PER_MM_X + 0.5f);
    const int32_t spy   = (int32_t)(STEPS_PER_MM_Y + 0.5f);
    const int32_t spz   = (int32_t)(STEPS_PER_MM_Z + 0.5f);
    const int32_t spphi = (int32_t)(STEPS_PER_DEG_PHI + 0.5f);

    int32_t dx = cur->target_pos.x_mm_scaled    - g_status.pos_cmd.x_mm_scaled;
    int32_t dy = cur->target_pos.y_mm_scaled    - g_status.pos_cmd.y_mm_scaled;
    int32_t dz = cur->target_pos.z_mm_scaled    - g_status.pos_cmd.z_mm_scaled;
    int32_t dp = cur->target_pos.phi_deg_scaled - g_status.pos_cmd.phi_deg_scaled;

    int32_t sx = scaled_to_steps(dx, spx,   SCALE_MM);
    int32_t sy = scaled_to_steps(dy, spy,   SCALE_MM);
    int32_t sz = scaled_to_steps(dz, spz,   SCALE_MM);
    int32_t sp = scaled_to_steps(dp, spphi, SCALE_DEG);

    m.sign[AX_X]   = (sx >= 0) ? +1 : -1;
    m.sign[AX_Y]   = (sy >= 0) ? +1 : -1;
    m.sign[AX_Z]   = (sz >= 0) ? +1 : -1;
    m.sign[AX_PHI] = (sp >= 0) ? +1 : -1;

    dir_set(AX_X,   sx >= 0);
    dir_set(AX_Y,   sy >= 0);
    dir_set(AX_Z,   sz >= 0);
    dir_set(AX_PHI, sp >= 0);

    uint32_t ax = (uint32_t)iabs32(sx);
    uint32_t ay = (uint32_t)iabs32(sy);
    uint32_t az = (uint32_t)iabs32(sz);
    uint32_t ap = (uint32_t)iabs32(sp);

    uint32_t major = ax;
    axis_e major_ax = AX_X;
    if (ay > major){ major = ay; major_ax = AX_Y; }
    if (az > major){ major = az; major_ax = AX_Z; }
    if (ap > major){ major = ap; major_ax = AX_PHI; }

    if (major == 0){
        m.active = false;
        m.done = true;
        m.err = ERR_NONE;
        return ERR_NONE;
    }

    // tracking config
    m.steps_per_unit[AX_X]   = spx;
    m.steps_per_unit[AX_Y]   = spy;
    m.steps_per_unit[AX_Z]   = spz;
    m.steps_per_unit[AX_PHI] = spphi;

    m.scale[AX_X]   = SCALE_MM;
    m.scale[AX_Y]   = SCALE_MM;
    m.scale[AX_Z]   = SCALE_MM;
    m.scale[AX_PHI] = SCALE_DEG;

    m.pos_num[AX_X]   = (int64_t)g_status.pos_cmd.x_mm_scaled    * (int64_t)m.steps_per_unit[AX_X];
    m.pos_num[AX_Y]   = (int64_t)g_status.pos_cmd.y_mm_scaled    * (int64_t)m.steps_per_unit[AX_Y];
    m.pos_num[AX_Z]   = (int64_t)g_status.pos_cmd.z_mm_scaled    * (int64_t)m.steps_per_unit[AX_Z];
    m.pos_num[AX_PHI] = (int64_t)g_status.pos_cmd.phi_deg_scaled * (int64_t)m.steps_per_unit[AX_PHI];

    // reset state
    m.done = false;
    m.err = ERR_NONE;
    m.tick_div = 0;

    m.steps_abs[AX_X]   = ax;
    m.steps_abs[AX_Y]   = ay;
    m.steps_abs[AX_Z]   = az;
    m.steps_abs[AX_PHI] = ap;

    for (int i=0;i<AX_N;i++){
        m.acc[i] = 0;
        m.pulse_left[i] = 0;
        step_set((axis_e)i, false);
    }

    m.major_total = major;
    m.major_left  = major;

    // --- time-optimal profile caps (step-domain) ---
    uint32_t vmax_sps   = 1000u;
    uint32_t vstart_sps = 200u;
    uint32_t acc_sps2   = 5000u;

    switch (major_ax) {
    case AX_X:
        vmax_sps   = X_MAX_STEP_RATE_SPS;
        vstart_sps = X_START_STEP_RATE_SPS;
        acc_sps2   = X_ACCEL_SPS2;
        break;
    case AX_Y:
        vmax_sps   = Y_MAX_STEP_RATE_SPS;
        vstart_sps = Y_START_STEP_RATE_SPS;
        acc_sps2   = Y_ACCEL_SPS2;
        break;
    case AX_Z:
        vmax_sps   = Z_MAX_STEP_RATE_SPS;
        vstart_sps = Z_START_STEP_RATE_SPS;
        acc_sps2   = Z_ACCEL_SPS2;
        break;
    case AX_PHI:
        vmax_sps   = PHI_MAX_STEP_RATE_SPS;
        vstart_sps = PHI_START_STEP_RATE_SPS;
        acc_sps2   = PHI_ACCEL_SPS2;
        break;
    default: break;
    }

    if (vmax_sps < 1u) vmax_sps = 1u;
    if (vstart_sps < 1u) vstart_sps = 1u;
    if (vstart_sps > vmax_sps) vstart_sps = vmax_sps;
    if (acc_sps2 < 1u) acc_sps2 = 1u;

    m.v_start_q16 = (vstart_sps << 16);
    m.v_max_q16   = (vmax_sps   << 16);
    m.a_q16       = (acc_sps2   << 16);

    // Start NICHT bei 0 → "menschlich", kein Schneckentempo
    m.v_q16 = m.v_start_q16;

    m.step_period_ticks        = ticks_from_v_q16(m.v_q16);
    m.step_period_target_ticks = m.step_period_ticks;

    m.active = true;
    return ERR_NONE;
}

static void motion_tick_isr(void)
{
    // STEP low wenn pulsbreite vorbei
    for (int i=0;i<AX_N;i++){
        if (m.pulse_left[i]){
            m.pulse_left[i]--;
            if (m.pulse_left[i] == 0){
                step_set((axis_e)i, false);
            }
        }
    }

    if (!m.active) return;

    if (g_status.estop || g_status.state == STATE_EMERGENCY_STOP){
        motion_finish(ERR_ESTOP);
        return;
    }

    // slice timing
    m.tick_div++;
    if (m.tick_div < m.step_period_ticks) return;
    m.tick_div = 0;

    uint8_t changed_mask = 0;

    // DDA/Bresenham slice
    for (int i=0;i<AX_N;i++){
        uint32_t s = m.steps_abs[i];
        if (s == 0) continue;

        m.acc[i] += s;

        if (m.acc[i] >= m.major_total){
            m.acc[i] -= m.major_total;

            step_set((axis_e)i, true);
            m.pulse_left[i] = PULSE_WIDTH_TICKS;

            m.pos_num[i] += (int64_t)m.sign[i] * (int64_t)m.scale[i];
            changed_mask |= (1u<<i);
        }
    }

    if (changed_mask & (1u<<AX_X)){
        g_status.pos_cmd.x_mm_scaled = (int32_t)(m.pos_num[AX_X] / (int64_t)m.steps_per_unit[AX_X]);
    }
    if (changed_mask & (1u<<AX_Y)){
        g_status.pos_cmd.y_mm_scaled = (int32_t)(m.pos_num[AX_Y] / (int64_t)m.steps_per_unit[AX_Y]);
    }
    if (changed_mask & (1u<<AX_Z)){
        g_status.pos_cmd.z_mm_scaled = (int32_t)(m.pos_num[AX_Z] / (int64_t)m.steps_per_unit[AX_Z]);
    }
    if (changed_mask & (1u<<AX_PHI)){
        g_status.pos_cmd.phi_deg_scaled = (int32_t)(m.pos_num[AX_PHI] / (int64_t)m.steps_per_unit[AX_PHI]);
    }

    // solange kein encoder: measured = cmd
    g_status.pos_measured = g_status.pos_cmd;

    // major countdown
    if (--m.major_left == 0){
        motion_finish(ERR_NONE);
        return;
    }

    // ----------------- time-optimal accel/decel (triangle/trapez automatisch) -----------------
    uint32_t stop_steps = stop_steps_from_vmin_q16(m.v_q16, m.v_start_q16, m.a_q16);
    bool need_decel = (m.major_left <= (stop_steps + 1u));

    uint32_t dv = dv_per_step_q16(m.a_q16, m.v_q16);

    if (need_decel) {
        if (m.v_q16 > m.v_start_q16 + dv) m.v_q16 -= dv;
        else                              m.v_q16  = m.v_start_q16;
    } else {
        uint64_t vnew = (uint64_t)m.v_q16 + (uint64_t)dv;
        if (vnew > (uint64_t)m.v_max_q16) vnew = (uint64_t)m.v_max_q16;
        m.v_q16 = (uint32_t)vnew;
    }

    // Zielperiode aus Profil
    m.step_period_target_ticks = ticks_from_v_q16(m.v_q16);

#if ENABLE_PERIOD_SMOOTHING
    // ----------------- SOUND SMOOTHING (weicher Klang, weniger "Treppengeräusch") -----------------
    // IIR: period += (target - period) / 2^SHIFT
    int32_t err = (int32_t)m.step_period_target_ticks - (int32_t)m.step_period_ticks;
    int32_t delta = (err >> PERIOD_SMOOTH_SHIFT);

    if (delta == 0 && err != 0) {
        delta = (err > 0) ? (int32_t)PERIOD_SMOOTH_MINSTEP : -(int32_t)PERIOD_SMOOTH_MINSTEP;
    }

    int32_t newp = (int32_t)m.step_period_ticks + delta;

    if (newp < (int32_t)MIN_STEP_PERIOD_TICKS) newp = (int32_t)MIN_STEP_PERIOD_TICKS;
    m.step_period_ticks = (uint32_t)newp;
#else
    // ohne smoothing: direkt springen
    m.step_period_ticks = m.step_period_target_ticks;
#endif
}
