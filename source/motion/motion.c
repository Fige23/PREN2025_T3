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

#include "motion.h"
#include "io.h"
#include "protocol.h"
#include "bot_engine.h"
#include "robot_config.h"
#include "ftm3.h"
#include "limit_switch.h"
#include "poll.h"
#include "tmc2209.h"
#if !ENABLE_CONSOLE_UART_SIM
#include "position.h"
#endif
#if SYSTEMVIEW
#include "SEGGER_SYSVIEW.h"
#include "debug.h"
#endif



// ---------- motion tick / pulse ----------
#define PULSE_WIDTH_TICKS      STEP_PULSE_WIDTH_TICKS
#define MIN_STEP_PERIOD_TICKS  STEP_MIN_PERIOD_TICKS

typedef enum {
    AX_X = 0,
    AX_Y = 1,
    AX_Z = 2,
    AX_PHI = 3,
    AX_N = 4
} axis_e;

static inline int32_t iabs32(int32_t v)
{
    return (v < 0) ? -v : v;
}

// d_scaled * steps_per_unit / scale  (rounded)
static int32_t scaled_to_steps_q1000(int32_t d_scaled,
                                     int32_t steps_per_unit_q1000,
                                     int32_t scale)
{
    int64_t num = (int64_t)d_scaled * (int64_t)steps_per_unit_q1000;
    int64_t den = (int64_t)scale * 1000LL;

    if (num >= 0) {
        return (int32_t)((num + den / 2) / den);
    } else {
        return (int32_t)((num - den / 2) / den);
    }
}

static inline uint32_t ticks_from_speed_sps(uint32_t speed_sps)
{
    if (speed_sps < 1u) {
        speed_sps = 1u;
    }

    uint32_t period = (STEP_TICK_HZ + speed_sps - 1u) / speed_sps;

    if (period < MIN_STEP_PERIOD_TICKS) {
        period = MIN_STEP_PERIOD_TICKS;
    }

    return period;
}

static int32_t scale_steps_per_unit_for_microsteps(int32_t nominal_steps_q1000,
                                                   uint16_t active_microsteps,
                                                   uint16_t nominal_microsteps)
{
    if (nominal_microsteps == 0u) {
        return nominal_steps_q1000;
    }

    uint64_t num = (uint64_t)nominal_steps_q1000 * (uint64_t)active_microsteps;
    return (int32_t)((num + (nominal_microsteps / 2u)) / (uint64_t)nominal_microsteps);
}

static uint32_t isqrt_u64(uint64_t x)
{
    uint64_t op = x;
    uint64_t res = 0;
    uint64_t one = 1ull << 62;

    while (one > op) {
        one >>= 2;
    }

    while (one != 0) {
        if (op >= res + one) {
            op -= (res + one);
            res += (one << 1);
        }
        res >>= 1;
        one >>= 2;
    }

    return (uint32_t)res;
}

// Soll-Geschwindigkeit in steps/s
// v^2 = v0^2 + 2*a*s
// acc-limit aus steps_done
// dec-limit aus steps_left
// v_cmd = min(v_acc, v_dec, v_max)
static uint32_t profile_speed_sps(uint32_t steps_done,
                                  uint32_t steps_left,
                                  uint32_t v_start_sps,
                                  uint32_t v_max_sps,
                                  uint32_t a_sps2)
{
#if MOTION_PROFILE_ENABLE && MOTION_PROFILE_SYMMETRIC
    uint64_t vstart2  = (uint64_t)v_start_sps * (uint64_t)v_start_sps;
    uint64_t vacc2    = vstart2 + 2ull * (uint64_t)a_sps2 * (uint64_t)steps_done;
    uint64_t vdec2    = vstart2 + 2ull * (uint64_t)a_sps2 * (uint64_t)steps_left;

    uint32_t v_acc_sps = isqrt_u64(vacc2);
    uint32_t v_dec_sps = isqrt_u64(vdec2);

    uint32_t v_cmd_sps = v_acc_sps;

    if (v_dec_sps < v_cmd_sps) {
        v_cmd_sps = v_dec_sps;
    }
    if (v_max_sps < v_cmd_sps) {
        v_cmd_sps = v_max_sps;
    }
    if (v_cmd_sps < v_start_sps) {
        v_cmd_sps = v_start_sps;
    }

    return v_cmd_sps;
#else
    (void)steps_done;
    (void)steps_left;
    (void)a_sps2;

    if (v_max_sps < 1u) {
        v_max_sps = 1u;
    }
    if (v_start_sps > v_max_sps) {
        v_start_sps = v_max_sps;
    }

    return v_start_sps;
#endif
}

