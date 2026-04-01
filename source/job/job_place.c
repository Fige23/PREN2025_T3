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
	robot_pos_s target_pos;           // Full target pose (XY+phi) from the action
	uint32_t wait_start_tick;         // ISR tick timestamp when wait started
} job_place_s;

static job_place_s jp;

// Motion profiles for Z movements
static const motion_profile_s g_place_z_down_profile = PLACE_Z_DOWN_PROFILE;
static const motion_profile_s g_place_z_up_profile = PLACE_Z_UP_PROFILE;

// Wait cycles derived from config (see place_config.h)
#define WAIT_TICKS_XY_PHI_SETTLE       PLACE_WAIT_TICKS_XY_PHI_SETTLE
#define WAIT_TICKS_Z_SETTLE            PLACE_WAIT_TICKS_Z_SETTLE
#define WAIT_TICKS_MAGNET_RELEASE      PLACE_WAIT_TICKS_MAGNET_RELEASE


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
	jp.wait_start_tick = 0;

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
	uint32_t elapsed;

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
		jp.wait_start_tick = 0;  // Will be initialized when entering WAITING_XY_PHI
		return false;


	case PLACE_STATE_WAITING_XY_PHI:
		// Initialize timestamp on first call to this state
		if (jp.wait_start_tick == 0) {
			jp.wait_start_tick = motion_get_isr_tick_count();
		}

		// Check if enough ISR ticks have elapsed
		elapsed = motion_get_isr_tick_count() - jp.wait_start_tick;
		if (elapsed < WAIT_TICKS_XY_PHI_SETTLE) {
			return false;  // Still waiting
		}

		// XY+phi is stable, start Z down movement
		me = place_start_z_down();
		if (me != ERR_NONE) {
			jp.state = PLACE_STATE_ERROR;
			if (out_err) *out_err = me;
			return true;
		}

		jp.state = PLACE_STATE_Z_DOWN;
		jp.wait_start_tick = 0;  // Reset for next wait
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
		jp.wait_start_tick = 0;  // Will be initialized when entering WAITING_Z_DOWN
		return false;


	case PLACE_STATE_WAITING_Z_DOWN:
		// Initialize timestamp on first call to this state
		if (jp.wait_start_tick == 0) {
			jp.wait_start_tick = motion_get_isr_tick_count();
		}

		// Check if enough ISR ticks have elapsed
		elapsed = motion_get_isr_tick_count() - jp.wait_start_tick;
		if (elapsed < WAIT_TICKS_Z_SETTLE) {
			return false;  // Still waiting
		}

		// Z is at place position, deactivate magnet
		jp.state = PLACE_STATE_MAGNET_OFF;
		jp.wait_start_tick = 0;  // Reset for next wait
		magnet_on_off(false);
		return false;


	// --------
	// MAGNET PHASE: Deactivating magnet and waiting for release
	// --------
	case PLACE_STATE_MAGNET_OFF:
		jp.state = PLACE_STATE_MAGNET_RELEASE;
		jp.wait_start_tick = 0;  // Will be initialized when entering MAGNET_RELEASE
		return false;


	case PLACE_STATE_MAGNET_RELEASE:
		// Initialize timestamp on first call to this state
		if (jp.wait_start_tick == 0) {
			jp.wait_start_tick = motion_get_isr_tick_count();
		}

		// Check if enough ISR ticks have elapsed
		elapsed = motion_get_isr_tick_count() - jp.wait_start_tick;
		if (elapsed < WAIT_TICKS_MAGNET_RELEASE) {
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
		jp.wait_start_tick = 0;  // Reset for next wait
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
		jp.wait_start_tick = 0;  // Will be initialized when entering WAITING_Z_UP
		return false;


	case PLACE_STATE_WAITING_Z_UP:
		// Initialize timestamp on first call to this state
		if (jp.wait_start_tick == 0) {
			jp.wait_start_tick = motion_get_isr_tick_count();
		}

		// Check if enough ISR ticks have elapsed
		elapsed = motion_get_isr_tick_count() - jp.wait_start_tick;
		if (elapsed < WAIT_TICKS_Z_SETTLE) {
			return false;  // Still waiting
		}

		// Place complete!
		jp.state = PLACE_STATE_DONE;
		jp.wait_start_tick = 0;  // Reset for cleanliness
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
