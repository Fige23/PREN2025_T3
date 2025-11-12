/*
 * cmd.c
 *	Project: PREN_Puzzleroboter
 *  Created on: 12.11.2025
 *  Author: Fige23
 */
//aaa

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stddef.h>

#include "serial_port.h"
#include "protocol.h"

#ifndef CMD_LINE_MAX
#define CMD_LINE_MAX 128
#endif

//Globaler Status (startwerte)
volatile bot_status_s g_status = {
		.state = STATE_IDLE,
		.homed = false,
		.has_part = false,
		.estop = false,
		.last_err = ERR_NONE,
		.pos = {0,0,0,0}
};


static const char *EOL = "\n";		//end of line: \n, alternativ \r\n

//Utils:
static void replyf(const char *fmt, ...){	// Für antworten:
	char buf[192];							// Lokaler Buffer
	va_list ap; 							// va_list: zeiger auf variable Argumente der Funktion
	va_start(ap, fmt);			 			// initialisiert ap direkt nach fmt
	vsnprintf(buf, sizeof(buf), fmt, ap);	// schreibt argumente in buf
	va_end(ap);								// schliesst die varargs nutzung ab
	serial_puts(buf);						// sendet was in buf steht per UART
}
/* Aufrufbeispiele:
 *
 * replyf("OK %s%s, "PING", EOL);
 * replyf("POS X=%ld Y=%ld Z=%ld PHI=%ld%s, (long)x, (long)y, (long)z, (long)phi, EOL);
 *
 */

//sendet ok auf entspr. befehl retour
static void send_ok(const char *cmd){
	replyf("OK %s%s", cmd, EOL);
}
//sendet error auf entsp befehl retour
static void send_err(const char *cmd, const char *e){
	replyf("ERR %s %s%s", cmd, e, EOL);
}

static const char* err_to_str(err_e e) {

	switch (e) {
	case ERR_NONE:
		return "NONE";
	case ERR_SYNTAX:
		return "SYNTAX";
	case ERR_RANGE:
		return "RANGE";
	case ERR_NO_HOME:
		return "NO_HOME";
	case ERR_NO_PART:
		return "NO_PART";
	case ERR_PLACE_FAIL:
		return "PLACE_FAIL";
	case ERR_MOTOR:
		return "MOTOR";
	case ERR_ESTOP:
		return "ESTOP";
	default:
		return "INTERNAL";
	}
}

static const char* state_to_str(bot_state_e s) {
	switch (s) {
	case STATE_INIT:
		return "INIT";
	case STATE_IDLE:
		return "IDLE";
	case STATE_MOVING:
		return "MOVING";
	case STATE_HOMING:
		return "HOMING";
	case STATE_PICKING:
		return "PICKING";
	case STATE_PLACING:
		return "PLACING";
	case STATE_ERROR:
		return "ERROR";
	case STATE_EMERGENCY_STOP:
		return "EMERGENCY_STOP";
	default:
		return "UNKNOWN";
	}
}

static const char* yesno(bool b){
	return b? "YES":"NO";
}


//Command handlers:


typedef bool (*cmd_handler_fp_t)(int argc, char **argv);

typedef struct {
	const char *name;
	cmd_handler_fp_t fn;
} cmd_entry_s;


//ping
static bool cmd_ping(int argc, char **argv){
	(void)argc; (void)argv;
	send_ok("PING");
	return true;
}

//status
static bool cmd_status(int argc, char **argv) {
	(void) argc;
	(void) argv; //ungenutzte Parameter unterdrücken
	replyf("STATUS state=%s (%d) homed=%s part=%s estop=%s err=%s " "POS x=%ld y=%ld z=%ld phi=%ld%s",
			state_to_str(g_status.state), (int) g_status.state,
			yesno(g_status.homed), yesno(g_status.has_part),
			yesno(g_status.estop), err_to_str(g_status.last_err),
			(long) g_status.pos.x_mm, (long) g_status.pos.y_mm,
			(long) g_status.pos.z_mm, (long) g_status.pos.phi_deg, EOL);
	return true;
}

//position
static bool cmd_pos(int argc, char **argv){
	(void)argc; (void)argv;
	replyf("POS x=%ld y=%ld z=%ld phi=%ld%s", (long)g_status.pos.x_mm, (long)g_status.pos.y_mm, (long)g_status.pos.z_mm, (long)g_status.pos.phi_deg, EOL);
	return true;
}
//Hier später weitere befehle einfügen....





static const cmd_entry_s s_cmds[] = {
		{"PING", cmd_ping},
		{"STATUS", cmd_status},
		{"POS", cmd_pos}
		//Hier später weitere einfügen....
};



//dispatch:

static void str_upper(char *s) {
	while (*s) {
		*s = (char) toupper((unsigned char )*s);
		++s;
	}
}

static void dispatch_line(char *line){
	char *argv[12]; int argc = 0;

	for( char *tok = strtok(line, " \t\r\n"); tok && argc < (int)(sizeof(argv)/sizeof(argv[0])); tok = strtok(NULL, " \t\r\n")){
		argv[argc++] = tok;
	}
	if(argc == 0)return;

	str_upper(argv[0]);
	for(size_t i = 0; i < sizeof(s_cmds)/sizeof(s_cmds[0]); i++){
		if(strcmp(argv[0], s_cmds[i].name) == 0){
			(void)s_cmds[i].fn(argc, argv);
			return;
		}
	}
	replyf("ERR %s UNKNOWN%s", argv[0], EOL);
}

//"Öffentliche" funktionen:

void cmd_init(void){
	serial_puts("CMD_READY\n");
}

void cmd_poll(void){
    static char   s_line[CMD_LINE_MAX];
    static size_t s_len = 0;

    int ch;
    while ((ch = serial_getchar_nonblock()) >= 0) {
        if (ch == '\n' || ch == '\r') {         // LF oder CR beendet die Zeile
            if (s_len > 0) {
                s_line[s_len] = '\0';
                dispatch_line(s_line);
                s_len = 0;
            }
        } else {
            if (s_len < (CMD_LINE_MAX - 1)) {   // Platz für '\0' lassen
                s_line[s_len++] = (char)ch;
            } else {
                s_len = 0;                      // overflow -> verwerfen
                replyf("ERR LINE OVERFLOW%s", "\n");
            }
        }
    }
}


