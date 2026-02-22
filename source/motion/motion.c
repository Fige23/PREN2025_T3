/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
 (()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
 (_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
 | _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|
 |  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |
 |_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|
 motion.c	Created on: 18.12.2025	   Author: Fige23	Team 3

===============================================================================
motion.c  (Multi-Axis Stepper Pulse Generator)

Aufgabe:
- Generiert Step/Dir Pulse für X/Y/Z/PHI aus einer Zielpose.
- Läuft zeitkritisch über FTM3 Tick-ISR.
- Nutzt DDA/Bresenham-ähnliche Verteilung, damit alle Achsen synchron ankommen.
- Updatet pose_cmd anhand Stepper-Zählung (und aktuell pose_meas = pose_cmd).

Wichtig:
- motion_start(...) startet eine Bewegung.
- motion_is_done()/motion_last_err() melden Completion.
- IO ist gekapselt in io.c (Pins toggeln).
===============================================================================
*/



#include <stdint.h>
#include <stdbool.h>
#include "MK22F51212.h"
#include "platform.h"

#include "io.h"
#include "protocol.h"
#include "bot.h"
#include "robot_config.h"
#include "ftm3.h"

/////TODO:NOCH SCHAUEN WIE IN BOT.C integrieren!

static void motion_tick_isr(void);

//----------tuning----------
#define MOTION_TICK_HZ 50000u			//50kHz -> 20us Tick
#define PULSE_WIDTH_TICKS 1u			//step = high for 1 Tick
#define MIN_STEP_PERIOD_TICKS 2u		//garantiert low zeit
#define DEFAULT_STEP_PERIOD_TICKS 10u	//50k/10 = 5k steps on major axis

typedef enum {
	AX_X = 0, AX_Y = 1, AX_Z = 2, AX_PHI = 3, AX_N = 4
} axis_e;

//entfernt vorzeichen
static inline int32_t iabs32(int32_t v) {
	return (v < 0) ? -v : v;
}

static int32_t scaled_to_steps(int32_t d_scaled, int32_t steps_per_unit,
		int32_t scale) {
	//steps = (d_scaled / scale) * steps_per_unit
	int64_t num = (int64_t) d_scaled * (int64_t) steps_per_unit;
	if (num >= 0)
		return (int32_t) ((num + scale / 2) / scale);
	else
		return (int32_t) ((num - scale / 2) / scale);
}

static void step_set(axis_e ax, bool level) {
	switch (ax) {
	case AX_X:
		stepper_x_step(level);
		break;
	case AX_Y:
		stepper_y_step(level);
		break;
	case AX_Z:
		stepper_z_step(level);
		break;
	case AX_PHI:
		stepper_phi_step(level);
		break;
	case AX_N: break;
	default:
		break;
	}
}

static void dir_set(axis_e ax, bool dir_pos) {
	switch (ax) {
	case AX_X:
		stepper_x_dir(dir_pos);
		break;
	case AX_Y:
		stepper_y_dir(dir_pos);
		break;
	case AX_Z:
		stepper_z_dir(dir_pos);
		break;
	case AX_PHI:
		stepper_phi_dir(dir_pos);
		break;
	case AX_N: break;
	default:
		break;
	}
}

typedef struct {
	volatile bool active;
	volatile bool done;
	volatile err_e err;

	uint32_t step_period_ticks;
	uint32_t tick_div;

	uint32_t major_total;
	uint32_t major_left;

	uint32_t steps_abs[AX_N];
	uint32_t acc[AX_N];

	uint8_t pulse_left[AX_N];

	//für position tracking:
	int64_t pos_num[AX_N];
	int32_t steps_per_unit[AX_N];
	int32_t scale[AX_N];
	int8_t sign[AX_N]; //+1 oder -1 für jede Achse
} motion_s;

static motion_s m = { 0 };

//----------API----------
bool motion_is_done(void) {
	return m.done;
}
err_e motion_last_err(void) {
	return m.err;
}
static void motion_tick_dispatch(void)
{
    // Timer läuft immer – aber Motion-ISR nur, wenn aktiv
    if (!m.active) return;
    motion_tick_isr();
}

void motion_init(void){
	ftm3_tick_set_callback(motion_tick_dispatch);
}

static void motion_finish(err_e err) {
	m.active = false;
	m.done = true;
	m.err = err;

	for (int i = 0; i < AX_N; i++) {
		m.pulse_left[i] = 0;
		step_set((axis_e) i, false);
	}
}

err_e motion_start(const bot_action_s *cur) {

	if (m.active)
		return ERR_INTERNAL;

	//steps_per_unit aus robot_config.h floats
	const int32_t spx = (int32_t) (STEPS_PER_MM_X + 0.5f);//steps_per_unit_x = spx
	const int32_t spy = (int32_t) (STEPS_PER_MM_Y + 0.5f);
	const int32_t spz = (int32_t) (STEPS_PER_MM_Z + 0.5f);
	const int32_t spphi = (int32_t) (STEPS_PER_DEG_PHI + 0.5f);

	//Delta (Ziel - Ist) in scaled units
	int32_t dx_mm_scaled = cur->target_pos.x_mm_scaled - g_status.pos_cmd.x_mm_scaled;
	int32_t dy_mm_scaled = cur->target_pos.y_mm_scaled - g_status.pos_cmd.y_mm_scaled;
	int32_t dz_mm_scaled = cur->target_pos.z_mm_scaled - g_status.pos_cmd.z_mm_scaled;
	int32_t dphi_deg_scaled = cur->target_pos.phi_deg_scaled - g_status.pos_cmd.phi_deg_scaled;

	//In Steps umrechnen:
	int32_t sx = scaled_to_steps(dx_mm_scaled, spx, SCALE_MM);
	int32_t sy = scaled_to_steps(dy_mm_scaled, spy, SCALE_MM);
	int32_t sz = scaled_to_steps(dz_mm_scaled, spz, SCALE_MM);
	int32_t sp = scaled_to_steps(dphi_deg_scaled, spphi, SCALE_DEG);

	//Richtung bzw. Vorzeichen merken
	m.sign[AX_X] = (sx >= 0) ? +1 : -1;
	m.sign[AX_Y] = (sy >= 0) ? +1 : -1;
	m.sign[AX_Z] = (sz >= 0) ? +1 : -1;
	m.sign[AX_PHI] = (sp >= 0) ? +1 : -1;

	dir_set(AX_X, sx >= 0);
	dir_set(AX_Y, sy >= 0);
	dir_set(AX_Z, sz >= 0);
	dir_set(AX_PHI, sp >= 0);

	uint32_t ax = (uint32_t) iabs32(sx);
	uint32_t ay = (uint32_t) iabs32(sy);
	uint32_t az = (uint32_t) iabs32(sz);
	uint32_t ap = (uint32_t) iabs32(sp);

	uint32_t major = ax;
	axis_e major_ax = AX_X;
	if (ay > major) {
		major = ay;
		major_ax = AX_Y;
	}
	if (az > major) {
		major = az;
		major_ax = AX_Z;
	}
	if (ap > major) {
		major = ap;
		major_ax = AX_PHI;
	}

	if (major == 0) {
		m.active = false;
		m.done = true;
		m.err = ERR_NONE;
		return ERR_NONE;
	}
	//Realtime Positionstracking

	m.steps_per_unit[AX_X] = spx;
	m.steps_per_unit[AX_Y] = spy;
	m.steps_per_unit[AX_Z] = spz;
	m.steps_per_unit[AX_PHI] = spphi;

	m.scale[AX_X] = SCALE_MM;
	m.scale[AX_Y] = SCALE_MM;
	m.scale[AX_Z] = SCALE_MM;
	m.scale[AX_PHI] = SCALE_DEG;

	m.pos_num[AX_X] = (int64_t) g_status.pos_cmd.x_mm_scaled
			* (int64_t) m.steps_per_unit[AX_X];
	m.pos_num[AX_Y] = (int64_t) g_status.pos_cmd.y_mm_scaled
			* (int64_t) m.steps_per_unit[AX_Y];
	m.pos_num[AX_Z] = (int64_t) g_status.pos_cmd.z_mm_scaled
			* (int64_t) m.steps_per_unit[AX_Z];
	m.pos_num[AX_PHI] = (int64_t) g_status.pos_cmd.phi_deg_scaled
			* (int64_t) m.steps_per_unit[AX_PHI];

	// State reset
	m.done = false;
	m.err = ERR_NONE;
	m.tick_div = 0;

	m.steps_abs[AX_X] = ax;
	m.steps_abs[AX_Y] = ay;
	m.steps_abs[AX_Z] = az;
	m.steps_abs[AX_PHI] = ap;

	for (int i = 0; i < AX_N; i++) {
		m.acc[i] = 0;
		m.pulse_left[i] = 0;
		step_set((axis_e) i, false);
	}

	m.major_total = major;
	m.major_left = major;

	//speed limit (nicht sicher ob nötig/funktioniert)
	double vmax_steps_s = 1000.0;
	switch (major_ax) {
	case AX_X:
		vmax_steps_s = (double) VMAX_X_MM_S * (double) STEPS_PER_MM_X;
		break;
	case AX_Y:
		vmax_steps_s = (double) VMAX_Y_MM_S * (double) STEPS_PER_MM_Y;
		break;
	case AX_Z:
		vmax_steps_s = (double) VMAX_Z_MM_S * (double) STEPS_PER_MM_Z;
		break;
	case AX_PHI:
		vmax_steps_s = (double) VMAX_PHI_DEG_S * (double) STEPS_PER_DEG_PHI;
		break;
	case AX_N:
		break;
	}
    if (vmax_steps_s < 1.0) vmax_steps_s = 1.0;

    uint32_t period = (uint32_t)((double)STEP_TICK_HZ / vmax_steps_s);
    if (((double)STEP_TICK_HZ / vmax_steps_s) > (double)period) period++;
    if (period < MIN_STEP_PERIOD_TICKS) period = MIN_STEP_PERIOD_TICKS;

    m.step_period_ticks = period;

    m.active = true;

    return ERR_NONE;
}

static void motion_tick_isr(void){


	//step low wenn pulsbreite vorbei
	for(int i = 0; i<AX_N; i++){
		if(m.pulse_left[i]){
			m.pulse_left[i]--;
			if(m.pulse_left[i] == 0){
				step_set((axis_e)i, false);
			}
		}
	}
	//wenn nichts aktiv zurückkehren
	if(!m.active) return;
	//wenn notaus: motoren stoppen und error zurückgeben
	if(g_status.estop || g_status.state == STATE_EMERGENCY_STOP){
		motion_finish(ERR_ESTOP);
		return;
	}

	//entscheiden ob ein step gemacht wird oder nicht:¨
	m.tick_div++;
	if(m.tick_div < m.step_period_ticks){
		//zu früh für nächsten tick
		return;
	}
	m.tick_div = 0; //nicht rausgeflogen: jetzt tick div wieder auf 0 setzen

	uint8_t changed_mask = 0; //welche achsen haben in diesem "slice" einen step gemacht?

	//DDA/Bresenham slice:
	//jede achse hat anzahl steps zu fahren, major_ax ist die achse mit den meisten steps
	//in jedem "slice" werden s (steps_abs) auf einen akkumulator addiert,
	//wenn akkumulator >= major_total -> diese achse bekommt step (major_total wieder abziehen)
	//major achse macht immer einen step, andere achsen nur wenn akkumulator "überläuft"
	//am schluss alle achsen gleichzeitig am ziel

	for(int i = 0; i < AX_N; i++){
		uint32_t s = m.steps_abs[i];
		if(s == 0){
			//diese achse muss nicht bewegt werden
			continue;
		}
		//akkumulator erhöhen
		m.acc[i] += s;

		//wenn über major total: hier bekommt diese achse einen step
		if(m.acc[i] >= m.major_total){
			m.acc[i] -= m.major_total;

			//step puls starten:
			step_set((axis_e)i, true);
			m.pulse_left[i] = PULSE_WIDTH_TICKS;


			//position in echtzeit aktualisieren:
			m.pos_num[i] += (int64_t)m.sign[i] * (int64_t)m.scale[i];
			//achse als geändert markieren:
			changed_mask |= (1u<<i);
		}
	}
	//g_status.pos_cmd wird nur aktualisiert wenn in diesem slice wirklich etwas verändert wurde:
	if(changed_mask & (1u << AX_X)){
		g_status.pos_cmd.x_mm_scaled = (int32_t)(m.pos_num[AX_X] / (int64_t)m.steps_per_unit[AX_X]);
	}
	if(changed_mask & (1u << AX_Y)){
		g_status.pos_cmd.y_mm_scaled = (int32_t)(m.pos_num[AX_Y] / (int64_t)m.steps_per_unit[AX_Y]);
	}
	if(changed_mask & (1u << AX_Z)){
		g_status.pos_cmd.z_mm_scaled = (int32_t)(m.pos_num[AX_Z] / (int64_t)m.steps_per_unit[AX_Z]);
	}
	if(changed_mask & (1u << AX_PHI)){
		g_status.pos_cmd.phi_deg_scaled = (int32_t)(m.pos_num[AX_PHI] / (int64_t)m.steps_per_unit[AX_PHI]);
	}

	g_status.pos_measured = g_status.pos_cmd; 		//noch keine encoder

	//major left zählt herunter wieviele major steps noch fehlen:
	if(--m.major_left == 0){
		motion_finish(ERR_NONE);
	}
}






//TODO: start funktionen implementieren:
err_e home_start(bot_action_s *cur){
	(void)cur;
	return ERR_INTERNAL;
}






