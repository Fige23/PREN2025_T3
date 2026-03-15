/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_))
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|

robot_config.h
Created on: 24.11.2025
Author: Fige23
Team 3

=====================================================================
Config file: Hier können alle zentralen Parameter des Roboters angepasst werden
=====================================================================
*/

#ifndef CONFIG_ROBOT_CONFIG_H_
#define CONFIG_ROBOT_CONFIG_H_

#include <stdint.h>

/* ============================================================================
 * 1) BUILD / FEATURE MODES
 * ========================================================================== */

// Build mode selection
// Genau EIN Mode soll 1 sein.
#define IMPLEMENTATION_STEPPER          1
#define UART_DEMO                       0

#if (IMPLEMENTATION_STEPPER + UART_DEMO) != 1
#error "Select exactly one mode: IMPLEMENTATION_STEPPER or UART_DEMO"
#endif

// 1 = MOVE nur erlaubt nach HOME
// 0 = MOVE auch ohne HOME erlauben
#define REQUIRE_HOME_FOR_MOVE           0

// 1 = beim Start Kalibrier-Routine ausführen
// 0 = normaler Start ohne Kalibrierung
#define CALIBRATION_MODE                0

// Position feedback / closed loop
#define POSITION_ENABLE                 1   // 0=kein Encoder, 1=Encoder aktiv
#define POSITION_CLOSED_LOOP_ENABLE     1   // 0=nur messen, 1=MOVE korrigiert nach

/* ============================================================================
 * 2) COMMUNICATION / DEBUG / TEST
 * ========================================================================== */

// Protokoll-Antworten
#define PROTO_REPLY_BUFFER_LEN          192

// Debug-Ausgabe
#define DEBUG_ENABLE                    1
#define DEBUG_BUFFER_LEN                192

#define DEBUG_BACKEND_NONE              0
#define DEBUG_BACKEND_SEMIHOST          1
#define DEBUG_BACKEND_UART              2

#define DEBUG_BACKEND                   DEBUG_BACKEND_SEMIHOST

// Test-Frontend: lokale UART-Simulation über Semihost-Konsole
// 1 = console_uart_sim aktiv
// 0 = nur normales cmd_poll()
#define ENABLE_CONSOLE_UART_SIM         1

/* ============================================================================
 * 3) CALIBRATION
 * ========================================================================== */

// Kalibrierachsen
#define CAL_AXIS_X                      0
#define CAL_AXIS_Y                      1
#define CAL_AXIS_Z                      2

// Such-/Fahrparameter
#define CAL_STEP_WAIT_TIME_US           500U
#define CAL_MAX_SEARCH_STEPS            200000ULL
#define CAL_MAX_TRAVEL_STEPS            400000ULL

// Entprellung / Re-Trigger-Schutz
#define CAL_SWITCH_DEBOUNCE_US          3000U
#define CAL_SWITCH_LOCKOUT_STEPS        20U

// Gemessene Achslängen zwischen den gültigen Triggerpunkten
#define CAL_AXIS_X_LENGTH_MM            523.7f
#define CAL_AXIS_Y_LENGTH_MM            0.0f
#define CAL_AXIS_Z_LENGTH_MM            0.0f

/* ============================================================================
 * 4) SCALING / GEOMETRY / AXIS CONVERSION
 * ========================================================================== */

// Fixed-point Auflösung
#define SCALE_MM                        1000   // 0.001 mm
#define SCALE_DEG                       100    // 0.01 °

// Achsumrechnung
// mechanikabhängig, kalibrieren
// Angaben in steps pro 1000 mm bzw. 1000 deg
#define STEPS_PER_MM_X_Q1000            80000
#define STEPS_PER_MM_Y_Q1000            80000
#define STEPS_PER_MM_Z_Q1000           400000
#define STEPS_PER_DEG_PHI_Q1000         10000

// Drehrichtungs-Invertierung der Motoren
#define INVERT_ROT_X                    0
#define INVERT_ROT_Y                    0
#define INVERT_ROT_Z                    0
#define INVERT_ROT_PHI                  0

/* ============================================================================
 * 5) MOTION ENGINE / PROFILE
 * ========================================================================== */

// Motion-Timer / Pulse Engine
#define STEP_TICK_HZ                    240000u
#define STEP_PULSE_WIDTH_TICKS          2u
#define STEP_MIN_PERIOD_TICKS           2u

// Bewegungsprofil
// kurze Wege -> Dreiecksprofil
// lange Wege -> durch VMAX gedeckelt
#define MOTION_PROFILE_ENABLE           1
#define MOTION_PROFILE_SYMMETRIC        1

// X axis
#define X_MAX_STEP_RATE_SPS             12000u
#define X_START_STEP_RATE_SPS             400u
#define X_ACCEL_SPS2                     30000u

// Y axis
#define Y_MAX_STEP_RATE_SPS             12000u
#define Y_START_STEP_RATE_SPS             400u
#define Y_ACCEL_SPS2                     30000u

