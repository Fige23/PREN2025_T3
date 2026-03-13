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

/* ------------------------------------------------------------
 * Calibration
 * ---------------------------------------------------------- */
#define CALIBRATION_MODE                 1

#define CAL_AXIS_X                       0
#define CAL_AXIS_Y                       1
#define CAL_AXIS_Z                       2

#define CAL_STEP_WAIT_TIME_US            500U
#define CAL_MAX_SEARCH_STEPS             200000ULL
#define CAL_MAX_TRAVEL_STEPS             400000ULL

#define CAL_SWITCH_DEBOUNCE_US           3000U
#define CAL_SWITCH_LOCKOUT_STEPS         20U

/* Gemessene Achslängen zwischen den gültigen Triggerpunkten */
#define CAL_AXIS_X_LENGTH_MM             523.7f
#define CAL_AXIS_Y_LENGTH_MM             0.0f
#define CAL_AXIS_Z_LENGTH_MM             0.0f

// -----------------------------------------------------------------------------
// Debug/Test: GOTO über Konsole (Semihost printf/scanf) ohne cmd.c Protokoll
// 1 = Konsole nimmt Koordinaten an und queued ACT_MOVE
// 0 = normales cmd.c Protokoll (MOVE x=..., PICK, PLACE, ...)
// -----------------------------------------------------------------------------
// Ausgabe (OK/ERR/Status) optional in Debug-Konsole via printf (Semihost).
#define ENABLE_CONSOLE_GOTO         0
#define ENABLE_CONSOLE_UART_SIM     1
#define USE_SEMIHOST_CONSOLE        1
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
// (Diese Werte sind mechanik-abhängig und müssen kalibriert werden.)
// --> steps/1000mm
#define STEPS_PER_MM_X_Q1000      80000
#define STEPS_PER_MM_Y_Q1000      80000
#define STEPS_PER_MM_Z_Q1000     400000
#define STEPS_PER_DEG_PHI_Q1000   10000



// -----------------------------------------------------------------------------
// Motion tuning (Step-Domain) + Sound smoothing
// -----------------------------------------------------------------------------

// Pulse-Tick (ISR-Rate) für den Motion-Timer (FTM3)
#define STEP_TICK_HZ       240000u          //setzt maximalwert geschwindigkeit (wenns nicht schneller wird hier schrauben!)

// X/Y Limits in steps/s und steps/s^2 (HIER TUNEN!)
#define X_MAX_STEP_RATE_SPS      20000u		// max speed (major axis), steps/s
#define X_START_STEP_RATE_SPS      500u		// start speed (muss aus Stand gehen)
#define X_ACCEL_SPS2            500000u		// accel, steps/s^2

#define Y_MAX_STEP_RATE_SPS      20000u		// max speed (major axis), steps/s
#define Y_START_STEP_RATE_SPS      500u		// start speed (muss aus Stand gehen)
#define Y_ACCEL_SPS2            500000u		// accel, steps/s^2
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
#define ENABLE_PERIOD_SMOOTHING   0

// Filterstärke: kleiner = schneller (mehr "direkt"), grösser = weicher (besserer Sound)
// 3 => ~1/8 pro Update (oft guter Kompromiss)
// 4 => ~1/16 pro Update (noch smoother, minimal träger)
#define PERIOD_SMOOTH_SHIFT       1

// Mindeständerung in Ticks pro Update, damit es bei kleinen Fehlern nicht "klebt"
#define PERIOD_SMOOTH_MINSTEP     1

// -----------------------------------------------------------------------------
// Abgeleitete "mm/s" Werte (nur Info/Debug). Ändern sich mit STEPS_PER_MM_*,
// die Motor-Limits oben bleiben aber stabil.
// -----------------------------------------------------------------------------
#define VMAX_X_MM_S   ((float)X_MAX_STEP_RATE_SPS / ((float)STEPS_PER_MM_X_Q1000 / 1000.0f))
#define VMAX_Y_MM_S   ((float)Y_MAX_STEP_RATE_SPS / ((float)STEPS_PER_MM_Y_Q1000 / 1000.0f))
#define VMAX_Z_MM_S   ((float)Z_MAX_STEP_RATE_SPS / ((float)STEPS_PER_MM_Z_Q1000 / 1000.0f))
#define VMAX_PHI_DEG_S ((float)PHI_MAX_STEP_RATE_SPS / ((float)STEPS_PER_DEG_PHI_Q1000 / 1000.0f))

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

// -----------------------------------------------------------------------------
// Position feedback (AS5311 Encoder auf X/Y)
// -----------------------------------------------------------------------------
#define POSITION_ENABLE               0   // 0=kein Encoder, 1=Encoder aktiv
#define POSITION_CLOSED_LOOP_ENABLE   0   // 0=nur messen, 1=MOVE korrigiert nach

// AS5311 (Quadratur, x4): 1024 edges pro 2.0mm => 512 counts/mm
#define ENC_X_COUNTS_PER_MM          512u
#define ENC_Y_COUNTS_PER_MM          512u

// Falls Encoder-Richtung nicht mit +mm übereinstimmt
#define ENC_X_INVERT                  0
#define ENC_Y_INVERT                  0

// Toleranzen (Fixed-Point mm*1000)
#define POS_TOL_X_SCALED             (20)   // 0.020mm
#define POS_TOL_Y_SCALED             (20)   // 0.020mm

// Korrektur-Regelung
#define POS_CORR_P_ZAEHLER              1     // gain = NUM/DEN
#define POS_CORR_P_NENNER               1
#define POS_CORR_MAX_STEP_SCALED    	(2000)  // max 2.0mm pro Korrektur
#define POS_CORR_MAX_ITERATIONS         6



// Skaliert
#define LIM_X_MIN_S  ((int32_t)(LIMIT_X_MIN  * SCALE_MM))
#define LIM_X_MAX_S  ((int32_t)(LIMIT_X_MAX  * SCALE_MM))
#define LIM_Y_MIN_S  ((int32_t)(LIMIT_Y_MIN  * SCALE_MM))
#define LIM_Y_MAX_S  ((int32_t)(LIMIT_Y_MAX  * SCALE_MM))
#define LIM_Z_MIN_S  ((int32_t)(LIMIT_Z_MIN  * SCALE_MM))
#define LIM_Z_MAX_S  ((int32_t)(LIMIT_Z_MAX  * SCALE_MM))
#define LIM_P_MIN_S  ((int32_t)(LIMIT_PHI_MIN* SCALE_DEG))
#define LIM_P_MAX_S  ((int32_t)(LIMIT_PHI_MAX* SCALE_DEG))

// Flips rotation of motor. used in motion.c
#define INVERT_ROT_X 	0
#define INVERT_ROT_Y 	0
#define INVERT_ROT_Z 	0
#define INVERT_ROT_PHI 	0



#endif /* CONFIG_ROBOT_CONFIG_H_ */
