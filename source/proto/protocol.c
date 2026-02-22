/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
protocol.c	Created on: 13.11.2025	   Author: Fige23	Team 3                                                                

===============================================================================
protocol.[h|c]

Aufgabe:
- Zentrale Definition von States/Errors und dem globalen Status g_status.
- g_status ist die gemeinsame Wahrheit für:
    state/homed/has_part/estop/last_err
    pose_cmd  (Stepper-Zählung / commanded)
    pose_meas (Encoder/Measured, später)

Warum pose_cmd + pose_meas?
- Heute: ohne Encoder -> pose_meas wird gleich pose_cmd gehalten.
- Später: AS5311/Encoder -> pose_meas kommt aus Sensor, pose_cmd bleibt Stepper-basiert.
===============================================================================
*/


#include "robot_config.h"
#include "protocol.h"

// Globaler Status mit Startwerten nach Boot.
// Wird als volatile geführt, weil er aus verschiedenen Kontexten
// (Main-Loop / später ISR / Bot-Engine) gelesen/geschrieben wird.
volatile bot_status_s g_status = {
    .state = STATE_IDLE,
    .homed = false,
    .has_part = false,
    .estop = false,
    .last_err = ERR_NONE,

    .pos_cmd  = { .x_mm_scaled=0, .y_mm_scaled=0, .z_mm_scaled=0, .phi_deg_scaled=0 },
    .pos_measured = { .x_mm_scaled=0, .y_mm_scaled=0, .z_mm_scaled=0, .phi_deg_scaled=0 }, //z, phi immer null: ungemessen
};

// Fehlercode -> Protokoll-String.
// Diese Strings gehen 1:1 über UART zurück (ERR ...).
const char* err_to_str(err_e e) {

	switch (e) {
	case ERR_NONE:
		return "NONE";
	case ERR_SYNTAX:
		return "SYNTAX";
	case ERR_RANGE:
		return "RANGE";
	case ERR_NO_HOME:
		return "NO_HOME";
	case ERR_NO_PART:
		return "NO_PART";
	case ERR_PLACE_FAIL:
		return "PLACE_FAIL";
	case ERR_MOTOR:
		return "MOTOR";
	case ERR_ESTOP:
		return "ESTOP";
	default:
		return "INTERNAL";
	}
}

// State -> Protokoll-String.
// Wird z.B. in STATUS ausgegeben, damit am Pi klar ist, was gerade läuft.
const char* state_to_str(bot_state_e s) {
	switch (s) {
	case STATE_INIT:
		return "INIT";
	case STATE_IDLE:
		return "IDLE";
	case STATE_MOVING:
		return "MOVING";
	case STATE_HOMING:
		return "HOMING";
	case STATE_PICKING:
		return "PICKING";
	case STATE_PLACING:
		return "PLACING";
	case STATE_ERROR:
		return "ERROR";
	case STATE_EMERGENCY_STOP:
		return "EMERGENCY_STOP";
	default:
		return "UNKNOWN";
	}
}
