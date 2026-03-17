/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
limit_switch.h	Created on: 17.03.2026	   Author: Fige23	Team 3                                                                
*/

#ifndef MOTION_LIMIT_SWITCH_H_
#define MOTION_LIMIT_SWITCH_H_

/*
 	 	1	1	1	1
		z	y	x	0
*/


typedef enum {
	limit_none = 0,
	limit_x = 1u<<0,
	limit_y = 1u<<1,
	limit_z = 1u<<2
}limit_switch_e;


void poll_limit_switch(void);

void reset_limit_switch(limit_switch_e s);



#endif /* MOTION_LIMIT_SWITCH_H_ */
