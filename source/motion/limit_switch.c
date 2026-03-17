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
#include "io.h"
#include "protocol.h"

//endschalter pollen und g_status.limits setzen
void poll_limit_switch(void){

	bool x = limit_switch_x_pressed();
	bool y = limit_switch_y_pressed();
	bool z = limit_switch_z_pressed();

	g_status.limits.x_now = x;
	g_status.limits.y_now = y;
	g_status.limits.z_now = z;

	if(x) g_status.limits.x_latched = true;
	if(y) g_status.limits.y_latched = true;
	if(z) g_status.limits.z_latched = true;
}




//bitlogik schalter zurücksetzen
void reset_limit_switch(limit_switch_e s){
	if(s & limit_x) g_status.limits.x_latched = false;
	if(s & limit_y) g_status.limits.y_latched = false;
	if(s & limit_z) g_status.limits.z_latched = false;
}
