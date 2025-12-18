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
#include "robot_config.h"
#include "protocol.h"

typedef enum{
	MOVE_PHASE_NONE = 0,
	MOVE_PHASE_Z_PRE,
	MOVE_PHASE_XY_PHI,
	MOVE_PHASE_Z_POST,
	MOVE_PHASE_DONE
}move_phase_e;

typedef struct{
	bool active;
	move_phase_e phase;
	bot_action_s final;

	int32_t z_pre; //z alleine
	int32_t z_xy; //z während xy phase
	bool need_z_pre;
	bool need_z_post;

	err_e last_err;
}job_s;

static job_s j = {0};

static err_e start_segment(move_phase_e ph) {
	bot_action_s seg = j.final; // default: final

	// Für "Z-only" halten wir X/Y/PHI fest, damit wirklich nur Z bewegt wird.
	int32_t cur_x = g_status.pos.x_mm_scaled;
	int32_t cur_y = g_status.pos.y_mm_scaled;
	int32_t cur_phi = g_status.pos.phi_deg_scaled;

	switch (ph) {
	case MOVE_PHASE_Z_PRE:
		seg.x_mm_scaled = cur_x;
		seg.y_mm_scaled = cur_y;
		seg.phi_deg_scaled = cur_phi;
		seg.z_mm_scaled = j.z_pre;
		break;

	case MOVE_PHASE_XY_PHI:
		// X/Y/PHI sollen ans Ziel
		// Z darf (je nach Fall) nur bis z_xy mitlaufen
		seg.z_mm_scaled = j.z_xy;
		break;

	case MOVE_PHASE_Z_POST:
		// Jetzt finalen Z fahren, X/Y/PHI bleiben beim Ziel (seg = final passt)
		// (Falls mini Rundungsabweichung existiert, "snapped" er sauber aufs Ziel)
		break;

	default:
		return ERR_INTERNAL;
	}

	return motion_start(&seg);
}



void job_init(void)
{
    j.active = false;
    j.phase = MOVE_PHASE_NONE;
    j.last_err = ERR_NONE;
}

bool job_is_active(void)
{
    return j.active;
}

err_e job_start_move(const bot_action_s *a)
{
    if (j.active) return ERR_INTERNAL;

    j.final = *a;
    j.last_err = ERR_NONE;

    const int32_t z_cur  = g_status.pos.z_mm_scaled;
    const int32_t z_tgt  = a->z_mm_scaled;
    const int32_t z_safe = SAFE_Z_MAX_DURING_XY;

    j.need_z_pre  = false;
    j.need_z_post = false;
    j.z_pre = z_cur;
    j.z_xy  = z_tgt;

    // ------------------------------------------------------------
    // ABSOLUTE SAFE-Z REGEL:
    // - XY/PHI darf nur laufen, wenn wir in "safe zone" sind: z <= z_safe
    // - Wenn wir aktuell tiefer sind (z_cur > z_safe): zuerst Z-only nach oben
    // - Während XY/PHI: Z darf höchstens bis z_safe (oder wenn Ziel darüber liegt: bis Ziel)
    // - Wenn Ziel tiefer als z_safe: nach XY/PHI Z-only auf Ziel
    // ------------------------------------------------------------

    // 1) Falls wir aktuell zu tief sind -> zuerst Z-only hoch (in safe zone)
    if (z_cur > z_safe) {
        j.need_z_pre = true;

        // Wenn das Ziel bereits innerhalb safe zone liegt, können wir direkt aufs Ziel hochfahren
        // (weil danach sowieso safe erfüllt ist).
        j.z_pre = (z_tgt <= z_safe) ? z_tgt : z_safe;
    }

    // 2) XY/PHI Segment: Z darf nur bis safe zone mitlaufen (oder direkt bis Ziel, wenn Ziel <= safe)
    j.z_xy = (z_tgt <= z_safe) ? z_tgt : z_safe;

    // 3) Wenn Ziel tiefer als safe zone -> nach XY/PHI Z-only nach unten aufs Ziel
    j.need_z_post = (z_tgt > z_safe);

    // Startphase bestimmen
    j.phase = j.need_z_pre ? MOVE_PHASE_Z_PRE : MOVE_PHASE_XY_PHI;

    // Erstes Segment starten
    err_e e = start_segment(j.phase);
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
        return false; // Job läuft noch
    }

    err_e me = motion_last_err();
    if (me != ERR_NONE) {
        j.last_err = me;
        j.active = false;
        if (out_err) *out_err = me;
        return true;
    }

    // Segment OK -> nächste Phase
    if (j.phase == MOVE_PHASE_Z_PRE) {
        j.phase = MOVE_PHASE_XY_PHI;
        err_e e = start_segment(j.phase);
        if (e != ERR_NONE) {
            j.last_err = e;
            j.active = false;
            if (out_err) *out_err = e;
            return true;
        }
        return false;
    }

    if (j.phase == MOVE_PHASE_XY_PHI) {
        if (j.need_z_post) {
            j.phase = MOVE_PHASE_Z_POST;
            err_e e = start_segment(j.phase);
            if (e != ERR_NONE) {
                j.last_err = e;
                j.active = false;
                if (out_err) *out_err = e;
                return true;
            }
            return false;
        }

        // fertig
        j.phase = MOVE_PHASE_DONE;
        j.last_err = ERR_NONE;
        j.active = false;
        if (out_err) *out_err = ERR_NONE;
        return true;
    }

    if (j.phase == MOVE_PHASE_Z_POST) {
        j.phase = MOVE_PHASE_DONE;
        j.last_err = ERR_NONE;
        j.active = false;
        if (out_err) *out_err = ERR_NONE;
        return true;
    }

    // sollte nie passieren
    j.last_err = ERR_INTERNAL;
    j.active = false;
    if (out_err) *out_err = ERR_INTERNAL;
    return true;
}


