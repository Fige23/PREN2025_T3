/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
bot.h	Created on: 14.11.2025	   Author: Fige23	Team 3                                                                
*/

#ifndef CONTROLS_BOT_H_
#define CONTROLS_BOT_H_

#include <stdint.h>
#include <stdbool.h>
#include "protocol.h"

typedef enum {
  ACT_MOVE, ACT_HOME, ACT_PICK, ACT_PLACE, ACT_MAGNET
} bot_action_e;

typedef struct {
  bot_action_e type;
  // Ziele in Fixed-Point:
  int32_t x_001mm, y_001mm, z_001mm;  // 0.001 mm
  int32_t phi_001deg;                 // 0.01°
  bool    on;                         // für MAGNET
  uint16_t req_id;                    // Antwort-Korrelation
} bot_action_s;

bool bot_enqueue(const bot_action_s *a);
void bot_step(void);                  // im main-Loop aufrufen


#endif /* CONTROLS_BOT_H_ */