// ---------- ISR tick counter ----------
static volatile uint32_t isr_tick_count = 0u;

uint32_t motion_get_isr_tick_count(void)
{
    return isr_tick_count;
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
    case AX_X:
#if INVERT_ROT_X
        dir_pos = !dir_pos;
#endif
        stepper_x_dir(dir_pos);
        break;

    case AX_Y:
#if INVERT_ROT_Y
        dir_pos = !dir_pos;
#endif
        stepper_y_dir(dir_pos);
        break;

    case AX_Z:
#if INVERT_ROT_Z
        dir_pos = !dir_pos;
#endif
        stepper_z_dir(dir_pos);
        break;

    case AX_PHI:
#if INVERT_ROT_PHI
        dir_pos = !dir_pos;
#endif
        stepper_phi_dir(dir_pos);
        break;

    default:
        break;
    }
}

typedef struct {
    volatile bool active;
    volatile bool done;
    volatile err_e err;

    volatile bool stopped_by_limit;
    volatile limit_switch_e stop_on_limits;
    volatile limit_switch_e hit_limits;

    uint32_t tick_div;
    uint32_t step_period_ticks;
    uint32_t step_period_target_ticks;

    uint32_t major_total;
    uint32_t major_left;

    // speed profile on MAJOR axis (integer domain)
    uint32_t v_sps;
    uint32_t v_start_sps;
    uint32_t v_max_sps;
    uint32_t a_sps2;

    uint32_t steps_abs[AX_N];
    uint32_t acc[AX_N];
    uint8_t  pulse_left[AX_N];

    // position tracking
    int64_t pos_num[AX_N];          // scaled * steps_per_unit
    int32_t steps_per_unit[AX_N];
    int32_t scale[AX_N];
    int8_t  sign[AX_N];
} motion_s;

static motion_s m = {0};

static void motion_tick_isr(void);

bool motion_is_done(void)
{
    return m.done;
}

err_e motion_last_err(void)
{
    return m.err;
}

bool motion_stopped_by_limit(void){
	return m.stopped_by_limit;
}

limit_switch_e motion_limit_hit(void){
	return m.hit_limits;
}



static void motion_finish(err_e e)
{
    m.active = false;
    m.done = true;
    m.err = e;

    for (int i = 0; i < AX_N; i++) {
        m.pulse_left[i] = 0;
        step_set((axis_e)i, false);
    }
}

static void motion_tick_dispatch(void)
{
#if ESTOP_POLL_IN_MOTION_ISR
    static uint32_t estop_poll_div = 0u;
    estop_poll_div++;
    if (estop_poll_div >= ESTOP_POLL_ISR_DIVIDER) {
        estop_poll_div = 0u;
        estop_poll();
    }
#endif

    if (!m.active) {
        return;
    }
    motion_tick_isr();
}
//setzt callback Funktion für FTM3 Tick
void motion_init(void)
{
    ftm3_tick_set_callback(motion_tick_dispatch);
}

