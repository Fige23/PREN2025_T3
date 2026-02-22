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



===============================================================================
bot.c  (Bot Engine / Action Scheduler)

Aufgabe:
- Ringbuffer-Queue von bot_action_s (FIFO).
- Abarbeitung sequenziell:
    - holt nächste Action
    - startet Job/Motion (oder schaltet Magnet)
    - wartet auf completion (job_step/motion_is_done)
    - setzt g_status.state/last_err
    - sendet final OK/ERR mit request_id

Design-Regeln:
- Reihenfolge ist garantiert (Queue).
- Keine direkte Bewegung aus cmd.c heraus.
- ESTOP: Queue wird verworfen, Status in EMERGENCY_STOP.
===============================================================================
*/


#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

#include "io.h"
#include "bot.h"
#include "protocol.h"     // g_status, err_to_str(), state/error enums
#include "serial_port.h"  // serial_puts()
#include "job.h"          // job_start_move(), job_step(), job_init()
#include "robot_config.h"
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

static void reply_ok_action(const bot_action_s *a)
{
    replyf("OK %s id=%u%s", act_name(a->type), (unsigned)a->request_id, EOL);
}

static void reply_err_action(const bot_action_s *a, err_e e)
{
    replyf("ERR %s id=%u%s", err_to_str(e), (unsigned)a->request_id, EOL);
}

// -----------------------------------------------------------------------------
// Helpers: Positionsübernahme (Fixed-Point)
// -----------------------------------------------------------------------------
static inline void apply_target_to_status(const bot_action_s *a)
{
    g_status.pos_cmd.x_mm_scaled    = a->target_pos.x_mm_scaled;
    g_status.pos_cmd.y_mm_scaled    = a->target_pos.y_mm_scaled;
    g_status.pos_cmd.z_mm_scaled    = a->target_pos.z_mm_scaled;
    g_status.pos_cmd.phi_deg_scaled = a->target_pos.phi_deg_scaled;
}

// -----------------------------------------------------------------------------
// Action Queue (Ringbuffer)
// -----------------------------------------------------------------------------
#define BOT_Q_LEN 8

static bot_action_s q[BOT_Q_LEN];
static uint8_t q_head = 0, q_tail = 0, q_count = 0;

static bool bot_dequeue(bot_action_s *out)
{
    if (q_count == 0) return false;
    *out = q[q_tail];
    q_tail = (uint8_t)((q_tail + 1u) % BOT_Q_LEN);
    q_count--;
    return true;
}

bool bot_enqueue(const bot_action_s *a)
{
    if (q_count >= BOT_Q_LEN) return false;
    q[q_head] = *a;
    q_head = (uint8_t)((q_head + 1u) % BOT_Q_LEN);
    q_count++;
    return true;
}

static void bot_queue_clear(void)
{
    q_head = q_tail = q_count = 0;
}

// -----------------------------------------------------------------------------
// Bot Engine
// -----------------------------------------------------------------------------
#if IMPLEMENTATION_STEPPER
void bot_step(void)
{
    static bool busy = false;
    static bot_action_s cur;

    // Wenn ESTOP aktiv: keine neuen Actions starten, Queue verwerfen.
    if (g_status.estop || g_status.state == STATE_EMERGENCY_STOP) {
        bot_queue_clear();

        // Falls gerade ein MOVE-Job läuft, lassen wir ihn auslaufen:
        // motion/job wird per ISR auf ERR_ESTOP finishen, job_step wird dann fertig.
        if (!busy) return;
    }

    // -------------------------------------------------------------------------
    // 1) IDLE: nächste Action holen + starten
    // -------------------------------------------------------------------------
    if (!busy) {
        if (!bot_dequeue(&cur)) return;

        // Instant Action: MAGNET
        if (cur.type == ACT_MAGNET) {
            magnet_on_off(cur.magnet_on);
            reply_ok_action(&cur);
            return;
        }

        switch (cur.type) {

            case ACT_MOVE: {
                g_status.state = STATE_MOVING;

                err_e e = job_start_move(&cur);
                if (e != ERR_NONE) {
                    // E-Stop Spezialfall: sauber in ESTOP-State bleiben
                    if (e == ERR_ESTOP) {
                        g_status.state = STATE_EMERGENCY_STOP;
                        g_status.estop = true;
                        g_status.homed = false;
                        bot_queue_clear();
                    } else {
                        g_status.state = STATE_ERROR;
                    }

                    g_status.last_err = e;
                    reply_err_action(&cur, e);
                    return; // busy bleibt false
                }

                busy = true;
                return;
            }

            // Später implementieren
            case ACT_HOME:
            case ACT_PICK:
            case ACT_PLACE:
            default: {
                g_status.state = STATE_ERROR;
                g_status.last_err = ERR_INTERNAL;
                reply_err_action(&cur, ERR_INTERNAL);
                return;
            }
        }
    }

    // -------------------------------------------------------------------------
    // 2) BUSY: Job/Motion weiterführen und ggf. final antworten
    // -------------------------------------------------------------------------
    if (cur.type == ACT_MOVE) {
        err_e je;

        // job_step() -> false: läuft noch
        if (!job_step(&je)) {
            return;
        }

        // Job ist fertig (OK oder ERR)
        if (je != ERR_NONE) {
            if (je == ERR_ESTOP) {
                g_status.state = STATE_EMERGENCY_STOP;
                g_status.estop = true;
                g_status.homed = false;
                bot_queue_clear();
            } else {
                g_status.state = STATE_ERROR;
            }

            g_status.last_err = je;
            reply_err_action(&cur, je);
            busy = false;
            return;
        }

        // Erfolg
        g_status.state = STATE_IDLE;
        g_status.last_err = ERR_NONE;
        reply_ok_action(&cur);

        busy = false;
        return;
    }

    // Safety fallback: busy=true, aber kein unterstützter Typ
    g_status.state = STATE_ERROR;
    g_status.last_err = ERR_INTERNAL;
    reply_err_action(&cur, ERR_INTERNAL);
    busy = false;
}



#endif

#if UART_DEMO
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
            replyf("OK MAGNET id=%u%s", (unsigned)cur.request_id, EOL);
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
                       (unsigned)cur.request_id, EOL);
                busy = false;
                return;
        }

        // >>> HIER STARTET SPÄTER ECHTE ASYNC-ENGINE <<<
        // MOVE/HOME: motion_start_move/home(...)
        // PICK/PLACE: job_start_pick/place(...) (interne Sequenzen)
    }

    // 2) Wenn busy: auf Ende warten (stub)
    // Später ersetzt durch:
    //   done = motion_is_done() oder job_done() inkl. error handling.
    bool done = true;  // STUB: im Moment sofort fertig

    if (!done) return;


    //das hier brauchts nur solange keine richtige bewegung implementiert:
    //die Bewegungsfunktionen aktualisieren status/pos selber.
    // 3) Abschluss: Status/Position updaten je nach Action
    switch (cur.type) {
        case ACT_HOME:
            g_status.homed = true;
            g_status.pos_cmd.x_mm_scaled    = 0;
            g_status.pos_cmd.y_mm_scaled    = 0;
            g_status.pos_cmd.z_mm_scaled    = 0;
            g_status.pos_cmd.phi_deg_scaled = 0;
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
                   (unsigned)cur.request_id, EOL);
            busy = false;
            return;
    }

    // 4) Final OK
    g_status.state = STATE_IDLE;
    replyf("OK %s id=%u%s", act_name(cur.type),
           (unsigned)cur.request_id, EOL);

    busy = false;
}
#endif
