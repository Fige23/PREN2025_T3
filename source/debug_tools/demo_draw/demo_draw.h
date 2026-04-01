/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
demo_draw.h	Created on: 23.03.2026	   Author: Fige23	Team 3                                                                
*/

#ifndef DEMO_DRAW_H_
#define DEMO_DRAW_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    DEMO_PATTERN_STAR = 0,
    DEMO_PATTERN_SQUARE,
    DEMO_PATTERN_TRIANGLE,
    DEMO_PATTERN_SPIRAL,
    DEMO_PATTERN_ZIGZAG,
    DEMO_PATTERN_DIAMOND,
    DEMO_PATTERN_CROSS,
    DEMO_PATTERN_HOUSE,
    DEMO_PATTERN_ARROW,
    DEMO_PATTERN_HEART,
    DEMO_PATTERN_WAVE,
    DEMO_PATTERN_FLOWER,
    DEMO_PATTERN_SMILEY,
    DEMO_PATTERN_INFINITY,
    DEMO_PATTERN_SNOWFLAKE
} demo_pattern_e;
bool demo_enqueue_pattern(demo_pattern_e pattern);


#endif
