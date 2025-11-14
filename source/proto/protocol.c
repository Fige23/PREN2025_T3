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
*/

#include "protocol.h"

//Globaler Status (startwerte)
volatile bot_status_s g_status = {
		.state = STATE_IDLE,
		.homed = false,
		.has_part = false,
		.estop = false,
		.last_err = ERR_NONE,
		.pos = {0,0,0,0}
};

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
