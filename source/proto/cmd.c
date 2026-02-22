/*Project: PREN_Puzzleroboter
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

===============================================================================
cmd.c  (UART Command Parser / Dispatcher)

Aufgabe:
- Liest Zeilen non-blocking von UART.
- Zerlegt Tokens, validiert Syntax und aktuellen Systemzustand (homed/estop/part).
- Konvertiert Positionswerte direkt in Fixed-Point (via parse_kv.c).
- Erstellt bot_action_s und queued sie (bot_enqueue()).

Wichtig:
- cmd.c führt keine Bewegung direkt aus.
- Sofortige Antwort ist "QUEUED ... id=...".
- Finales OK/ERR kommt später aus bot_step().
===============================================================================
*/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stddef.h>

#include "robot_config.h"
#include "serial_port.h"
#include "protocol.h"   // g_status + state/err helper + Limits/Scales
#include "parse_kv.h"   // parse_pos_tokens_mask() + KV_* masks
#include "bot.h"        // bot_enqueue() + bot_action_s

#ifndef CMD_LINE_MAX
#define CMD_LINE_MAX 128
#endif

// -----------------------------------------------------------------------------
// UART reply helpers
// -----------------------------------------------------------------------------
// Einheitliche Protokoll-Ausgabe nach oben.
// EOL ist überall gleich, replyf ist unser printf-Wrapper.
static const char *EOL = "\n";   // End-of-line for replies



static void replyf(const char *fmt, ...) {
	char buf[192];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
#if USE_SEMIHOST_CONSOLE
    // Semihost: Ausgabe in MCUXpresso Debug Console
    printf("%s", buf);
    fflush(stdout);
#else
    // Dein bisheriger Weg (UART/serial_puts)
    serial_puts(buf);
#endif
}

// Standard-OK für rein synchrone Befehle (PING, RESET, ...).
static void send_ok(const char *cmd) {
	replyf("OK %s%s", cmd, EOL);
}
// Standard-ERR mit einfachem Error-String (SYNTAX, ESTOP, NO_HOME, ...).
static void send_err(const char *cmd, const char *e) {
	replyf("ERR %s %s%s", cmd, e, EOL);
}

// Request-IDs laufen monoton hoch und werden mit jeder Action gepusht.
// Der Pi verwendet id=.. später zum Zuordnen der finalen OK/ERR.
static uint16_t s_next_req_id = 1;
static void send_queued(const char *cmd, uint16_t id) {
	replyf("QUEUED %s id=%u%s", cmd, (unsigned) id, EOL);
}

// Kleiner Helper für STATUS-Ausgabe.
static const char* yesno(bool b) {
	return b ? "YES" : "NO";
}

// Formatierung von Fixed-Point Positionen für UART, ohne float printf.
// um -> "mm.mmm" / cdeg -> "deg.dd".
static void print_mm3(const char *name, int32_t um)
{
    int32_t mm   = um / 1000;
    int32_t frac = um % 1000;
    if (frac < 0) frac = -frac;

    replyf("%s=%ld.%03ld ", name, (long)mm, (long)frac);
}

static void print_deg2(const char *name, int32_t cdeg)
{
    int32_t deg  = cdeg / 100;
    int32_t frac = cdeg % 100;
    if (frac < 0) frac = -frac;

    replyf("%s=%ld.%02ld ", name, (long)deg, (long)frac);
}

// -----------------------------------------------------------------------------
// Command handlers
// -----------------------------------------------------------------------------
// Jeder Handler bekommt argc/argv (argv[0] ist der Command-Name).
typedef bool (*cmd_handler_fp_t)(int argc, char **argv);

typedef struct {
	const char *name;  // Befehl in UPPERCASE
	cmd_handler_fp_t fn;
} cmd_entry_s;

// --- PING ---
// Minimaler Sync-Test: antwortet sofort mit OK.
static bool cmd_ping(int argc, char **argv) {
	(void) argc;
	(void) argv;
	send_ok("PING");
	return true;
}

// --- STATUS ---
// Gibt den aktuellen globalen Status inkl. interner Position aus.
// Position wird human-readable formatiert, intern bleibt alles Fixed-Point.
static bool cmd_status(int argc, char **argv)
{
    (void)argc; (void)argv;

    replyf("STATUS state=%s (%d) homed=%s part=%s estop=%s err=%s POS ",
           state_to_str(g_status.state),
           (int)g_status.state,
           yesno(g_status.homed),
           yesno(g_status.has_part),
           yesno(g_status.estop),
           err_to_str(g_status.last_err));

    print_mm3("x", g_status.pos_cmd.x_mm_scaled);
    print_mm3("y", g_status.pos_cmd.y_mm_scaled);
    print_mm3("z", g_status.pos_cmd.z_mm_scaled);
    print_deg2("phi", g_status.pos_cmd.phi_deg_scaled);

    replyf(" POS_MEASURED");
    print_mm3("x", g_status.pos_measured.x_mm_scaled);
    print_mm3("y", g_status.pos_measured.y_mm_scaled);


    replyf("%s", EOL);
    return true;
}

