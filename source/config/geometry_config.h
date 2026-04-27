/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_))
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|
geometry_config.h	Created on: 24.03.2026	   Author: Fige23	Team 3
*/

#ifndef CONFIG_GEOMETRY_CONFIG_H_
#define CONFIG_GEOMETRY_CONFIG_H_

/* ============================================================================
 * SCALING / GEOMETRY / AXIS CONVERSION
 * ========================================================================== */

// Fixed-point Aufloesung
#define SCALE_MM                        1000   // 0.001 mm
#define SCALE_DEG                       100    // 0.01 deg


/* ============================================================================
 * USER SETTINGS
 * Only edit the values in this section.
 * ========================================================================== */

// Motor step angle per axis.
// 1.8 deg stepper => 200 full steps per revolution.
#define MOTOR_STEP_ANGLE_DEG_X          1.8f
#define MOTOR_STEP_ANGLE_DEG_Y          1.8f
#define MOTOR_STEP_ANGLE_DEG_Z          1.8f
#define MOTOR_STEP_ANGLE_DEG_PHI        1.8f

// Driver microstepping per axis.
#define MICROSTEPS_X                    8u
#define MICROSTEPS_Y                    8u
#define MICROSTEPS_Z                    8u
#define MICROSTEPS_PHI                  8u

// Linear travel per motor revolution in um.
// These are the main mechanics values for X/Y/Z.
#define X_TRAVEL_PER_REV_UM             40640u
#define Y_TRAVEL_PER_REV_UM             40640u
#define Z_TRAVEL_PER_REV_UM              4000u

// Phi gear ratio: output revolutions per motor revolution = NUM / DEN.
// Direct drive => 1 / 1.
#define PHI_OUTPUT_REV_PER_MOTOR_REV_NUM 1u
#define PHI_OUTPUT_REV_PER_MOTOR_REV_DEN 1u

// Drehrichtungs-Invertierung der Motoren
#define INVERT_ROT_X                    0
#define INVERT_ROT_Y                    0
#define INVERT_ROT_Z                    0
#define INVERT_ROT_PHI                  0


/* ============================================================================
 * DERIVED AXIS CONVERSION
 * Normally do not edit below this line.
 * ========================================================================== */

#define FULL_STEPS_PER_REV_FROM_ANGLE(step_angle_deg) \
    ((uint32_t)((360.0f / (step_angle_deg)) + 0.5f))

#define FULL_STEPS_PER_REV_X \
    FULL_STEPS_PER_REV_FROM_ANGLE(MOTOR_STEP_ANGLE_DEG_X)
#define FULL_STEPS_PER_REV_Y \
    FULL_STEPS_PER_REV_FROM_ANGLE(MOTOR_STEP_ANGLE_DEG_Y)
#define FULL_STEPS_PER_REV_Z \
    FULL_STEPS_PER_REV_FROM_ANGLE(MOTOR_STEP_ANGLE_DEG_Z)
#define FULL_STEPS_PER_REV_PHI \
    FULL_STEPS_PER_REV_FROM_ANGLE(MOTOR_STEP_ANGLE_DEG_PHI)

#define STEPS_PER_REV_X                 (FULL_STEPS_PER_REV_X * MICROSTEPS_X)
#define STEPS_PER_REV_Y                 (FULL_STEPS_PER_REV_Y * MICROSTEPS_Y)
#define STEPS_PER_REV_Z                 (FULL_STEPS_PER_REV_Z * MICROSTEPS_Z)
#define STEPS_PER_REV_PHI               (FULL_STEPS_PER_REV_PHI * MICROSTEPS_PHI)

// Angaben in steps pro 1000 mm bzw. 1000 deg
#define STEPS_PER_MM_X_Q1000 \
    ((int32_t)((((uint64_t)STEPS_PER_REV_X) * 1000000ull + (X_TRAVEL_PER_REV_UM / 2u)) / X_TRAVEL_PER_REV_UM))

#define STEPS_PER_MM_Y_Q1000 \
    ((int32_t)((((uint64_t)STEPS_PER_REV_Y) * 1000000ull + (Y_TRAVEL_PER_REV_UM / 2u)) / Y_TRAVEL_PER_REV_UM))

#define STEPS_PER_MM_Z_Q1000 \
    ((int32_t)((((uint64_t)STEPS_PER_REV_Z) * 1000000ull + (Z_TRAVEL_PER_REV_UM / 2u)) / Z_TRAVEL_PER_REV_UM))

#define STEPS_PER_DEG_PHI_Q1000 \
    ((int32_t)((((uint64_t)STEPS_PER_REV_PHI) * PHI_OUTPUT_REV_PER_MOTOR_REV_DEN * 1000ull \
              + (180ull * PHI_OUTPUT_REV_PER_MOTOR_REV_NUM)) \
             / (360ull * PHI_OUTPUT_REV_PER_MOTOR_REV_NUM)))


/* ============================================================================
 * WORKSPACE / SAFETY LIMITS
 * ========================================================================== */

// Physikalische Limits (unskaliert)
#define LIMIT_X_MIN                      0
#define LIMIT_X_MAX                      350
#define LIMIT_Y_MIN                      0
#define LIMIT_Y_MAX                      255
#define LIMIT_Z_MIN                      0
#define LIMIT_Z_MAX                      150
#define LIMIT_PHI_MIN                   -180
#define LIMIT_PHI_MAX                    180

// Safe-Z Hoehe fuer PICK/PLACE Sequenzen
// MOVE ignoriert Safe-Z bewusst
#define SAFE_Z_MAX_DURING_XY             (50 * SCALE_MM)


/* ============================================================================
 * DERIVED / HELPER MACROS
 * ========================================================================== */

// Skaliertes Arbeitsfenster
#define LIM_X_MIN_S   ((int32_t)(LIMIT_X_MIN   * SCALE_MM))
#define LIM_X_MAX_S   ((int32_t)(LIMIT_X_MAX   * SCALE_MM))
#define LIM_Y_MIN_S   ((int32_t)(LIMIT_Y_MIN   * SCALE_MM))
#define LIM_Y_MAX_S   ((int32_t)(LIMIT_Y_MAX   * SCALE_MM))
#define LIM_Z_MIN_S   ((int32_t)(LIMIT_Z_MIN   * SCALE_MM))
#define LIM_Z_MAX_S   ((int32_t)(LIMIT_Z_MAX   * SCALE_MM))
#define LIM_P_MIN_S   ((int32_t)(LIMIT_PHI_MIN * SCALE_DEG))
#define LIM_P_MAX_S   ((int32_t)(LIMIT_PHI_MAX * SCALE_DEG))


#endif /* CONFIG_GEOMETRY_CONFIG_H_ */
