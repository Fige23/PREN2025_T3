/*Project: ${project_name}
 (   (          )   (            )   ) (               )
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_))
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|
${file_name}	Created on: ${date}	   Author: Fige23	Team 3
*/

#ifndef PROTO_PROTOCOL_H_
#define PROTO_PROTOCOL_H_

#include <stdint.h>
#include <stdbool.h>


typedef enum {
	STATE_INIT = 0,
	STATE_IDLE,
	STATE_MOVING,
	STATE_HOMING,
	STATE_PICKING,
	STATE_PLACING,
	STATE_ERROR,
	STATE_EMERGENCY_STOP,
}bot_state_e;

typedef enum {
	ERR_NONE = 0,
	ERR_SYNTAX,
	ERR_RANGE,
	ERR_NO_HOME,
	ERR_NO_PART,
	ERR_PLACE_FAIL,
	ERR_MOTOR,
	ERR_ESTOP,
	ERR_INTERNAL
}err_e;


typedef struct {
	int32_t x_mm, y_mm, z_mm, phi_deg;
}pos_s;

typedef struct {
	bot_state_e state;
	bool homed;
	bool has_part;
	bool estop;
	err_e last_err;
	pos_s pos;
}bot_status_s;

extern volatile bot_status_s g_status;



#endif /* PROTO_PROTOCOL_H_ */