//startet eine Bewegung
err_e motion_start(const bot_action_s *cur, limit_switch_e stop_on_limits, const motion_profile_s *profile_override){

	if (m.active) {
        return ERR_INTERNAL;
    }
    #if SYSTEMVIEW
    g_systrack.sysview_track = true;
    #endif

    m.done = false;
    m.err = ERR_NONE;
    m.stopped_by_limit = false;
    m.stop_on_limits = stop_on_limits;
    m.hit_limits = limit_none;

    reset_limit_switch(stop_on_limits);



    const int32_t spx = scale_steps_per_unit_for_microsteps(
        STEPS_PER_MM_X_Q1000, driver_get_microsteps(DRIVER_MOTOR_X), MICROSTEPS_X);
    const int32_t spy = scale_steps_per_unit_for_microsteps(
        STEPS_PER_MM_Y_Q1000, driver_get_microsteps(DRIVER_MOTOR_Y), MICROSTEPS_Y);
    const int32_t spz = scale_steps_per_unit_for_microsteps(
        STEPS_PER_MM_Z_Q1000, driver_get_microsteps(DRIVER_MOTOR_Z), MICROSTEPS_Z);
    const int32_t spphi = scale_steps_per_unit_for_microsteps(
        STEPS_PER_DEG_PHI_Q1000, driver_get_microsteps(DRIVER_MOTOR_PHI), MICROSTEPS_PHI);

    int32_t dx = cur->target_pos.x_mm_scaled    - g_status.pos_internal.x_mm_scaled;
    int32_t dy = cur->target_pos.y_mm_scaled    - g_status.pos_internal.y_mm_scaled;
    int32_t dz = cur->target_pos.z_mm_scaled    - g_status.pos_internal.z_mm_scaled;
    int32_t dp = cur->target_pos.phi_deg_scaled - g_status.pos_internal.phi_deg_scaled;

    int32_t sx = scaled_to_steps_q1000(dx, spx,   SCALE_MM);
    int32_t sy = scaled_to_steps_q1000(dy, spy,   SCALE_MM);
    int32_t sz = scaled_to_steps_q1000(dz, spz,   SCALE_MM);
    int32_t sp = scaled_to_steps_q1000(dp, spphi, SCALE_DEG);

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

    if (ay > major) {
        major = ay;
        major_ax = AX_Y;
    }
    if (az > major) {
        major = az;
        major_ax = AX_Z;
    }
    if (ap > major) {
        major = ap;
        major_ax = AX_PHI;
    }

    if (major == 0u) {
        m.active = false;
        m.done = true;
        m.err = ERR_NONE;
        m.stopped_by_limit = false;
        m.hit_limits = limit_none;
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

    m.pos_num[AX_X]   = (int64_t)g_status.pos_internal.x_mm_scaled    * (int64_t)m.steps_per_unit[AX_X];
    m.pos_num[AX_Y]   = (int64_t)g_status.pos_internal.y_mm_scaled    * (int64_t)m.steps_per_unit[AX_Y];
    m.pos_num[AX_Z]   = (int64_t)g_status.pos_internal.z_mm_scaled    * (int64_t)m.steps_per_unit[AX_Z];
    m.pos_num[AX_PHI] = (int64_t)g_status.pos_internal.phi_deg_scaled * (int64_t)m.steps_per_unit[AX_PHI];

    // reset state
    m.done = false;
    m.err = ERR_NONE;
    m.tick_div = 0u;

    m.steps_abs[AX_X]   = ax;
    m.steps_abs[AX_Y]   = ay;
    m.steps_abs[AX_Z]   = az;
    m.steps_abs[AX_PHI] = ap;

    for (int i = 0; i < AX_N; i++) {
        m.acc[i] = 0u;
        m.pulse_left[i] = 0u;
        step_set((axis_e)i, false);
    }

    m.major_total = major;
    m.major_left  = major;

    // --- profile caps for major axis ---
    uint32_t vmax_sps   = 0;
    uint32_t vstart_sps = 0;
    uint32_t acc_sps2   = 0;

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

    default:
        break;
    }
    if (profile_override != NULL) {
    	vstart_sps  = profile_override->start_step_rate_sps;
        vmax_sps    = profile_override->max_step_rate_sps;
        acc_sps2    = profile_override->accel_sps2;
    }

    if (vmax_sps < 1u) {
        vmax_sps = 1u;
    }
    if (vstart_sps < 1u) {
        vstart_sps = 1u;
    }
    if (vstart_sps > vmax_sps) {
        vstart_sps = vmax_sps;
    }
    if (acc_sps2 < 1u) {
        acc_sps2 = 1u;
    }

    m.v_start_sps = vstart_sps;
    m.v_max_sps   = vmax_sps;
    m.a_sps2      = acc_sps2;

    // Start bewusst nicht bei 0
    m.v_sps = m.v_start_sps;

    m.step_period_ticks        = ticks_from_speed_sps(m.v_sps);
    m.step_period_target_ticks = m.step_period_ticks;

    m.active = true;
    return ERR_NONE;
}
//wird in ISR aufgerufen
static void motion_tick_isr(void){
    isr_tick_count++;
    #if SYSTEMVIEW
    g_systrack.isr_cycles++;
    #endif

#if ENABLE_CONSOLE_UART_SIM
    // CRITICAL: Console-Sim kann blockieren (fgets in main loop)
    // Daher Position hier in ISR pollen wenn Console aktiv
    position_poll();
#endif

    // STEP low wenn Pulsbreite vorbei
    for (int i = 0; i < AX_N; i++) {
        if (m.pulse_left[i]) {
            m.pulse_left[i]--;
            if (m.pulse_left[i] == 0u) {
                step_set((axis_e)i, false);
            }
        }
    }

    if (!m.active) {
        return;
    }

    if (g_status.estop) {
        motion_finish(ERR_ESTOP);
        return;
    }

    //prüfen ob endschalter getroffen
    poll_limit_switch();

    limit_switch_e hit = limit_none;

    if ((m.stop_on_limits & limit_x) && g_status.limits.x_latched) {
        hit = (limit_switch_e)(hit | limit_x);
    }
    if ((m.stop_on_limits & limit_y) && g_status.limits.y_latched) {
        hit = (limit_switch_e)(hit | limit_y);
    }
    if ((m.stop_on_limits & limit_z) && g_status.limits.z_latched) {
        hit = (limit_switch_e)(hit | limit_z);
    }

    if (hit != limit_none) {
        m.hit_limits = hit;
        m.stopped_by_limit = true;
        motion_finish(ERR_NONE);
        return;
    }

    // slice timing
    m.tick_div++;
    if (m.tick_div < m.step_period_ticks) {
        return;
    }
    m.tick_div = 0u;

    uint8_t changed_mask = 0u;

    // DDA/Bresenham slice
    for (int i = 0; i < AX_N; i++) {
        uint32_t s = m.steps_abs[i];
        if (s == 0u) {
            continue;
        }

        m.acc[i] += s;

        if (m.acc[i] >= m.major_total) {
            m.acc[i] -= m.major_total;

            step_set((axis_e)i, true);
            m.pulse_left[i] = PULSE_WIDTH_TICKS;

            m.pos_num[i] += (int64_t)m.sign[i] * (int64_t)m.scale[i] * 1000LL;
            changed_mask |= (uint8_t)(1u << i);
            #if SYSTEMVIEW
            g_systrack.motion_steps++;
            #endif
        }
    }

    if (changed_mask & (1u << AX_X)) {
        g_status.pos_internal.x_mm_scaled = (int32_t)(m.pos_num[AX_X] / (int64_t)m.steps_per_unit[AX_X]);
    }
    if (changed_mask & (1u << AX_Y)) {
        g_status.pos_internal.y_mm_scaled = (int32_t)(m.pos_num[AX_Y] / (int64_t)m.steps_per_unit[AX_Y]);
    }
    if (changed_mask & (1u << AX_Z)) {
        g_status.pos_internal.z_mm_scaled = (int32_t)(m.pos_num[AX_Z] / (int64_t)m.steps_per_unit[AX_Z]);
    }
    if (changed_mask & (1u << AX_PHI)) {
        g_status.pos_internal.phi_deg_scaled = (int32_t)(m.pos_num[AX_PHI] / (int64_t)m.steps_per_unit[AX_PHI]);
    }

#if !POSITION_ENABLE
    g_status.pos_measured = g_status.pos_internal;
#endif

    // major countdown
    if (--m.major_left == 0u) {
        motion_finish(ERR_NONE);
        return;
    }

    // ----------------- symmetrisches Profil -----------------
    uint32_t steps_done = m.major_total - m.major_left;
    uint32_t steps_left = m.major_left;

    m.v_sps = profile_speed_sps(steps_done,
                                steps_left,
                                m.v_start_sps,
                                m.v_max_sps,
                                m.a_sps2);

    m.step_period_target_ticks = ticks_from_speed_sps(m.v_sps);

#if ENABLE_PERIOD_SMOOTHING
    {
        int32_t err = (int32_t)m.step_period_target_ticks - (int32_t)m.step_period_ticks;
        int32_t delta = (err >> PERIOD_SMOOTH_SHIFT);

        if ((delta == 0) && (err != 0)) {
            delta = (err > 0) ? (int32_t)PERIOD_SMOOTH_MINSTEP
                              : -(int32_t)PERIOD_SMOOTH_MINSTEP;
        }

        int32_t newp = (int32_t)m.step_period_ticks + delta;

        if (newp < (int32_t)MIN_STEP_PERIOD_TICKS) {
            newp = (int32_t)MIN_STEP_PERIOD_TICKS;
        }

        m.step_period_ticks = (uint32_t)newp;
    }
#else
    m.step_period_ticks = m.step_period_target_ticks;
#endif
}
