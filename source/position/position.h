/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
position.h	Created on: 25.02.2026	   Author: Fige23	Team 3                                                                
*/

#ifndef POSITION_POSITION_H_
#define POSITION_POSITION_H_

#include <stdint.h>
#include <stdbool.h>
#include "protocol.h"   // robot_pos_s, g_status

void position_init(void);
void position_poll(void);

int32_t position_get_x_mm_scaled(void);
int32_t position_get_y_mm_scaled(void);

int32_t position_get_x_counts(void);
int32_t position_get_y_counts(void);

// Setzt Koordinatensystem (Offset) so, dass measured == gewünschter Wert
void position_set_xy_mm_scaled(int32_t x_mm_scaled, int32_t y_mm_scaled);

// Praktisch nach Homing: measured an pos_cmd anpassen (Offset)
void position_sync_measured_to_cmd(void);

#endif /* POSITION_POSITION_H_ */
