/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_))
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|
job_place.c	Created on: 01.04.2026	   Author: Fige23	Team 3
*/

#include "job_place.h"

#include "motion.h"
#include "place_config.h"
#include "io.h"
#include "bot_engine.h"

typedef enum {
	PLACE_STATE_IDLE = 0,
	PLACE_STATE_XY_PHI_MOVE,       // Moving to target XY+phi at SAFE_Z height
	PLACE_STATE_WAITING_XY_PHI,    // Waiting for XY+phi to settle
	PLACE_STATE_Z_DOWN,            // Moving Z down to PLACE_POS
	PLACE_STATE_WAITING_Z_DOWN,    // Waiting for Z to settle
	PLACE_STATE_MAGNET_OFF,        // Turning magnet off
	PLACE_STATE_MAGNET_RELEASE,    // Waiting for magnet to release
	PLACE_STATE_Z_UP,              // Moving Z back up to SAFE_POS
	PLACE_STATE_WAITING_Z_UP,      // Waiting for Z to settle
	PLACE_STATE_DONE,
	PLACE_STATE_ERROR
} place_state_e;

typedef struct {
	place_state_e state;
	robot_pos_s target_pos;     // Full target pose (XY+phi) from the action
	uint16_t wait_counter;      // Cycle counter for timing-free waits
} job_place_s;

static job_place_s jp;

// Motion profiles for Z movements
static const motion_profile_s g_place_z_down_profile = PLACE_Z_DOWN_PROFILE;
static const motion_profile_s g_place_z_up_profile = PLACE_Z_UP_PROFILE;

// Number of step() calls to wait for various phases (assuming ~10ms per call)
#define WAIT_CYCLES_XY_PHI_SETTLE       5    // ~50ms
#define WAIT_CYCLES_Z_SETTLE            5    // ~50ms
#define WAIT_CYCLES_MAGNET_RELEASE     20   // ~200ms (matches PLACE_MAGNET_RELEASE_WAIT_MS)


// ============================================================================
// Helper: Start XY+phi movement to target at SAFE_Z height
// ============================================================================
static err_e place_start_xy_phi_move(const robot_pos_s *target)
{
	bot_action_s a = {0};
	a.type = ACT_PLACE;
	a.target_pos = *target;
	a.target_pos.z_mm_scaled = PLACE_Z_SAFE_POS_MM_SCALED;
	// phi stays as specified in target (rotate to target orientation during XY movement)

	return motion_start(&a, limit_none, 0);
}


// ============================================================================
// Helper: Start Z movement down to PLACE position
// ============================================================================
static err_e place_start_z_down(void)
{
	bot_action_s a = {0};
	a.type = ACT_PLACE;
	a.target_pos = g_status.pos_internal;  // Keep current XY+phi position
	a.target_pos.z_mm_scaled = PLACE_Z_PLACE_POS_MM_SCALED;

	return motion_start(&a, limit_none, &g_place_z_down_profile);
}


// ============================================================================
// Helper: Start Z movement up back to SAFE position
// ============================================================================
static err_e place_start_z_up(void)
{
	bot_action_s a = {0};
	a.type = ACT_PLACE;
	a.target_pos = g_status.pos_internal;  // Keep current XY+phi position
	a.target_pos.z_mm_scaled = PLACE_Z_SAFE_POS_MM_SCALED;

	return motion_start(&a, limit_none, &g_place_z_up_profile);
}


// ============================================================================
// Public API
// ============================================================================

err_e job_place_start(const bot_action_s *a)
{
	if (a == 0) return ERR_INTERNAL;

	jp.state = PLACE_STATE_IDLE;
	jp.target_pos = a->target_pos;
	jp.wait_counter = 0;

	// Start XY+phi movement to target at SAFE_Z height
	err_e me = place_start_xy_phi_move(&a->target_pos);
	if (me != ERR_NONE) {
		jp.state = PLACE_STATE_ERROR;
		return me;
	}

	jp.state = PLACE_STATE_XY_PHI_MOVE;
	return ERR_NONE;
}


