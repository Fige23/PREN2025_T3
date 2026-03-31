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
Config file: Dieses File inkludiert alle kleineren Config Files!
=====================================================================
*/

#ifndef CONFIG_ROBOT_CONFIG_H_
#define CONFIG_ROBOT_CONFIG_H_

#include <stdint.h>

#include "build_config.h"
#include "calibration_config.h"
#include "communication_config.h"
#include "encoder_config.h"
#include "hardware_config.h"
#include "home_config.h"
#include "motion_config.h"

/* ============================================================================
 * CONFIG CHECKS
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

#if (UART_USBC_ENABLE != 0) && (UART_USBC_ENABLE != 1)
#error "UART_USBC_ENABLE must be 0 or 1"
#endif

#if (ENABLE_CONSOLE_UART_SIM != 0) && (ENABLE_CONSOLE_UART_SIM != 1)
#error "ENABLE_CONSOLE_UART_SIM must be 0 or 1"
#endif

#endif /* CONFIG_ROBOT_CONFIG_H_ */
