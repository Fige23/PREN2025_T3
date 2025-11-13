/*Project: ${project_name}
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
#ifndef CONTROL_PROTOCOL_H_
#define CONTROL_PROTOCOL_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum {
	CMD_NONE,
	CMD_PING,
	CMD_HOME,
	CMD_MOVE,
	CMD_PICK,
	CMD_PLACE,
	CMD_MAGNET,
	CMD_STATUS,
	CMD_POS,
	CMD_RESET,
} CommandId_e;

typedef enum {
	ERR_NONE = 0,
	ERR_SYNTAX,
	ERR_RANGE,
	ERR_NO_HOME,
	ERR_NO_PART,
	ERR_PLACE_FAIL,
	ERR_NOTAUS_AKTIV,
	ERR_MOTOR,
	ERR_INTERNAL
} ErrorCode_e;

typedef struct {
	CommandId_e cmd;
	int no;
	float x_mm;
	float y_mm;
	float z_mm;
	bool magnet_on;
}Command_s;




//Von main/state aufrufen:
void protocol_init(void);
void protocol_task(void); //in main loop aufrufen
void protocol_get_command(Command_s* out_cmd);


//antworten senden:
void protocol_send_ok(CommandId_e cmd);
void protocol_send_err(CommandId_e cmd, ErrorCode_e err);
void protocol_send_busy(void);
void protocol_send_state(const char* state_str);
void protocol_send_pos(float x_mm, float y_mm, float z_mm);





#endif /* CONTROL_PROTOCOL_H_ */
