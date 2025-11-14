/*Project: ${project_name}
 (   (          )   (            )   ) (               )
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )
 (()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_))
 (_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
 | _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|
 |  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |
 |_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|
 cmd.c
 Created on: 12.11.2025
 Author: Fige23
 Team 3
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stddef.h>

#include "serial_port.h"
#include "protocol.h"   // g_status, state_to_str(), err_to_str(), Limits/Scales
#include "parse_kv.h"   // kv_fixed_any_lower(), kv_fixed_spec_s
#include "bot.h"        // bot_enqueue(), bot_action_s, bot_action_e

#ifndef CMD_LINE_MAX
#define CMD_LINE_MAX 128
#endif

// -----------------------------------------------------------------------------
// UART reply helpers
// -----------------------------------------------------------------------------
static const char *EOL = "\n";   // End-of-line for replies

static void replyf(const char *fmt, ...) {
	char buf[192];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	serial_puts(buf);
}

static void send_ok(const char *cmd) {
	replyf("OK %s%s", cmd, EOL);
}
static void send_err(const char *cmd, const char *e) {
	replyf("ERR %s %s%s", cmd, e, EOL);
}

static uint16_t s_next_req_id = 1;
static void send_queued(const char *cmd, uint16_t id) {
	replyf("QUEUED %s id=%u%s", cmd, (unsigned) id, EOL);
}

static const char* yesno(bool b) {
	return b ? "YES" : "NO";
}

// -----------------------------------------------------------------------------
// Command handlers
// -----------------------------------------------------------------------------
typedef bool (*cmd_handler_fp_t)(int argc, char **argv);

typedef struct {
	const char *name;  // Befehl in UPPERCASE
	cmd_handler_fp_t fn;
} cmd_entry_s;

// --- PING ---
static bool cmd_ping(int argc, char **argv) {
	(void) argc;
	(void) argv;
	send_ok("PING");
	return true;
}

// --- STATUS ---
static bool cmd_status(int argc, char **argv) {
	(void) argc;
	(void) argv;
	replyf("STATUS state=%s (%d) homed=%s part=%s estop=%s err=%s "
			"POS x=%ld y=%ld z=%ld phi=%ld%s", state_to_str(g_status.state),
			(int) g_status.state, yesno(g_status.homed),
			yesno(g_status.has_part), yesno(g_status.estop),
			err_to_str(g_status.last_err), (long) g_status.pos.x_mm,
			(long) g_status.pos.y_mm, (long) g_status.pos.z_mm,
			(long) g_status.pos.phi_deg, EOL);
	return true;
}

// --- POS ---
static bool cmd_pos(int argc, char **argv) {
	(void) argc;
	(void) argv;
	replyf("POS x=%ld y=%ld z=%ld phi=%ld%s", (long) g_status.pos.x_mm,
			(long) g_status.pos.y_mm, (long) g_status.pos.z_mm,
			(long) g_status.pos.phi_deg, EOL);
	return true;
}

// --- HOME (asynchron) ---
static bool cmd_home(int argc, char **argv) {
	(void) argv;
	if (argc != 1) {
		send_err("HOME", "SYNTAX");
		return false;
	}
	if (g_status.estop) {
		send_err("HOME", "ESTOP");
		return false;
	}

	bot_action_s a = {
			.type = ACT_HOME,
			.req_id = s_next_req_id++
	};
	if (!bot_enqueue(&a)) {
		send_err("HOME", "INTERNAL");
		return false;
	}

	send_queued("HOME", a.req_id);
	return true;
}

// --- MAGNET ON|OFF (asynchron) ---
static bool cmd_magnet(int argc, char **argv) {
	if (argc != 2) {
		send_err("MAGNET", "SYNTAX");
		return false;
	}
	bool on;
	if (strcmp(argv[1], "ON") == 0)
		on = true;
	else if (strcmp(argv[1], "OFF") == 0)
		on = false;
	else {
		send_err("MAGNET", "SYNTAX");
		return false;
	}

	bot_action_s a = {
			.type = ACT_MAGNET,
			.on = on,
			.req_id = s_next_req_id++
	};

	if (!bot_enqueue(&a)) {
		send_err("MAGNET", "INTERNAL");
		return false;
	}

	send_queued("MAGNET", a.req_id);
	return true;
}

// --- MOVE x= y= z= phi=  (Floats erlaubt; intern Fixed-Point, asynchron) ---
static bool cmd_move(int argc, char **argv){
    if (argc < 2)        { send_err("MOVE","SYNTAX");  return false; }
    if (g_status.estop)  { send_err("MOVE","ESTOP");   return false; }
    if (!g_status.homed) { send_err("MOVE","NO_HOME"); return false; }

    int32_t x_s  = g_status.pos.x_mm    * SCALE_MM;
    int32_t y_s  = g_status.pos.y_mm    * SCALE_MM;
    int32_t z_s  = g_status.pos.z_mm    * SCALE_MM;
    int32_t ph_s = g_status.pos.phi_deg * SCALE_DEG;

    // MOVE: beliebige Teilmenge erlaubt, aber mindestens 1 Key
    err_e e = parse_pos_tokens_mask(
        argc, argv, 1, &x_s, &y_s, &z_s, &ph_s,
        /*require_mask=*/0,                              // mind. 1 Key, aber beliebige Teilmenge
        /*allowed_mask=*/KV_X | KV_Y | KV_Z | KV_PHI,    // alle vier erlaubt
        /*seen_out=*/NULL
    );
    if (e != ERR_NONE) { send_err("MOVE", err_to_str(e)); return false; }

    bot_action_s a = {
    		.type=ACT_MOVE,
			.x_001mm=x_s,
			.y_001mm=y_s,
            .z_001mm=z_s,
			.phi_001deg=ph_s,
			.req_id=s_next_req_id++
    };

    if (!bot_enqueue(&a)) { send_err("MOVE","QUEUE_FULL"); return false; }
    send_queued("MOVE", a.req_id);
    return true;
}