bool job_place_step(err_e *out_err)
{
	err_e me;

	switch (jp.state) {

	// --------
	// XY+PHI MOVE PHASE: Moving to target position and orientation at SAFE_Z
	// --------
	case PLACE_STATE_XY_PHI_MOVE:
		if (!motion_is_done()) {
			return false;  // Motion still running
		}

		// Check for motion errors
		me = motion_last_err();
		if (me != ERR_NONE) {
			jp.state = PLACE_STATE_ERROR;
			if (out_err) *out_err = me;
			return true;
		}

		// XY+phi reached, now wait for motion system to stabilize
		jp.state = PLACE_STATE_WAITING_XY_PHI;
		jp.wait_counter = 0;
		return false;


	case PLACE_STATE_WAITING_XY_PHI:
		jp.wait_counter++;
		if (jp.wait_counter < WAIT_CYCLES_XY_PHI_SETTLE) {
			return false;
		}

		// XY+phi is stable, start Z down movement
		me = place_start_z_down();
		if (me != ERR_NONE) {
			jp.state = PLACE_STATE_ERROR;
			if (out_err) *out_err = me;
			return true;
		}

		jp.state = PLACE_STATE_Z_DOWN;
		return false;


	// --------
	// Z DOWN PHASE: Moving Z down to placement position
	// --------
	case PLACE_STATE_Z_DOWN:
		if (!motion_is_done()) {
			return false;  // Motion still running
		}

		// Check for motion errors
		me = motion_last_err();
		if (me != ERR_NONE) {
			jp.state = PLACE_STATE_ERROR;
			if (out_err) *out_err = me;
			return true;
		}

		jp.state = PLACE_STATE_WAITING_Z_DOWN;
		jp.wait_counter = 0;
		return false;


	case PLACE_STATE_WAITING_Z_DOWN:
		jp.wait_counter++;
		if (jp.wait_counter < WAIT_CYCLES_Z_SETTLE) {
			return false;
		}

		// Z is at place position, deactivate magnet
		jp.state = PLACE_STATE_MAGNET_OFF;
		magnet_on_off(false);
		return false;


	// --------
	// MAGNET PHASE: Deactivating magnet and waiting for release
	// --------
	case PLACE_STATE_MAGNET_OFF:
		jp.state = PLACE_STATE_MAGNET_RELEASE;
		jp.wait_counter = 0;
		return false;


	case PLACE_STATE_MAGNET_RELEASE:
		jp.wait_counter++;
		if (jp.wait_counter < WAIT_CYCLES_MAGNET_RELEASE) {
			return false;  // Still waiting for magnet to release
		}

		// Magnet released, start Z up movement
		me = place_start_z_up();
		if (me != ERR_NONE) {
			jp.state = PLACE_STATE_ERROR;
			if (out_err) *out_err = me;
			return true;
		}

		jp.state = PLACE_STATE_Z_UP;
		return false;


	// --------
	// Z UP PHASE: Moving Z back up to safe height
	// --------
	case PLACE_STATE_Z_UP:
		if (!motion_is_done()) {
			return false;  // Motion still running
		}

		// Check for motion errors
		me = motion_last_err();
		if (me != ERR_NONE) {
			jp.state = PLACE_STATE_ERROR;
			if (out_err) *out_err = me;
			return true;
		}

		jp.state = PLACE_STATE_WAITING_Z_UP;
		jp.wait_counter = 0;
		return false;


	case PLACE_STATE_WAITING_Z_UP:
		jp.wait_counter++;
		if (jp.wait_counter < WAIT_CYCLES_Z_SETTLE) {
			return false;
		}

		// Place complete!
		jp.state = PLACE_STATE_DONE;
		if (out_err) *out_err = ERR_NONE;
		return true;


	// --------
	// FINAL STATES
	// --------
	case PLACE_STATE_DONE:
		if (out_err) *out_err = ERR_NONE;
		return true;

	case PLACE_STATE_IDLE:
	case PLACE_STATE_ERROR:
	default:
		if (out_err) *out_err = ERR_INTERNAL;
		return true;
	}
}
