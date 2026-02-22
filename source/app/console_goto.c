#include "robot_config.h"

#if ENABLE_CONSOLE_GOTO

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "protocol.h"
#include "bot.h"
#include "job.h"

// Falls du es anders benannt hast: nur diese Zeile anpassen
#define POSE_CMD (g_status.pos_cmd)

static uint16_t s_req_id = 1;

static void print_help(void)
{
    printf("\r\n=== CONSOLE GOTO (semihost) ===\r\n");
    printf("Input:\r\n");
    printf("  x y [z] [phi]\r\n");
    printf("Examples:\r\n");
    printf("  120 50\r\n");
    printf("  120 50 10\r\n");
    printf("  120 50 10 90\r\n");
    printf("Commands: pos | home | ?\r\n\r\n");
    fflush(stdout);
}

static void print_pose(void)
{
    int32_t x = POSE_CMD.x_mm_scaled;
    int32_t y = POSE_CMD.y_mm_scaled;
    int32_t z = POSE_CMD.z_mm_scaled;
    int32_t p = POSE_CMD.phi_deg_scaled;

    long x_i = (long)(x / SCALE_MM);
    long y_i = (long)(y / SCALE_MM);
    long z_i = (long)(z / SCALE_MM);
    long p_i = (long)(p / SCALE_DEG);

    long x_f = (long)(x % SCALE_MM); if (x_f < 0) x_f = -x_f;
    long y_f = (long)(y % SCALE_MM); if (y_f < 0) y_f = -y_f;
    long z_f = (long)(z % SCALE_MM); if (z_f < 0) z_f = -z_f;
    long p_f = (long)(p % SCALE_DEG); if (p_f < 0) p_f = -p_f;

    printf("POS x=%ld.%03ld  y=%ld.%03ld  z=%ld.%03ld  phi=%ld.%02ld\r\n",
           x_i, x_f, y_i, y_f, z_i, z_f, p_i, p_f);
    fflush(stdout);
}

static void queue_home(void)
{
    bot_action_s a = {
        .type = ACT_HOME,
        .target_pos = {0},
        .magnet_on = false,
        .request_id = s_req_id++
    };

    if (!bot_enqueue(&a)) {
        printf("ERR queue_full\r\n");
    } else {
        printf("QUEUED HOME id=%u\r\n", (unsigned)a.request_id);
    }
    fflush(stdout);
}

static void queue_goto(int32_t x_s, int32_t y_s, int32_t z_s, int32_t ph_s)
{
    bot_action_s a = {
        .type = ACT_MOVE,
        .target_pos = {
            .x_mm_scaled = x_s,
            .y_mm_scaled = y_s,
            .z_mm_scaled = z_s,
            .phi_deg_scaled = ph_s
        },
        .magnet_on = false,
        .request_id = s_req_id++
    };

    if (!bot_enqueue(&a)) {
        printf("ERR queue_full\r\n");
    } else {
        printf("QUEUED GOTO id=%u\r\n", (unsigned)a.request_id);
    }
    fflush(stdout);
}

static void handle_line(char *line)
{
    // newline strip
    line[strcspn(line, "\r\n")] = 0;

    // trim leading spaces
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

    // defaults = aktuelle pose (damit "120 50" nur XY ändert)
    int32_t x_s  = POSE_CMD.x_mm_scaled;
    int32_t y_s  = POSE_CMD.y_mm_scaled;
    int32_t z_s  = POSE_CMD.z_mm_scaled;
    int32_t ph_s = POSE_CMD.phi_deg_scaled;

    // parse 2-4 ints (mm / deg)
    int x=0, y=0, z=0, phi=0;
    int n = sscanf(line, "%d %d %d %d", &x, &y, &z, &phi);

    if (n < 2) {
        printf("ERR: need at least x y\r\n");
        fflush(stdout);
        return;
    }

    x_s = x * SCALE_MM;
    y_s = y * SCALE_MM;
    if (n >= 3) z_s = z * SCALE_MM;
    if (n >= 4) ph_s = phi * SCALE_DEG;

    queue_goto(x_s, y_s, z_s, ph_s);
}

void console_goto_init(void)
{
    print_help();
    printf("> ");
    fflush(stdout);
}

void console_goto_poll(void)
{


    // Während Bewegung/Job läuft: NICHT blockierend auf input warten
    if (job_is_active()) return;

    // Blockierendes Lesen ist ok, weil wir hier "idle" sind.
    char line[96];
    if (!fgets(line, sizeof(line), stdin)) {
        return;
    }

    handle_line(line);

    printf("> ");
    fflush(stdout);
}

#else

void console_goto_init(void) {}
void console_goto_poll(void) {}

#endif
