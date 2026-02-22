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

    // Zielpose für MOVE/HOME/PICK/PLACE
    robot_pos_s target_pos;

    // Zusatzdaten je nach Action
    bool     magnet_on;     // nur für ACT_MAGNET
    uint16_t request_id;    // Antwort-Korrelation (ID)
} bot_action_s;

bool bot_enqueue(const bot_action_s *a);
void bot_step(void);                  // im main-Loop aufrufen


#endif /* CONTROLS_BOT_H_ */
