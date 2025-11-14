/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )
 (()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_))
 (_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
 | _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|
 |  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |
 |_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|
 bot.c  Created on: 14.11.2025   Author: Fige23  Team 3
*/

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

#include "bot.h"
#include "protocol.h"     // g_status, SCALE_MM, SCALE_DEG, states
#include "serial_port.h"  // serial_puts

// -----------------------------------------------------------------------------
// Reply helper
// -----------------------------------------------------------------------------
static const char *EOL = "\n";
static void replyf(const char *fmt, ...)
{
    char buf[192];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    serial_puts(buf);
}

static const char* act_name(bot_action_e t)
{
    switch (t) {
        case ACT_MOVE:   return "MOVE";
        case ACT_HOME:   return "HOME";
        case ACT_PICK:   return "PICK";
        case ACT_PLACE:  return "PLACE";
        case ACT_MAGNET: return "MAGNET";
        default:         return "CMD";
    }
}

// -----------------------------------------------------------------------------
// Helpers: Positionsübernahme (Fixed->mm/deg) mit Rundung
// -----------------------------------------------------------------------------
static inline int32_t round_div_s(int32_t s, int32_t scale)
{
    if (s >= 0) return (s + scale/2) / scale;
    return -(( -s + scale/2) / scale);
}

static inline void apply_target_to_status(const bot_action_s *a)
{
    g_status.pos.x_mm    = round_div_s(a->x_001mm,    SCALE_MM);
    g_status.pos.y_mm    = round_div_s(a->y_001mm,    SCALE_MM);
    g_status.pos.z_mm    = round_div_s(a->z_001mm,    SCALE_MM);   // bei PICK/PLACE typ. 0 (Z-Profil intern)
    g_status.pos.phi_deg = round_div_s(a->phi_001deg, SCALE_DEG);
}

// -----------------------------------------------------------------------------
// Einfache Ring-Queue für Actions
// -----------------------------------------------------------------------------
#define BOT_Q_LEN 8

static bot_action_s q[BOT_Q_LEN];
static uint8_t q_head = 0, q_tail = 0, q_count = 0;

static bool bot_dequeue(bot_action_s *out)
{
    if (q_count == 0) return false;
    *out = q[q_tail];
    q_tail = (uint8_t)((q_tail + 1) % BOT_Q_LEN);
    q_count--;
    return true;
}

bool bot_enqueue(const bot_action_s *a)
{
    if (q_count >= BOT_Q_LEN) return false;
    q[q_head] = *a;
    q_head = (uint8_t)((q_head + 1) % BOT_Q_LEN);
    q_count++;
    return true;
}

// Optional: alles verwerfen (z.B. bei ESTOP)
// void bot_queue_clear(void) { q_head=q_tail=q_count=0; }

// -----------------------------------------------------------------------------
// Bot-Engine Stub: arbeitet jeweils 1 Action; Updates + OK id=… senden
// -----------------------------------------------------------------------------
void bot_step(void)
{
    static bool busy = false;
    static bot_action_s cur;

    if (!busy) {
        if (!bot_dequeue(&cur)) return;  // nichts zu tun
        busy = true;

        // State während der Ausführung (nur Anzeige)
        switch (cur.type) {
            case ACT_HOME:   g_status.state = STATE_HOMING;  break;
            case ACT_PICK:   g_status.state = STATE_PICKING; break;
            case ACT_PLACE:  g_status.state = STATE_PLACING; break;
            case ACT_MOVE:   g_status.state = STATE_MOVING;  break;
            case ACT_MAGNET: g_status.state = STATE_MOVING;  break;
            default:         g_status.state = STATE_MOVING;  break;
        }

        // → Hier später reale Motor-Sequenz starten
    }

    // Stub: sofort fertig (später: Fortschritt/Ende prüfen)
    bool done = true;

    if (done) {
        switch (cur.type) {
            case ACT_HOME:
                g_status.homed = true;
                g_status.pos.x_mm = 0;
                g_status.pos.y_mm = 0;
                g_status.pos.z_mm = 0;
                g_status.pos.phi_deg = 0;
                break;

            case ACT_MOVE:
                apply_target_to_status(&cur);
                break;

            case ACT_PICK:
                // XY (+phi) ins Ziel; Z fährt intern. Danach Teil vorhanden.
                apply_target_to_status(&cur);
                g_status.has_part = true;
                break;

            case ACT_PLACE:
                // XY+phi ins Ziel; Z intern. Danach kein Teil mehr.
                apply_target_to_status(&cur);
                g_status.has_part = false;
                break;

            case ACT_MAGNET:
                // nur Magnet schalten – Position bleibt
                break;
        }

        g_status.state = STATE_IDLE;
        replyf("OK %s id=%u%s", act_name(cur.type), (unsigned)cur.req_id, EOL);

        busy = false;
    }
}
