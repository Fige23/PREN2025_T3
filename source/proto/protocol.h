/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_))
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|
protocol.h
Created on: 13.11.2025
Author: Fige23
Team 3
*/

#ifndef PROTO_PROTOCOL_H_
#define PROTO_PROTOCOL_H_

#include <stdint.h>
#include <stdbool.h>




//States und errors

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


// Position intern in Fixed-Point
// x/y/z: 0.001 mm (= 1 µm)
// phi:   0.001°
typedef struct {
    int32_t x_001mm;
    int32_t y_001mm;
    int32_t z_001mm;
    int32_t phi_001deg;
} pos_s;



typedef struct {
	bot_state_e state;
	bool homed;
	bool has_part;
	bool estop;
	err_e last_err;
	pos_s pos;
}bot_status_s;

extern volatile bot_status_s g_status;
//string helpers
const char *err_to_str(err_e e);
const char *state_to_str(bot_state_e s);

#endif /* PROTO_PROTOCOL_H_ */
