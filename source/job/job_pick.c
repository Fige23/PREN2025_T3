/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_))
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|
job_pick.c	Created on: 01.04.2026	   Author: Fige23	Team 3
*/

#include "job_pick.h"

#include "motion.h"
#include "pick_config.h"
#include "io.h"
#include "bot_engine.h"

typedef enum {
	PICK_STATE_IDLE = 0,
	PICK_STATE_XY_MOVE,        // Moving to target XY at SAFE_Z height
	PICK_STATE_WAITING_XY,     // Waiting for XY to settle (counter-based)
	PICK_STATE_Z_DOWN,         // Moving Z down to PICK_POS
	PICK_STATE_WAITING_Z_DOWN, // Waiting for Z to settle (counter-based)
	PICK_STATE_MAGNET_ON,      // Turning magnet on
	PICK_STATE_MAGNET_WAIT,    // Waiting for magnet to grab (counter-based)
	PICK_STATE_Z_UP,           // Moving Z back up to SAFE_POS
	PICK_STATE_WAITING_Z_UP,   // Waiting for Z to settle (counter-based)
	PICK_STATE_DONE,
	PICK_STATE_ERROR
} pick_state_e;

typedef struct {
	pick_state_e state;
	robot_pos_s target_xy;      // XY target from the action
	uint16_t wait_counter;      // Cycle counter for timing-free waits
} job_pick_s;

static job_pick_s jp;

// Motion profiles for Z movements
static const motion_profile_s g_pick_z_down_profile = PICK_Z_DOWN_PROFILE;
static const motion_profile_s g_pick_z_up_profile = PICK_Z_UP_PROFILE;

// Wait cycles derived from config (see pick_config.h)
#define WAIT_CYCLES_XY_SETTLE       PICK_WAIT_CYCLES_XY_SETTLE
#define WAIT_CYCLES_Z_SETTLE        PICK_WAIT_CYCLES_Z_SETTLE
#define WAIT_CYCLES_MAGNET_GRAB     PICK_WAIT_CYCLES_MAGNET_GRAB


// ============================================================================
// Helper: Start XY+phi movement to target at SAFE_Z height, phi reset to 0
// ============================================================================
static err_e pick_start_xy_move(const robot_pos_s *target)
{
	bot_action_s a = {0};
	a.type = ACT_PICK;
	a.target_pos = *target;
	a.target_pos.z_mm_scaled = PICK_Z_SAFE_POS_MM_SCALED;
	a.target_pos.phi_deg_scaled = 0;  // Reset phi to 0 during XY movement

	return motion_start(&a, limit_none, 0);
}


// ============================================================================
// Helper: Start Z movement down to PICK position
// ============================================================================
static err_e pick_start_z_down(void)
{
	bot_action_s a = {0};
	a.type = ACT_PICK;
	a.target_pos = g_status.pos_internal;  // Keep current XY position
	a.target_pos.z_mm_scaled = PICK_Z_PICK_POS_MM_SCALED;

	return motion_start(&a, limit_none, &g_pick_z_down_profile);
}


// ============================================================================
// Helper: Start Z movement up back to SAFE position
// ============================================================================
static err_e pick_start_z_up(void)
{
	bot_action_s a = {0};
	a.type = ACT_PICK;
	a.target_pos = g_status.pos_internal;  // Keep current XY position
	a.target_pos.z_mm_scaled = PICK_Z_SAFE_POS_MM_SCALED;

	return motion_start(&a, limit_none, &g_pick_z_up_profile);
}


// ============================================================================
// Public API
// ============================================================================

err_e job_pick_start(const bot_action_s *a)
{
	if (a == 0) return ERR_INTERNAL;

	jp.state = PICK_STATE_IDLE;
	jp.target_xy = a->target_pos;
	jp.wait_counter = 0;

	// Start XY movement to target at SAFE_Z height
	err_e me = pick_start_xy_move(&a->target_pos);
	if (me != ERR_NONE) {
		jp.state = PICK_STATE_ERROR;
		return me;
	}

	jp.state = PICK_STATE_XY_MOVE;
	return ERR_NONE;
}


