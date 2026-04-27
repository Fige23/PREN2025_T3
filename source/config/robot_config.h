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
#include "geometry_config.h"
#include "calibration_config.h"
#include "communication_config.h"
#include "encoder_config.h"
#include "home_config.h"
#include "motion_config.h"
#include "tmc2209_config.h"

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

#if (ENABLE_CONSOLE_UART_SIM != 0) && (ENABLE_CONSOLE_UART_SIM != 1)
#error "ENABLE_CONSOLE_UART_SIM must be 0 or 1"
#endif

#if (ESTOP_POLL_MODE != ESTOP_POLL_MODE_EDGE) && (ESTOP_POLL_MODE != ESTOP_POLL_MODE_LEVEL_LATCH)
#error "ESTOP_POLL_MODE must be ESTOP_POLL_MODE_EDGE or ESTOP_POLL_MODE_LEVEL_LATCH"
#endif

#if (ESTOP_POLL_IN_MOTION_ISR != 0) && (ESTOP_POLL_IN_MOTION_ISR != 1)
#error "ESTOP_POLL_IN_MOTION_ISR must be 0 or 1"
#endif

#if (ESTOP_POLL_ISR_DIVIDER < 1u)
#error "ESTOP_POLL_ISR_DIVIDER must be >= 1"
#endif

#if (TMC2209_ENABLE != 0) && (TMC2209_ENABLE != 1)
#error "TMC2209_ENABLE must be 0 or 1"
#endif

#if (TMC2209_MICROSTEPS_MOVE != 1u) && (TMC2209_MICROSTEPS_MOVE != 2u) && \
    (TMC2209_MICROSTEPS_MOVE != 4u) && (TMC2209_MICROSTEPS_MOVE != 8u) && \
    (TMC2209_MICROSTEPS_MOVE != 16u) && (TMC2209_MICROSTEPS_MOVE != 32u) && \
    (TMC2209_MICROSTEPS_MOVE != 64u) && (TMC2209_MICROSTEPS_MOVE != 128u) && \
    (TMC2209_MICROSTEPS_MOVE != 256u)
#error "TMC2209_MICROSTEPS_MOVE must be a supported TMC2209 microstep value"
#endif

#if (TMC2209_MICROSTEPS_CORRECTION != 1u) && (TMC2209_MICROSTEPS_CORRECTION != 2u) && \
    (TMC2209_MICROSTEPS_CORRECTION != 4u) && (TMC2209_MICROSTEPS_CORRECTION != 8u) && \
    (TMC2209_MICROSTEPS_CORRECTION != 16u) && (TMC2209_MICROSTEPS_CORRECTION != 32u) && \
    (TMC2209_MICROSTEPS_CORRECTION != 64u) && (TMC2209_MICROSTEPS_CORRECTION != 128u) && \
    (TMC2209_MICROSTEPS_CORRECTION != 256u)
#error "TMC2209_MICROSTEPS_CORRECTION must be a supported TMC2209 microstep value"
#endif

#if (TMC2209_DEFAULT_FREEWHEEL > 3u)
#error "TMC2209_DEFAULT_FREEWHEEL must be 0..3"
#endif

#if (TMC2209_IHOLDDELAY > 15u)
#error "TMC2209_IHOLDDELAY must be 0..15"
#endif

#endif /* CONFIG_ROBOT_CONFIG_H_ */
