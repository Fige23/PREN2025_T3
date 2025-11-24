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

 Bot-Engine / Action-Scheduler.
 - cmd.c/parsing erzeugt bot_action_s und ruft bot_enqueue().
 - bot.c puffert die Actions in einer FIFO-Queue und arbeitet sie nacheinander ab.
 - bot_step() ist der einzige Ort, der finale OK/ERR Antworten ans Pi schickt.
 - Ab hier ist alles Fixed-Point:
     x/y/z: 0.001 mm (= 1 µm)  -> int32 x_001mm
     phi : 0.01°              -> int32 phi_001deg
 - Abarbeitung ist bereits asynchron aufgebaut:
     * wir starten eine Action
     * warten später auf done (Motion/Job)
     * erst dann wird OK gesendet
   Aktuell ist done noch ein Stub, damit der Rest der Architektur schon passt.
*/

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

#include "bot.h"
#include "protocol.h"     // g_status (globaler Status), state/error enums
#include "serial_port.h"  // serial_puts() für UART-Ausgabe

// -----------------------------------------------------------------------------
// Reply helper
// -----------------------------------------------------------------------------
// Kleine printf-Hilfe für UART-Ausgaben.
// Alle Rückmeldungen (OK/ERR/EVT) laufen über replyf() für einheitliches Format.
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

// Wandelt bot_action_e in ein lesbares Protokoll-Token.
// Wird im finalen OK benutzt, z.B. "OK MOVE id=3".
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
// Helpers: Positionsübernahme (Fixed-Point)
// -----------------------------------------------------------------------------
// Signed Division mit Rundung auf nächstes Integer.
// (Aktuell nicht zwingend gebraucht, bleibt als Utility.)
static inline int32_t round_div_s(int32_t s, int32_t scale)
{
    if (s >= 0) return (s + scale/2) / scale;
    return -(( -s + scale/2) / scale);
}

// Übernimmt die Zielkoordinaten einer Action in den globalen Status.
// Damit ist g_status.pos nach einer fertig gemeldeten Action konsistent.
// Hinweis zu Z: PICK/PLACE erlauben kein z= im Protokoll -> z bleibt dort typ. 0,
// weil die echte Z-Sequenz intern in der Pick/Place-Routine passiert.
static inline void apply_target_to_status(const bot_action_s *a)
{
    g_status.pos.x_001mm   = a->x_001mm;
    g_status.pos.y_001mm   = a->y_001mm;
    g_status.pos.z_001mm   = a->z_001mm;     // bei PICK/PLACE typ. 0 (Z-Profil intern)
    g_status.pos.phi_001deg= a->phi_001deg;
}

// -----------------------------------------------------------------------------
// Einfache Ring-Queue für Actions
// -----------------------------------------------------------------------------
// FIFO-Ringbuffer für Actions.
// - bot_enqueue() pusht hinten rein (q_head).
// - bot_dequeue() poppt vorne raus (q_tail).
// - q_count schützt vor Overflow/Underflow.
#define BOT_Q_LEN 8

static bot_action_s q[BOT_Q_LEN];
static uint8_t q_head = 0, q_tail = 0, q_count = 0;

// Holt die älteste Action aus der Queue.
// return false -> Queue leer.
static bool bot_dequeue(bot_action_s *out)
{
    if (q_count == 0) return false;
    *out = q[q_tail];
    q_tail = (uint8_t)((q_tail + 1) % BOT_Q_LEN);
    q_count--;
    return true;
}

// Legt eine neue Action in die Queue.
// return false -> Queue voll (cmd.c soll dann QUEUE_FULL melden).
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
// Ablauf:
// 1) Idle-Phase: nächste Action holen.
//    - ACT_MAGNET ist instant -> sofort schalten und OK senden.
//    - Alle anderen Actions sind "zeitkritisch" -> busy=true setzen.
// 2) Busy-Phase: später auf done warten (Motion/Job).
// 3) Done-Phase: Status finalisieren (pos/homed/has_part) und OK senden.
//
// Wichtige Designregel:
// - OK/ERR wird erst geschickt, wenn die Action wirklich fertig ist.
// - Während busy wird g_status.state auf den passenden Busy-State gesetzt,
//   damit STATUS-Abfragen am Pi Sinn machen.
void bot_step(void)
{
    static bool busy = false;
    static bot_action_s cur;

    // 1) Wenn idle: nächste Action holen
    if (!busy) {
        if (!bot_dequeue(&cur)) return;  // nichts zu tun

        // ---- Instant Action: MAGNET ----
        // Magnet schaltet nur den Greifer. Keine Achsenbewegung -> kein busy/statewechsel.
        if (cur.type == ACT_MAGNET) {
            //magnet_set(cur.on);
            replyf("OK MAGNET id=%u%s", (unsigned)cur.req_id, EOL);
            return;
        }

        // ---- Bewegungs-/Job-Action startet async ----
        busy = true;

        // Busy-State setzen (nur Anzeige/Debug).
        switch (cur.type) {
            case ACT_HOME:  g_status.state = STATE_HOMING;  break;
            case ACT_MOVE:  g_status.state = STATE_MOVING;  break;
            case ACT_PICK:  g_status.state = STATE_PICKING; break;
            case ACT_PLACE: g_status.state = STATE_PLACING; break;

            default:
                // Unbekannter Typ -> sofort Fehler melden.
                g_status.state = STATE_ERROR;
                g_status.last_err = ERR_SYNTAX;
                replyf("ERR UNKNOWN_ACTION id=%u%s",
                       (unsigned)cur.req_id, EOL);
                busy = false;
                return;
        }

        // >>> HIER STARTET SPÄTER DEINE ECHTE ASYNC-ENGINE <<<
        // MOVE/HOME: motion_start_move/home(...)
        // PICK/PLACE: job_start_pick/place(...) (interne Sequenzen)
    }

    // 2) Wenn busy: auf Ende warten (stub)
    // Später ersetzt durch:
    //   done = motion_is_done() oder job_done() inkl. error handling.
    bool done = true;  // STUB: im Moment sofort fertig

    if (!done) return;

    // 3) Abschluss: Status/Position updaten je nach Action
    switch (cur.type) {
        case ACT_HOME:
            g_status.homed = true;
            g_status.pos.x_001mm    = 0;
            g_status.pos.y_001mm    = 0;
            g_status.pos.z_001mm    = 0;
            g_status.pos.phi_001deg = 0;
            break;

        case ACT_MOVE:
            apply_target_to_status(&cur);
            break;

        case ACT_PICK:
            apply_target_to_status(&cur);
            g_status.has_part = true;
            break;

        case ACT_PLACE:
            apply_target_to_status(&cur);
            g_status.has_part = false;
            break;

        default:
            // Sollte nicht passieren, weil oben abgefangen.
            g_status.state = STATE_ERROR;
            g_status.last_err = ERR_INTERNAL;
            replyf("ERR INTERNAL id=%u%s",
                   (unsigned)cur.req_id, EOL);
            busy = false;
            return;
    }

    // 4) Final OK
    g_status.state = STATE_IDLE;
    replyf("OK %s id=%u%s", act_name(cur.type),
           (unsigned)cur.req_id, EOL);

    busy = false;
}
