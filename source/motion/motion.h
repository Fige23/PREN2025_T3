/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
motion.h	Created on: 24.11.2025	   Author: Fige23	Team 3                                                                
*/

#ifndef MOTION_MOTION_H_
#define MOTION_MOTION_H_

#include <stdint.h>
#include <stdbool.h>
#include "protocol.h"
#include "bot_engine.h"
#include "limit_switch.h"

typedef struct {
    uint32_t start_step_rate_sps;
    uint32_t max_step_rate_sps;
    uint32_t accel_sps2;
} motion_profile_s;


err_e motion_start(const bot_action_s *cur, limit_switch_e stop_on_limits, const motion_profile_s *profile_override);

void motion_init(void);
bool motion_is_done(void);
err_e motion_last_err(void);

bool motion_stopped_by_limit(void);
limit_switch_e motion_limit_hit(void);

// Get current ISR tick count for timing synchronization
uint32_t motion_get_isr_tick_count(void);

#endif /* MOTION_MOTION_H_ */