// --- POS ---
// Kurzform von STATUS: nur Position ausgeben.
static bool cmd_pos(int argc, char **argv)
{
    (void)argc; (void)argv;

    replyf("POS ");
    print_mm3("x", g_status.pos_cmd.x_mm_scaled);
    print_mm3("y", g_status.pos_cmd.y_mm_scaled);
    print_mm3("z", g_status.pos_cmd.z_mm_scaled);
    print_deg2("phi", g_status.pos_cmd.phi_deg_scaled);
    replyf("%s", EOL);

    return true;
}

// --- HOME (asynchron) ---
// Startet Homing über Bot-Queue. Keine Parameter erlaubt.
// Finales OK/ERR kommt aus bot_step().
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
	    .target_pos = {
	        .x_mm_scaled = 0,
	        .y_mm_scaled = 0,
	        .z_mm_scaled = 0,
	        .phi_deg_scaled = 0
	    },
	    .magnet_on = false,
	    .request_id = s_next_req_id++
	};


	if (!bot_enqueue(&a)) {
		send_err("HOME", "INTERNAL");
		return false;
	}

	send_queued("HOME", a.request_id);
	return true;
}

// --- MAGNET ON|OFF (asynchron) ---
// Schaltet Magnet über Bot-Queue. Argument muss ON oder OFF sein.
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
	    .target_pos = {   // unbenutzt, aber explizit
	        .x_mm_scaled = 0,
	        .y_mm_scaled = 0,
	        .z_mm_scaled = 0,
	        .phi_deg_scaled = 0
	    },
	    .magnet_on = on,
	    .request_id = s_next_req_id++
	};


	if (!bot_enqueue(&a)) {
		send_err("MAGNET", "INTERNAL");
		return false;
	}

	send_queued("MAGNET", a.request_id);
	return true;
}

// --- MOVE x= y= z= phi=  (asynchron) ---
// Erlaubt Float-Text als Input, intern wird direkt auf Fixed-Point geparst.
// Optional-Parameter: nicht angegebene Achsen bleiben auf aktueller Position.
static bool cmd_move(int argc, char **argv)
{
    if (argc < 2)        { send_err("MOVE","SYNTAX");  return false; }
    if (g_status.estop)  { send_err("MOVE","ESTOP");   return false; }

    if (REQUIRE_HOME_FOR_MOVE && !g_status.homed) { send_err("MOVE","NO_HOME"); return false; }

    // Defaults = aktuelle Position (Fixed-Point), damit Teil-MOVEs präzise bleiben.
    int32_t x_s  = g_status.pos_cmd.x_mm_scaled;
    int32_t y_s  = g_status.pos_cmd.y_mm_scaled;
    int32_t z_s  = g_status.pos_cmd.z_mm_scaled;
    int32_t ph_s = g_status.pos_cmd.phi_deg_scaled;

    // MOVE: mindestens 1 Key, erlaubt x/y/z/phi in beliebiger Kombination.
    err_e e = parse_pos_tokens_mask(
        argc, argv, 1, &x_s, &y_s, &z_s, &ph_s,
        /*require_mask=*/0,
        /*allowed_mask=*/KV_X | KV_Y | KV_Z | KV_PHI,
        /*seen_out=*/NULL
    );
    if (e != ERR_NONE) { send_err("MOVE", err_to_str(e)); return false; }

    bot_action_s a = {
        .type = ACT_MOVE,
        .target_pos = {
            .x_mm_scaled = x_s,
            .y_mm_scaled = y_s,
            .z_mm_scaled = z_s,
            .phi_deg_scaled = ph_s
        },
        .magnet_on = false,
        .request_id = s_next_req_id++
    };



    if (!bot_enqueue(&a)) { send_err("MOVE","QUEUE_FULL"); return false; }

    send_queued("MOVE", a.request_id);
    return true;
}


