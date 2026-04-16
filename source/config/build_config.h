/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_))
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|
build_config.h	Created on: 24.03.2026	   Author: Fige23	Team 3
*/

#ifndef CONFIG_BUILD_CONFIG_H_
#define CONFIG_BUILD_CONFIG_H_

/* ============================================================================
 * BUILD CONFIGURATION
 * ============================================================================
 *
 * This file controls the build mode and feature flags for the project.
 *
 * PRODUCTION BUILD:
 * - RELEASE = 1       → Optimized, all debug features disabled
 *
 * DEVELOPMENT BUILD:
 * - RELEASE = 0       → Debug enabled, test features available
 *
 * DEBUG/TEST FEATURES (only available when RELEASE=0):
 * - CALIBRATION_MODE  → Auto-calibrate axes on startup
 * - DEMO_DRAW_MODE    → Draw test patterns for hardware diagnostics
 * - POSITION_DEBUG    → Blocking loop showing live encoder values (for debugging only!)
 *
 * See source/debug_tools/ for debug feature implementations.
 * ========================================================================== */

// -------------------------------------------------------------------------
// RELEASE MODE
// -------------------------------------------------------------------------
// 0 = Debug Build (all debug features available)
// 1 = Release Build (optimized, debug features disabled)
#define RELEASE							0

// -------------------------------------------------------------------------
// DEBUG BUILD CONFIGURATION (!RELEASE)
// -------------------------------------------------------------------------
#if !RELEASE
#define DEBUG_MODE						1

#define SYSTEMVIEW                      1

// Position feedback / closed loop
#define POSITION_ENABLE                 1   // 0=no encoder, 1=encoder active
#define POSITION_CLOSED_LOOP_ENABLE     0   // 0=measure only, 1=MOVE corrects position

// -------------------------------------------------------------------------
// DEBUG/TEST FEATURES (for development/diagnostics)
// -------------------------------------------------------------------------

// CALIBRATION_MODE: Run automatic calibration on startup
// Location: source/debug_tools/calibration/
#define CALIBRATION_MODE                0

// DEMO_DRAW_MODE: Draw test patterns for hardware diagnostics
// Location: source/debug_tools/demo_draw/
#define DEMO_DRAW_MODE					0

// POSITION_DEBUG: Blocking loop continuously printing encoder values
// Location: source/debug_tools/position_debug/
// WARNING: Blocks entire application! Only use for encoder debugging.
#define POSITION_DEBUG					0
#endif

// -------------------------------------------------------------------------
// RELEASE BUILD CONFIGURATION
// -------------------------------------------------------------------------
#if RELEASE
#define DEBUG_MODE						0

// Position feedback / closed loop
#define POSITION_ENABLE                 1   // 0=no encoder, 1=encoder active
#define POSITION_CLOSED_LOOP_ENABLE     1   // Release: closed-loop correction active

// Debug/Test features disabled in release
#define CALIBRATION_MODE                0
#define DEMO_DRAW_MODE					0
#define POSITION_DEBUG					0
#endif


#endif /* CONFIG_BUILD_CONFIG_H_ */
