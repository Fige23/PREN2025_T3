/*
 * state.h
 *
 *  Created on: 06.11.2025
 *      Author: felix
 */

#ifndef CONTROL_STATE_H_
#define CONTROL_STATE_H_

#include "protocol.h"

typedef enum {
	STATE_IDLE = 0,
	STATE_HOMING,
	STATE_MOVING,
	STATE_PICKING,
	STATE_PLACING,
	STATE_ERROR,
	STATE_EMERGENCY_STOP
}State_e;

void state_init(void);
void state_task(void);


//von protocol.c aufgerufen bei neuem kommando:
void state_handle_command(const Command_s* cmd);



#endif /* CONTROL_STATE_H_ */
