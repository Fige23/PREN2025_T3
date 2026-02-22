/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
job.c	Created on: 18.12.2025	   Author: Fige23	Team 3                                                                
*/
#include "job.h"

#include "motion.h"
#include "protocol.h"

/*
===============================================================================
job.c  (Job Sequencing)

Aufgabe:
- Kapselt Bewegungssequenzen, die aus mehreren Segmenten bestehen können.
- Aktuell:
    - MOVE = 1 Segment direkt auf Zielpose
- Später:
    - PICK/PLACE = Sequenz aus mehreren Waypoints (Z safe -> XY -> Z down -> Magnet -> Z up)

Warum separates Modul?
- bot.c bleibt simpel (start/step/fertig).
- Komplexität (Waypoints/Phasen) bleibt an einem Ort.
===============================================================================
*/

#include "job.h"
#include "motion.h"

typedef struct {
    bool active;
    err_e last_err;
} job_s;

static job_s j = {0};

void job_init(void)
{
    j.active = false;
    j.last_err = ERR_NONE;
}

bool job_is_active(void)
{
    return j.active;
}

err_e job_start_move(const bot_action_s *a)
{
    if (j.active) return ERR_INTERNAL;

    j.last_err = ERR_NONE;

    err_e e = motion_start(a);   // <-- direkt, kein Segmenting mehr
    if (e != ERR_NONE) {
        j.active = false;
        j.last_err = e;
        return e;
    }

    j.active = true;
    return ERR_NONE;
}

bool job_step(err_e *out_err)
{
    if (!j.active) {
        if (out_err) *out_err = j.last_err;
        return true;
    }

    if (!motion_is_done()) {
        return false;
    }

    j.last_err = motion_last_err();
    j.active = false;

    if (out_err) *out_err = j.last_err;
    return true;
}
