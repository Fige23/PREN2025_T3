/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
limit_switch.c	Created on: 17.03.2026	   Author: Fige23	Team 3                                                                
*/

#include "limit_switch.h"
#include "robot_config.h"

#include "io.h"
#include "protocol.h"

typedef struct {
	bool initialized;
	bool stable;
	uint16_t changed_count;
} limit_filter_s;

static limit_filter_s g_limit_x = {0};
static limit_filter_s g_limit_y = {0};
static limit_filter_s g_limit_z = {0};

static bool filter_limit_sample(limit_filter_s *filter, bool raw){
#if LIMIT_SWITCH_FILTER_ENABLE
	uint16_t required = raw ? LIMIT_SWITCH_PRESS_POLLS
	                        : LIMIT_SWITCH_RELEASE_POLLS;

	if(!filter->initialized){
		filter->initialized = true;
		filter->stable = raw;
		filter->changed_count = 0u;
		return filter->stable;
	}

	if(raw == filter->stable){
		filter->changed_count = 0u;
		return filter->stable;
	}

	if(filter->changed_count < required){
		filter->changed_count++;
	}

	if(filter->changed_count >= required){
		filter->stable = raw;
		filter->changed_count = 0u;
	}

	return filter->stable;
#else
	(void)filter;
	return raw;
#endif
}

static void publish_limit_state(bool x, bool y, bool z){
	g_status.limits.x_now = x;
	g_status.limits.y_now = y;
	g_status.limits.z_now = z;

	if(x) g_status.limits.x_latched = true;
	if(y) g_status.limits.y_latched = true;
	if(z) g_status.limits.z_latched = true;
}

//endschalter pollen und g_status.limits setzen
void poll_limit_switch(void){
	bool x = limit_switch_x_pressed();
	bool y = limit_switch_y_pressed();
	bool z = limit_switch_z_pressed();

	#if LIMIT_SWITCH_FILTER_ENABLE
	x = filter_limit_sample(&g_limit_x, x);
	y = filter_limit_sample(&g_limit_y, y);
	z = filter_limit_sample(&g_limit_z, z);
	#endif

	publish_limit_state(x, y, z);
}




//bitlogik schalter zurücksetzen
void reset_limit_switch(limit_switch_e s){
	if(s & limit_x) g_status.limits.x_latched = false;
	if(s & limit_y) g_status.limits.y_latched = false;
	if(s & limit_z) g_status.limits.z_latched = false;
}
