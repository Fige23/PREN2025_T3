/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )
 (()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_))
 (_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
 | _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|
 |  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |
 |_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|
 bot_engine.c  Created on: 14.11.2025   Author: Fige23  Team 3
*/


#include <stdint.h>
#include <stdbool.h>

#include "job.h"
#include "io.h"
#include "bot_engine.h"
#include "protocol.h"     // g_status, err_to_str(), state/error enums
#include "proto_io.h"
#include "robot_config.h"
// -----------------------------------------------------------------------------
// Reply helper
// -----------------------------------------------------------------------------
static const char *EOL = "\n";

static const char* act_name(bot_action_e t)
{
    switch (t) {
    case ACT_MOVE:
        return "MOVE";
    case ACT_HOME:
        return "HOME";
    case ACT_PICK:
        return "PICK";
    case ACT_PLACE:
        return "PLACE";
    case ACT_MAGNET:
        return "MAGNET";
    default:
        return "CMD";
    }
}

static void reply_ok_action(const bot_action_s *a)
{
    proto_reply_printf("OK %s id=%u%s", act_name(a->type),
                       (unsigned)a->request_id, EOL);
}

static void reply_err_action(const bot_action_s *a, err_e e)
{
    proto_reply_printf("ERR %s id=%u%s", err_to_str(e),
                       (unsigned)a->request_id, EOL);
}

// -----------------------------------------------------------------------------
// Action Queue (Ringbuffer)
// -----------------------------------------------------------------------------
#ifndef BOT_Q_LEN
#define BOT_Q_LEN 16
#endif

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
    q_head = 0;
    q_tail = 0;
    q_count = 0;
}

void bot_clear_queue(void)
{
    bot_queue_clear();
}

// -----------------------------------------------------------------------------
// Small helper
// -----------------------------------------------------------------------------
static bot_state_e busy_state_from_action(bot_action_e t)
{
    switch (t) {
    case ACT_MOVE:
        return STATE_MOVING;
    case ACT_HOME:
        return STATE_HOMING;
    case ACT_PICK:
        return STATE_PICKING;
    case ACT_PLACE:
        return STATE_PLACING;
    default:
        return STATE_ERROR;
    }
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// Bot Engine
// -----------------------------------------------------------------------------
void bot_step(void)
{
    static bool busy = false;
    static bot_action_s cur;

    // Wenn ESTOP aktiv: keine neuen Actions starten, Queue verwerfen
    if (g_status.estop) {
        bot_queue_clear();
        g_status.state = STATE_EMERGENCY_STOP;
        g_status.last_err = ERR_ESTOP;

        if (!busy) {
            return;
        }
    }

    // -------------------------------------------------------------------------
    // 1) IDLE: nächste Action holen + starten
    // -------------------------------------------------------------------------
    if (!busy) {
        if (!bot_dequeue(&cur)) {
            return;
        }

        // Instant Action: MAGNET
        if (cur.type == ACT_MAGNET) {
            magnet_on_off(cur.magnet_on);
            g_status.last_err = ERR_NONE;
            reply_ok_action(&cur);
            return;
        }

        // Busy-State für Anzeige setzen
        g_status.state = busy_state_from_action(cur.type);
        
        err_e e = job_start(&cur);
        if (e != ERR_NONE) {
            if (e == ERR_ESTOP) {
                g_status.state = STATE_EMERGENCY_STOP;
                g_status.estop = true;
                bot_queue_clear();
            } else {
                g_status.state = STATE_ERROR;
            }

            g_status.last_err = e;
            reply_err_action(&cur, e);
            return;
        }

        busy = true;
        return;
    }

    // -------------------------------------------------------------------------
    // 2) BUSY: aktiven Job weiterführen und ggf. final antworten
    // -------------------------------------------------------------------------
    err_e je = ERR_NONE;

    // job_step() -> false: läuft noch
    if (!job_step(&je)) {
        return;
    }

    // Job ist fertig (OK oder ERR)
    if (je != ERR_NONE) {
        if (je == ERR_ESTOP) {
            g_status.state = STATE_EMERGENCY_STOP;
            g_status.estop = true;
            bot_queue_clear();
        } else {
            g_status.state = STATE_ERROR;
        }

        g_status.last_err = je;
        reply_err_action(&cur, je);
        busy = false;
        return;
    }

    // Erfolg: Status finalisieren je nach Action
    if (cur.type == ACT_PICK) {
        g_status.has_part = true;
    } else if (cur.type == ACT_PLACE) {
        g_status.has_part = false;
    }

    g_status.state = STATE_IDLE;
    g_status.last_err = ERR_NONE;
    reply_ok_action(&cur);

    busy = false;
}