// --- PICK / PLACE (asynchron, hier noch ohne Parameter) ---
static bool cmd_pick(int argc, char **argv){
    if (g_status.estop)  { send_err("PICK","ESTOP");   return false; }
    if (!g_status.homed) { send_err("PICK","NO_HOME"); return false; }
    if (g_status.has_part) { send_err("PICK","RANGE"); return false; } // schon beladen

    // Für PICK keine Defaults → alles explizit
    int32_t x_s=0, y_s=0, z_s=0, ph_s=0;

    // Pflicht: x,y,z — Erlaubt: x,y,z,phi (phi optional)
    err_e e = parse_pos_tokens_mask(
        argc, argv, 1,
        &x_s, &y_s, &z_s, &ph_s,
        /*require_mask=*/KV_X | KV_Y,                   // x,y Pflicht
        /*allowed_mask=*/KV_X | KV_Y | KV_PHI,          // z NICHT erlaubt
        /*seen_out=*/NULL
    );
    if (e != ERR_NONE) { send_err("PICK", err_to_str(e)); return false; }

    // phi wird akzeptiert, aber von der Engine ggf. ignoriert:
    bot_action_s a = {
    		.type=ACT_PICK,
			.x_001mm=x_s,
			.y_001mm=y_s,
            .z_001mm=z_s,
			.phi_001deg=ph_s,
			.req_id=s_next_req_id++
    };

    if (!bot_enqueue(&a)) { send_err("PICK","QUEUE_FULL"); return false; }
    send_queued("PICK", a.req_id);
    return true;
}


static bool cmd_place(int argc, char **argv){
    if (g_status.estop)		{ send_err("PLACE","ESTOP");   return false; }
    if (!g_status.homed)    { send_err("PLACE","NO_HOME"); return false; }
    if (!g_status.has_part) { send_err("PLACE","NO_PART"); return false; }

    int32_t x_s=0, y_s=0, z_s=0, ph_s=0;

    err_e e = parse_pos_tokens_mask(
        argc, argv, 1,
        &x_s, &y_s, &z_s, &ph_s,
        /*require_mask=*/KV_X | KV_Y | KV_PHI,          // x,y,phi Pflicht
        /*allowed_mask=*/KV_X | KV_Y | KV_PHI,          // z NICHT erlaubt
        /*seen_out=*/NULL
    );

    if (e != ERR_NONE) { send_err("PLACE", err_to_str(e)); return false; }
    bot_action_s a = {
    		.type=ACT_PLACE,
			.x_001mm=x_s,
			.y_001mm=y_s,
            .z_001mm=z_s,
			.phi_001deg=ph_s,
			.req_id=s_next_req_id++
    };

    if (!bot_enqueue(&a)) { send_err("PLACE","QUEUE_FULL"); return false; }
    send_queued("PLACE", a.req_id);
    return true;
}


// --- RESET (synchron – setzt nur lokalen Status zurück) ---
static bool cmd_reset(int argc, char **argv) {
	(void) argc;
	(void) argv;
	g_status.state = STATE_IDLE;
	g_status.has_part = false;
	g_status.homed = false;
	g_status.estop = false;
	g_status.last_err = ERR_NONE;
	send_ok("RESET");
	return true;
}

// -----------------------------------------------------------------------------
// Command table
// -----------------------------------------------------------------------------
static const cmd_entry_s s_cmds[] = {
		{ "PING", cmd_ping },
		{ "STATUS", cmd_status },
		{ "POS", cmd_pos },
		{ "HOME", cmd_home },
		{ "MAGNET", cmd_magnet },
		{ "MOVE", cmd_move },
		{ "PICK", cmd_pick },
		{ "PLACE", cmd_place },
		{ "RESET", cmd_reset }, };

// -----------------------------------------------------------------------------
// Dispatch & line collection
// -----------------------------------------------------------------------------
static void str_upper(char *s) {
	while (*s) {
		*s = (char) toupper((unsigned char )*s);
		++s;
	}
}

static void dispatch_line(char *line) {
	char *argv[12];
	int argc = 0;

	for (char *tok = strtok(line, " \t\r\n");
			tok && argc < (int) (sizeof(argv) / sizeof(argv[0]));
			tok = strtok(NULL, " \t\r\n")) {
		argv[argc++] = tok;
	}
	if (argc == 0)
		return;

	str_upper(argv[0]); // Befehl selbst UPPERCASE
	for (size_t i = 0; i < sizeof(s_cmds) / sizeof(s_cmds[0]); ++i) {
		if (strcmp(argv[0], s_cmds[i].name) == 0) {
			(void) s_cmds[i].fn(argc, argv);
			return;
		}
	}
	replyf("ERR %s UNKNOWN%s", argv[0], EOL);
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------
void cmd_init(void) {
	serial_puts("CMD_READY\n");
}

void cmd_poll(void) {
	static char s_line[CMD_LINE_MAX];
	static size_t s_len = 0;

	int ch;
	while ((ch = serial_getchar_nonblock()) >= 0) {
		if (ch == '\n' || ch == '\r') {
			if (s_len > 0) {
				s_line[s_len] = '\0';
				dispatch_line(s_line);
				s_len = 0;
			}
		} else {
			if (s_len < (CMD_LINE_MAX - 1)) {
				s_line[s_len++] = (char) ch;
			} else {
				s_len = 0;
				replyf("ERR LINE OVERFLOW%s", EOL);
			}
		}
	}
}
