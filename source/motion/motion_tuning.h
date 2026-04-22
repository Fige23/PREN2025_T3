/* Runtime motion tuning for UART-controlled speed scaling. */

#ifndef MOTION_MOTION_TUNING_H_
#define MOTION_MOTION_TUNING_H_

#include <stdbool.h>
#include <stdint.h>
#include "motion.h"

#define MOTION_TUNING_SCALE_MIN_PCT    10u
#define MOTION_TUNING_SCALE_MAX_PCT   200u

void motion_tuning_init(void);

bool motion_tuning_set_global_scale_pct(uint16_t scale_pct);
uint16_t motion_tuning_get_global_scale_pct(void);

bool motion_tuning_set_profile_scale_pct(motion_profile_kind_e kind, uint16_t scale_pct);
uint16_t motion_tuning_get_profile_scale_pct(motion_profile_kind_e kind);

motion_profile_s motion_tuning_apply_profile(const motion_profile_s *base,
                                             motion_profile_kind_e kind);

const char *motion_tuning_kind_to_str(motion_profile_kind_e kind);

#endif /* MOTION_MOTION_TUNING_H_ */
