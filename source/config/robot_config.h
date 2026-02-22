/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_))
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|
robot_config.h	Created on: 24.11.2025	   Author: Fige23	Team 3

=====================================================================
Config file: Hier können alle Parameter des Roboters angepasst werden
=====================================================================
*/

#ifndef CONFIG_ROBOT_CONFIG_H_
#define CONFIG_ROBOT_CONFIG_H_

#include <stdint.h>

// 1 = MOVE nur erlaubt nach HOME, 0 = MOVE auch ohne HOME erlauben
#define REQUIRE_HOME_FOR_MOVE  0

// -----------------------------------------------------------------------------
// Debug/Test: GOTO über Konsole (Semihost printf/scanf) ohne cmd.c Protokoll
// 1 = Konsole nimmt Koordinaten an und queued ACT_MOVE
// 0 = normales cmd.c Protokoll (MOVE x=..., PICK, PLACE, ...)
// -----------------------------------------------------------------------------
#define ENABLE_CONSOLE_GOTO     1

// Ausgabe (OK/ERR/Status) optional in Debug-Konsole via printf (Semihost).
#define USE_SEMIHOST_CONSOLE    1

// -----------------------------------------------------------------------------
// Build mode selection
// Genau EIN Mode soll 1 sein.
// -----------------------------------------------------------------------------
#define IMPLEMENTATION_STEPPER   1
#define UART_DEMO                0

#if (IMPLEMENTATION_STEPPER + UART_DEMO) != 1
#error "Select exactly one mode: IMPLEMENTATION_STEPPER or UART_DEMO"
#endif

// === Steps/mm oder Steps/deg (je Achse) ===
// (Diese Werte sind mechanik-abhängig und dürfen später kalibriert werden.)
#define STEPS_PER_MM_X     80
#define STEPS_PER_MM_Y     80
#define STEPS_PER_MM_Z     400
#define STEPS_PER_DEG_PHI  10

// Pulse-Tick (ISR-Rate) für den Motion-Timer (FTM3)
#define STEP_TICK_HZ       12000u

// -----------------------------------------------------------------------------
// Motion tuning (Step-Domain) + Sound smoothing
// -----------------------------------------------------------------------------

// X/Y Limits in steps/s und steps/s^2 (HIER TUNEN!)
#define X_MAX_STEP_RATE_SPS      6000u     // max speed (major axis), steps/s
#define X_START_STEP_RATE_SPS    2000u     // start speed (muss aus Stand gehen)
#define X_ACCEL_SPS2            25000u     // accel, steps/s^2

#define Y_MAX_STEP_RATE_SPS      6000u
#define Y_START_STEP_RATE_SPS    2000u
#define Y_ACCEL_SPS2            25000u

// Z/PHI (erstmal konservativ)
#define Z_MAX_STEP_RATE_SPS      2000u
#define Z_START_STEP_RATE_SPS     800u
#define Z_ACCEL_SPS2            12000u

#define PHI_MAX_STEP_RATE_SPS    1500u
#define PHI_START_STEP_RATE_SPS   600u
#define PHI_ACCEL_SPS2           8000u

// --- Akustik / Smoothness ---
// 0 = aus (härtere, "treppige" Änderungen)
// 1 = an  (Perioden werden weich in kleinen Schritten nachgeführt)
#define ENABLE_PERIOD_SMOOTHING   1

// Filterstärke: kleiner = schneller (mehr "direkt"), grösser = weicher (besserer Sound)
// 3 => ~1/8 pro Update (oft guter Kompromiss)
// 4 => ~1/16 pro Update (noch smoother, minimal träger)
#define PERIOD_SMOOTH_SHIFT       3

// Mindeständerung in Ticks pro Update, damit es bei kleinen Fehlern nicht "klebt"
#define PERIOD_SMOOTH_MINSTEP     1

// -----------------------------------------------------------------------------
// Abgeleitete "mm/s" Werte (nur Info/Debug). Ändern sich mit STEPS_PER_MM_*,
// die Motor-Limits oben bleiben aber stabil.
// -----------------------------------------------------------------------------
#define VMAX_X_MM_S        ((float)X_MAX_STEP_RATE_SPS / (float)STEPS_PER_MM_X)
#define VMAX_Y_MM_S        ((float)Y_MAX_STEP_RATE_SPS / (float)STEPS_PER_MM_Y)
#define VMAX_Z_MM_S        ((float)Z_MAX_STEP_RATE_SPS / (float)STEPS_PER_MM_Z)
#define VMAX_PHI_DEG_S     ((float)PHI_MAX_STEP_RATE_SPS / (float)STEPS_PER_DEG_PHI)

// Auflösung (Fixed-Point)
#define SCALE_MM    1000   // 0.001 mm
#define SCALE_DEG    100   // 0.01 °

// Physikalische Limits (unskaliert)
#define LIMIT_X_MIN      0
#define LIMIT_X_MAX    300
#define LIMIT_Y_MIN      0
#define LIMIT_Y_MAX    300
#define LIMIT_Z_MIN      0
#define LIMIT_Z_MAX    150
#define LIMIT_PHI_MIN -180
#define LIMIT_PHI_MAX  180

// Safe-Z Höhe für PICK/PLACE Sequenzen (MOVE ignoriert Safe-Z bewusst)
#define SAFE_Z_MAX_DURING_XY   (50 * SCALE_MM)

// Skaliert
#define LIM_X_MIN_S  ((int32_t)(LIMIT_X_MIN  * SCALE_MM))
#define LIM_X_MAX_S  ((int32_t)(LIMIT_X_MAX  * SCALE_MM))
#define LIM_Y_MIN_S  ((int32_t)(LIMIT_Y_MIN  * SCALE_MM))
#define LIM_Y_MAX_S  ((int32_t)(LIMIT_Y_MAX  * SCALE_MM))
#define LIM_Z_MIN_S  ((int32_t)(LIMIT_Z_MIN  * SCALE_MM))
#define LIM_Z_MAX_S  ((int32_t)(LIMIT_Z_MAX  * SCALE_MM))
#define LIM_P_MIN_S  ((int32_t)(LIMIT_PHI_MIN* SCALE_DEG))
#define LIM_P_MAX_S  ((int32_t)(LIMIT_PHI_MAX* SCALE_DEG))

#endif /* CONFIG_ROBOT_CONFIG_H_ */
