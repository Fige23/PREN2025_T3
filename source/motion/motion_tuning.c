/* Runtime motion tuning for UART-controlled speed scaling. */

#include "motion_tuning.h"

typedef struct {
    uint16_t global_scale_pct;
    uint16_t profile_scale_pct[5];
} motion_tuning_state_s;

static motion_tuning_state_s g_motion_tuning;

static bool motion_tuning_scale_valid(uint16_t scale_pct)
{
    return (scale_pct >= MOTION_TUNING_SCALE_MIN_PCT) &&
           (scale_pct <= MOTION_TUNING_SCALE_MAX_PCT);
}

static uint32_t scale_value_u32(uint32_t base, uint32_t scale_pct)
{
    uint64_t scaled = (uint64_t)base * (uint64_t)scale_pct;

    scaled = (scaled + 50u) / 100u;
    if (scaled < 1u) {
        scaled = 1u;
    }
    if (scaled > 0xFFFFFFFFu) {
        scaled = 0xFFFFFFFFu;
    }

    return (uint32_t)scaled;
}

static bool kind_valid(motion_profile_kind_e kind)
{
    return (kind >= MOTION_PROFILE_KIND_MOVE) && (kind <= MOTION_PROFILE_KIND_CORR);
}

void motion_tuning_init(void)
{
    uint32_t i;

    g_motion_tuning.global_scale_pct = 100u;

    for (i = 0; i < (uint32_t)(sizeof(g_motion_tuning.profile_scale_pct) /
                               sizeof(g_motion_tuning.profile_scale_pct[0])); ++i) {
        g_motion_tuning.profile_scale_pct[i] = 100u;
    }
}

bool motion_tuning_set_global_scale_pct(uint16_t scale_pct)
{
    if (!motion_tuning_scale_valid(scale_pct)) {
        return false;
    }

    g_motion_tuning.global_scale_pct = scale_pct;
    return true;
}

uint16_t motion_tuning_get_global_scale_pct(void)
{
    return g_motion_tuning.global_scale_pct;
}

bool motion_tuning_set_profile_scale_pct(motion_profile_kind_e kind, uint16_t scale_pct)
{
    if (!kind_valid(kind) || !motion_tuning_scale_valid(scale_pct)) {
        return false;
    }

    g_motion_tuning.profile_scale_pct[(uint32_t)kind] = scale_pct;
    return true;
}

uint16_t motion_tuning_get_profile_scale_pct(motion_profile_kind_e kind)
{
    if (!kind_valid(kind)) {
        return 100u;
    }

    return g_motion_tuning.profile_scale_pct[(uint32_t)kind];
}

motion_profile_s motion_tuning_apply_profile(const motion_profile_s *base,
                                             motion_profile_kind_e kind)
{
    motion_profile_s scaled = *base;
    uint32_t total_scale_pct = motion_tuning_get_global_scale_pct();

    total_scale_pct = (total_scale_pct * motion_tuning_get_profile_scale_pct(kind) + 50u) / 100u;
    if (total_scale_pct < 1u) {
        total_scale_pct = 1u;
    }

    scaled.start_step_rate_sps = scale_value_u32(base->start_step_rate_sps, total_scale_pct);
    scaled.max_step_rate_sps = scale_value_u32(base->max_step_rate_sps, total_scale_pct);
    scaled.accel_sps2 = scale_value_u32(base->accel_sps2, total_scale_pct);

    if (scaled.start_step_rate_sps > scaled.max_step_rate_sps) {
        scaled.start_step_rate_sps = scaled.max_step_rate_sps;
    }

    return scaled;
}

const char *motion_tuning_kind_to_str(motion_profile_kind_e kind)
{
    switch (kind) {
    case MOTION_PROFILE_KIND_MOVE:
        return "MOVE";
    case MOTION_PROFILE_KIND_HOME:
        return "HOME";
    case MOTION_PROFILE_KIND_PICK:
        return "PICK";
    case MOTION_PROFILE_KIND_PLACE:
        return "PLACE";
    case MOTION_PROFILE_KIND_CORR:
        return "CORR";
    default:
        return "UNKNOWN";
    }
}