bool job_pick_step(err_e *out_err)
{
	err_e me;

	switch (jp.state) {

	// --------
	// XY MOVE PHASE: Moving to target position at SAFE_Z
	// --------
	case PICK_STATE_XY_MOVE:
		if (!motion_is_done()) {
			return false;  // Motion still running
		}

		// Check for motion errors
		me = motion_last_err();
		if (me != ERR_NONE) {
			jp.state = PICK_STATE_ERROR;
			if (out_err) *out_err = me;
			return true;
		}

		// XY reached, now wait for motion system to stabilize
		jp.state = PICK_STATE_WAITING_XY;
		jp.wait_counter = 0;
		return false;


	case PICK_STATE_WAITING_XY:
		jp.wait_counter++;
		if (jp.wait_counter < WAIT_CYCLES_XY_SETTLE) {
			return false;
		}

		// XY is stable, start Z down movement
		me = pick_start_z_down();
		if (me != ERR_NONE) {
			jp.state = PICK_STATE_ERROR;
			if (out_err) *out_err = me;
			return true;
		}

		jp.state = PICK_STATE_Z_DOWN;
		return false;


	// --------
	// Z DOWN PHASE: Moving Z down to piece
	// --------
	case PICK_STATE_Z_DOWN:
		if (!motion_is_done()) {
			return false;  // Motion still running
		}

		// Check for motion errors
		me = motion_last_err();
		if (me != ERR_NONE) {
			jp.state = PICK_STATE_ERROR;
			if (out_err) *out_err = me;
			return true;
		}

		jp.state = PICK_STATE_WAITING_Z_DOWN;
		jp.wait_counter = 0;
		return false;


	case PICK_STATE_WAITING_Z_DOWN:
		jp.wait_counter++;
		if (jp.wait_counter < WAIT_CYCLES_Z_SETTLE) {
			return false;
		}

		// Z is at pick position, activate magnet
		jp.state = PICK_STATE_MAGNET_ON;
		magnet_on_off(true);
		return false;


	// --------
	// MAGNET PHASE: Activating magnet and waiting for grab
	// --------
	case PICK_STATE_MAGNET_ON:
		jp.state = PICK_STATE_MAGNET_WAIT;
		jp.wait_counter = 0;
		return false;


	case PICK_STATE_MAGNET_WAIT:
		jp.wait_counter++;
		if (jp.wait_counter < WAIT_CYCLES_MAGNET_GRAB) {
			return false;  // Still waiting
		}

		// Magnet grabbed, start Z up movement
		me = pick_start_z_up();
		if (me != ERR_NONE) {
			jp.state = PICK_STATE_ERROR;
			magnet_on_off(false);  // Safety: turn off magnet on error
			if (out_err) *out_err = me;
			return true;
		}

		jp.state = PICK_STATE_Z_UP;
		return false;


	// --------
	// Z UP PHASE: Moving Z back up to safe height
	// --------
	case PICK_STATE_Z_UP:
		if (!motion_is_done()) {
			return false;  // Motion still running
		}

		// Check for motion errors
		me = motion_last_err();
		if (me != ERR_NONE) {
			jp.state = PICK_STATE_ERROR;
			magnet_on_off(false);  // Safety: turn off magnet on error
			if (out_err) *out_err = me;
			return true;
		}

		jp.state = PICK_STATE_WAITING_Z_UP;
		jp.wait_counter = 0;
		return false;


	case PICK_STATE_WAITING_Z_UP:
		jp.wait_counter++;
		if (jp.wait_counter < WAIT_CYCLES_Z_SETTLE) {
			return false;
		}

		// Pick complete!
		jp.state = PICK_STATE_DONE;
		if (out_err) *out_err = ERR_NONE;
		return true;


	// --------
	// FINAL STATES
	// --------
	case PICK_STATE_DONE:
		if (out_err) *out_err = ERR_NONE;
		return true;

	case PICK_STATE_IDLE:
	case PICK_STATE_ERROR:
	default:
		if (out_err) *out_err = ERR_INTERNAL;
		return true;
	}
}
