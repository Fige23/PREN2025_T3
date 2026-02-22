/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
console_goto.c	Created on: 22.02.2026	   Author: Fige23	Team 3                                                                
*/


#include "robot_config.h"

#if ENABLE_CONSOLE_GOTO

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "serial_port.h"
#include "protocol.h"
#include "bot.h"
#include "parse_kv.h"

// Falls du in deinem Status-Struct andere Namen hast, ändere NUR diese Macro-Zeile:
#define POSE_CMD   (g_status.pos_cmd)   // alternativ: (g_status.pos)

static inline int32_t iabs32(int32_t v) { return (v < 0) ? -v : v; }

static void sp(const char *fmt, ...)
{
    char buf[160];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    serial_puts(buf);
}

static void print_pose(void)
{
    int32_t x = POSE_CMD.x_mm_scaled;
    int32_t y = POSE_CMD.y_mm_scaled;
    int32_t z = POSE_CMD.z_mm_scaled;
    int32_t p = POSE_CMD.phi_deg_scaled;

    sp("POS x=%ld.%03ld y=%ld.%03ld z=%ld.%03ld phi=%ld.%02ld\r\n",
       (long)(x / SCALE_MM), (long)iabs32(x % SCALE_MM),
       (long)(y / SCALE_MM), (long)iabs32(y % SCALE_MM),
       (long)(z / SCALE_MM), (long)iabs32(z % SCALE_MM),
       (long)(p / SCALE_DEG), (long)iabs32(p % SCALE_DEG));
}

static void print_help(void)
{
    sp("\r\n=== CONSOLE GOTO MODE ===\r\n");
    sp("Eingabe:\r\n");
    sp("  x=120 y=50 z=0 phi=0\r\n");
    sp("  oder kurz: 120 50 [z] [phi]\r\n");
    sp("Commands:  pos | home | ?\r\n\r\n");
}

static void queue_home(void)
{
    static uint16_t req = 30000;

    bot_action_s a = {
        .type = ACT_HOME,
        .target_pos = { .x_mm_scaled=0, .y_mm_scaled=0, .z_mm_scaled=0, .phi_deg_scaled=0 },
        .magnet_on = false,
        .request_id = req++
    };

    if (!bot_enqueue(&a)) {
        sp("ERR QUEUE_FULL\r\n");
        return;
    }
    sp("QUEUED HOME id=%u\r\n", (unsigned)a.request_id);
}

static void queue_goto(int32_t x_s, int32_t y_s, int32_t z_s, int32_t ph_s)
{
    static uint16_t req = 1;

    bot_action_s a = {
        .type = ACT_MOVE,
        .target_pos = {
            .x_mm_scaled = x_s,
            .y_mm_scaled = y_s,
            .z_mm_scaled = z_s,
            .phi_deg_scaled = ph_s
        },
        .magnet_on = false,
        .request_id = req++
    };

    if (!bot_enqueue(&a)) {
        sp("ERR QUEUE_FULL\r\n");
        return;
    }
    sp("QUEUED GOTO id=%u\r\n", (unsigned)a.request_id);
}

static void handle_line(char *line)
{
    // trim leading
    while (*line == ' ' || *line == '\t') line++;
    if (*line == '\0') return;

    if (!strcmp(line, "?") || !strcmp(line, "help") || !strcmp(line, "HELP")) {
        print_help();
        return;
    }
    if (!strcmp(line, "pos") || !strcmp(line, "POS")) {
        print_pose();
        return;
    }
    if (!strcmp(line, "home") || !strcmp(line, "HOME")) {
        queue_home();
        return;
    }

    // Defaults = aktuelle Pose (damit "120 50" nur XY ändert)
    int32_t x_s  = POSE_CMD.x_mm_scaled;
    int32_t y_s  = POSE_CMD.y_mm_scaled;
    int32_t z_s  = POSE_CMD.z_mm_scaled;
    int32_t ph_s = POSE_CMD.phi_deg_scaled;

    // tokenize whitespace
    char *argv[8];
    int argc = 0;
    for (char *tok = strtok(line, " \t"); tok && argc < 8; tok = strtok(NULL, " \t")) {
        argv[argc++] = tok;
    }
    if (argc < 2) {
        sp("ERR need at least x y\r\n");
        return;
    }

    // key=value erkannt?
    bool has_eq = false;
    for (int i=0;i<argc;i++) {
        if (strchr(argv[i], '=')) { has_eq = true; break; }
    }

    err_e e = ERR_NONE;

    if (has_eq) {
        // parse: x=.. y=.. z=.. phi=..
        char *argv2[9];
        argv2[0] = "GOTO";
        for (int i=0;i<argc;i++) argv2[i+1] = argv[i];

        e = parse_pos_tokens_mask(
            argc+1, argv2, 1,
            &x_s, &y_s, &z_s, &ph_s,
            KV_X | KV_Y,
            KV_X | KV_Y | KV_Z | KV_PHI,
            NULL
        );
    } else {
        // parse: x y [z] [phi] -> in key=value umformen
        char tx[24], ty[24], tz[24], tp[24];
        snprintf(tx, sizeof(tx), "x=%s", argv[0]);
        snprintf(ty, sizeof(ty), "y=%s", argv[1]);

        char *argv2[6];
        int n = 0;
        argv2[n++] = "GOTO";
        argv2[n++] = tx;
        argv2[n++] = ty;

        if (argc >= 3) { snprintf(tz, sizeof(tz), "z=%s", argv[2]); argv2[n++] = tz; }
        if (argc >= 4) { snprintf(tp, sizeof(tp), "phi=%s", argv[3]); argv2[n++] = tp; }

        e = parse_pos_tokens_mask(
            n, argv2, 1,
            &x_s, &y_s, &z_s, &ph_s,
            KV_X | KV_Y,
            KV_X | KV_Y | KV_Z | KV_PHI,
            NULL
        );
    }

    if (e != ERR_NONE) {
        sp("ERR %s\r\n", err_to_str(e));
        return;
    }

    queue_goto(x_s, y_s, z_s, ph_s);
}

void console_goto_init(void)
{
    print_help();
    serial_puts("> ");
}

void console_goto_poll(void)
{
    static char line[96];
    static uint32_t len = 0;

    int ch;
    while ((ch = serial_getchar_nonblock()) >= 0) {

        if (ch == '\r' || ch == '\n') {
            serial_puts("\r\n");
            line[len] = '\0';
            handle_line(line);
            len = 0;
            serial_puts("> ");
            continue;
        }

        // Backspace
        if (ch == 0x08 || ch == 0x7F) {
            if (len > 0) {
                len--;
#if CONSOLE_GOTO_ECHO
                serial_puts("\b \b");
#endif
            }
            continue;
        }

        if (len < sizeof(line) - 1) {
            line[len++] = (char)ch;
#if CONSOLE_GOTO_ECHO
            char c[2] = { (char)ch, 0 };
            serial_puts(c);
#endif
        } else {
            // overflow -> reset
            len = 0;
            serial_puts("\r\nERR LINE OVERFLOW\r\n> ");
        }
    }
}

#else
// Wenn ENABLE_CONSOLE_GOTO=0, liefern wir leere Stubs, damit main.c clean bleibt
void console_goto_init(void) {}
void console_goto_poll(void) {}
#endif