// --- PICK (asynchron) ---
// Nimmt ein Teil an Ziel XY auf. Protokoll erlaubt kein z= (Z läuft intern).
// Pflicht: x,y. Optional: phi.
static bool cmd_pick(int argc, char **argv)
{
    if (g_status.estop)     { send_err("PICK","ESTOP");   return false; }
    if (REQUIRE_HOME_FOR_MOVE && !g_status.homed)    { send_err("PICK","NO_HOME"); return false; }
    if (g_status.has_part)  { send_err("PICK","HAS_PART");   return false; }

    int32_t x_s=0, y_s=0, z_s=0, ph_s=0;

    // PICK: x,y Pflicht / phi optional / z verboten (Whitelist!)
    err_e e = parse_pos_tokens_mask(
        argc, argv, 1,
        &x_s, &y_s, &z_s, &ph_s,
        /*require_mask=*/KV_X | KV_Y,
        /*allowed_mask=*/KV_X | KV_Y | KV_PHI,
        /*seen_out=*/NULL
    );
    if (e != ERR_NONE) { send_err("PICK", err_to_str(e)); return false; }

    bot_action_s a = {
        .type = ACT_PICK,
        .target_pos = {
            .x_mm_scaled = x_s,
            .y_mm_scaled = y_s,
            .z_mm_scaled = z_s,        // bleibt 0 (Z wird intern im Job gemacht)
            .phi_deg_scaled = ph_s     // optional -> entweder geparst oder default
        },
        .magnet_on = false,
        .request_id = s_next_req_id++
    };



    if (!bot_enqueue(&a)) { send_err("PICK","QUEUE_FULL"); return false; }
    send_queued("PICK", a.request_id);
    return true;
}


// --- PLACE (asynchron) ---
// Legt ein Teil an Ziel XY/PHI ab. Protokoll erlaubt kein z=.
// Pflicht: x,y,phi.
static bool cmd_place(int argc, char **argv)
{
    if (g_status.estop)      { send_err("PLACE","ESTOP");   return false; }
    if (REQUIRE_HOME_FOR_MOVE && !g_status.homed)     { send_err("PLACE","NO_HOME"); return false; }
    if (!g_status.has_part)  { send_err("PLACE","NO_PART"); return false; }

    int32_t x_s=0, y_s=0, z_s=0, ph_s=0;

    // PLACE: x,y,phi Pflicht / z verboten (Whitelist!)
    err_e e = parse_pos_tokens_mask(
        argc, argv, 1,
        &x_s, &y_s, &z_s, &ph_s,
        /*require_mask=*/KV_X | KV_Y | KV_PHI,
        /*allowed_mask=*/KV_X | KV_Y | KV_PHI,
        /*seen_out=*/NULL
    );
    if (e != ERR_NONE) { send_err("PLACE", err_to_str(e)); return false; }

    // (nach parse_pos_tokens_mask)
    bot_action_s a = {
        .type = ACT_PLACE,
        .target_pos = {
            .x_mm_scaled = x_s,
            .y_mm_scaled = y_s,
            .z_mm_scaled = 0,       // bleibt 0 (Z-Profil intern)
            .phi_deg_scaled = ph_s  // Pflicht
        },
        .magnet_on = false,
        .request_id = s_next_req_id++
    };



    if (!bot_enqueue(&a)) { send_err("PLACE","QUEUE_FULL"); return false; }
    send_queued("PLACE", a.request_id);
    return true;
}



// --- RESET (synchron) ---
// Setzt nur den lokalen Status zurück. Keine Queue, keine Bewegung.
static bool cmd_reset(int argc, char **argv)
{
    (void)argc; (void)argv;

    g_status.state = STATE_IDLE;
    g_status.has_part = false;
    g_status.homed = false;
    g_status.estop = false;
    g_status.last_err = ERR_NONE;

    g_status.pos_cmd.x_mm_scaled = 0;
    g_status.pos_cmd.y_mm_scaled = 0;
    g_status.pos_cmd.z_mm_scaled = 0;
    g_status.pos_cmd.phi_deg_scaled = 0;

    send_ok("RESET");
    return true;
}


// -----------------------------------------------------------------------------
// Command table
// -----------------------------------------------------------------------------
// Alle unterstützten Befehle. argv[0] wird gegen name gematched.
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
// Macht ein String in-place uppercase (für case-insensitive Parsing).
static void str_upper(char *s) {
	while (*s) {
		*s = (char) toupper((unsigned char )*s);
		++s;
	}
}

// Zerlegt eine komplette Zeile in Tokens und ruft den passenden Handler.
// Unbekannte Befehle liefern "ERR <cmd> UNKNOWN".
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

	for(int i=0 ; i < argc; i++){
		str_upper(argv[i]);	//case insensitive
	}
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
// Init meldet dem Pi, dass der Parser bereit ist.
void cmd_init(void) {
	serial_puts("CMD_READY\n");
}

// Non-blocking Zeilensammler.
// - Liest Bytes von UART bis \n oder \r.
// - Bei kompletter Zeile -> dispatch_line().
// - Bei Overflow -> Zeile verwerfen + ERR LINE OVERFLOW.
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