// Z axis
#define Z_MAX_STEP_RATE_SPS              2500u
#define Z_START_STEP_RATE_SPS             400u
#define Z_ACCEL_SPS2                     10000u

// PHI axis
#define PHI_MAX_STEP_RATE_SPS            1800u
#define PHI_START_STEP_RATE_SPS           300u
#define PHI_ACCEL_SPS2                    7000u

// Optionales weiches Nachführen der Periodendauer
#define ENABLE_PERIOD_SMOOTHING          0

// Filterstärke:
// 1 => stark direkt
// 2 => 1/4 pro Update
// 3 => 1/8 pro Update
// 4 => 1/16 pro Update
#define PERIOD_SMOOTH_SHIFT              2

// minimale Tick-Änderung bei aktivem Smoothing
#define PERIOD_SMOOTH_MINSTEP            1

/* ============================================================================
 * 6) WORKSPACE / SAFETY LIMITS
 * ========================================================================== */

// Physikalische Limits (unskaliert)
#define LIMIT_X_MIN                      0
#define LIMIT_X_MAX                      300
#define LIMIT_Y_MIN                      0
#define LIMIT_Y_MAX                      300
#define LIMIT_Z_MIN                      0
#define LIMIT_Z_MAX                      150
#define LIMIT_PHI_MIN                   -180
#define LIMIT_PHI_MAX                    180

// Safe-Z Höhe für PICK/PLACE Sequenzen
// MOVE ignoriert Safe-Z bewusst
#define SAFE_Z_MAX_DURING_XY             (50 * SCALE_MM)

/* ============================================================================
 * 7) POSITION FEEDBACK (AS5311)
 * ========================================================================== */

// AS5311 (Quadratur, x4): 1024 edges pro 2.0 mm => 512 counts/mm
#define ENC_X_COUNTS_PER_MM              512u
#define ENC_Y_COUNTS_PER_MM              512u

// Falls Encoder-Richtung nicht mit +mm übereinstimmt
#define ENC_X_INVERT                     0
#define ENC_Y_INVERT                     0

// Toleranzen (Fixed-point mm * 1000)
#define POS_TOL_X_SCALED                 20      // 0.020 mm
#define POS_TOL_Y_SCALED                 20      // 0.020 mm

// Korrektur-Regelung
#define POS_CORR_P_ZAEHLER               1       // gain = NUM / DEN
#define POS_CORR_P_NENNER                1
#define POS_CORR_MAX_STEP_SCALED         2000    // max 2.0 mm pro Korrektur
#define POS_CORR_MAX_ITERATIONS          6

/* ============================================================================
 * 8) DERIVED / HELPER MACROS
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

// Abgeleitete Maximalgeschwindigkeiten in physikalischen Einheiten
#define VMAX_X_MM_S     ((float)X_MAX_STEP_RATE_SPS    / ((float)STEPS_PER_MM_X_Q1000    / 1000.0f))
#define VMAX_Y_MM_S     ((float)Y_MAX_STEP_RATE_SPS    / ((float)STEPS_PER_MM_Y_Q1000    / 1000.0f))
#define VMAX_Z_MM_S     ((float)Z_MAX_STEP_RATE_SPS    / ((float)STEPS_PER_MM_Z_Q1000    / 1000.0f))
#define VMAX_PHI_DEG_S  ((float)PHI_MAX_STEP_RATE_SPS  / ((float)STEPS_PER_DEG_PHI_Q1000 / 1000.0f))

/* ============================================================================
 * 9) CONFIG CHECKS
 * ========================================================================== */

#if (MOTION_PROFILE_ENABLE != 0) && (MOTION_PROFILE_ENABLE != 1)
#error "MOTION_PROFILE_ENABLE must be 0 or 1"
#endif

#if (MOTION_PROFILE_SYMMETRIC != 0) && (MOTION_PROFILE_SYMMETRIC != 1)
#error "MOTION_PROFILE_SYMMETRIC must be 0 or 1"
#endif

#if (ENABLE_PERIOD_SMOOTHING != 0) && (ENABLE_PERIOD_SMOOTHING != 1)
#error "ENABLE_PERIOD_SMOOTHING must be 0 or 1"
#endif

#if (POSITION_ENABLE != 0) && (POSITION_ENABLE != 1)
#error "POSITION_ENABLE must be 0 or 1"
#endif

#if (POSITION_CLOSED_LOOP_ENABLE != 0) && (POSITION_CLOSED_LOOP_ENABLE != 1)
#error "POSITION_CLOSED_LOOP_ENABLE must be 0 or 1"
#endif

#if (DEBUG_ENABLE != 0) && (DEBUG_ENABLE != 1)
#error "DEBUG_ENABLE must be 0 or 1"
#endif

#if (ENABLE_CONSOLE_UART_SIM != 0) && (ENABLE_CONSOLE_UART_SIM != 1)
#error "ENABLE_CONSOLE_UART_SIM must be 0 or 1"
#endif

#endif /* CONFIG_ROBOT_CONFIG_H_ */
