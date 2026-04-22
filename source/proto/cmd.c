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
#include <stddef.h>
#include <stdlib.h>
#include "serial_port.h"
#include "robot_config.h"
#include "proto_io.h"
#include "protocol.h"   // g_status + state/err helper + Limits/Scales
#include "parse_kv.h"   // parse_pos_tokens_mask() + KV_* masks
#include "bot_engine.h"        // bot_enqueue() + bot_action_s
#include "motion_tuning.h"
#include "motion_config.h"
#include "home_config.h"
#include "pick_config.h"
#include "place_config.h"
#include "encoder_config.h"
#if POSITION_ENABLE
#include "position.h"
#endif

#ifndef CMD_LINE_MAX
#define CMD_LINE_MAX 128
#endif

// -----------------------------------------------------------------------------
// UART reply helpers
// -----------------------------------------------------------------------------
// Einheitliche Protokoll-Ausgabe nach oben.
// EOL ist überall gleich, replyf ist unser printf-Wrapper.
static const char* EOL = "\n";   // End-of-line for replies


// Standard-OK für rein synchrone Befehle (PING, RESET, ...).
static void send_ok(const char* cmd){
    proto_reply_printf("OK %s%s", cmd, EOL);
}
// Standard-ERR mit einfachem Error-String (SYNTAX, ESTOP, NO_HOME, ...).
static void send_err(const char* cmd, const char* e){
    proto_reply_printf("ERR %s %s%s", cmd, e, EOL);
}

// Request-IDs laufen monoton hoch und werden mit jeder Action gepusht.
// Der Pi verwendet id=.. später zum Zuordnen der finalen OK/ERR.
static uint16_t s_next_req_id = 1;
static void send_queued(const char* cmd, uint16_t id){
    proto_reply_printf("QUEUED %s id=%u%s", cmd, (unsigned)id, EOL);
}

// Kleiner Helper für STATUS-Ausgabe.
static const char* yesno(bool b){
    return b ? "YES" : "NO";
}

// Formatierung von Fixed-Point Positionen für UART, ohne float printf.
// um -> "mm.mmm" / cdeg -> "deg.dd".
static void print_mm3(const char* name, int32_t um){
    int32_t mm = um / 1000;
    int32_t frac = um % 1000;
    if(frac < 0) frac = -frac;

    proto_reply_printf("%s=%ld.%03ld ", name, (long)mm, (long)frac);
}

static void print_deg2(const char* name, int32_t cdeg){
    int32_t deg = cdeg / 100;
    int32_t frac = cdeg % 100;
    if(frac < 0) frac = -frac;

    proto_reply_printf("%s=%ld.%02ld ", name, (long)deg, (long)frac);
}

static bool parse_scale_pct(const char* s, uint16_t* out_scale_pct){
    char* end = NULL;
    unsigned long value;

    if(s == NULL || out_scale_pct == NULL || *s == '\0'){
        return false;
    }

    value = strtoul(s, &end, 10);
    if(*end != '\0' || value > 0xFFFFu){
        return false;
    }

    *out_scale_pct = (uint16_t)value;
    return true;
}

static bool tune_token_is_all(const char* token){
    return (strcmp(token, "ALL") == 0);
}

static bool tune_token_is_global(const char* token){
    return (strcmp(token, "GLOBAL") == 0) || (strcmp(token, "MASTER") == 0);
}

static bool parse_tune_kind(const char* token, motion_profile_kind_e* out_kind){
    if(strcmp(token, "MOVE") == 0){
        *out_kind = MOTION_PROFILE_KIND_MOVE;
        return true;
    }
    if(strcmp(token, "HOME") == 0){
        *out_kind = MOTION_PROFILE_KIND_HOME;
        return true;
    }
    if(strcmp(token, "PICK") == 0){
        *out_kind = MOTION_PROFILE_KIND_PICK;
        return true;
    }
    if(strcmp(token, "PLACE") == 0){
        *out_kind = MOTION_PROFILE_KIND_PLACE;
        return true;
    }
    if(strcmp(token, "CORR") == 0 || strcmp(token, "POSCORR") == 0){
        *out_kind = MOTION_PROFILE_KIND_CORR;
        return true;
    }

    return false;
}

static uint32_t tune_effective_scale_pct(motion_profile_kind_e kind){
    uint32_t global_pct = motion_tuning_get_global_scale_pct();
    uint32_t local_pct = motion_tuning_get_profile_scale_pct(kind);

    return (global_pct * local_pct + 50u) / 100u;
}

static void tune_print_scales(void){
    proto_reply_printf("TUNE SCALES global=%u move=%u home=%u pick=%u place=%u corr=%u%s",
        (unsigned)motion_tuning_get_global_scale_pct(),
        (unsigned)motion_tuning_get_profile_scale_pct(MOTION_PROFILE_KIND_MOVE),
        (unsigned)motion_tuning_get_profile_scale_pct(MOTION_PROFILE_KIND_HOME),
        (unsigned)motion_tuning_get_profile_scale_pct(MOTION_PROFILE_KIND_PICK),
        (unsigned)motion_tuning_get_profile_scale_pct(MOTION_PROFILE_KIND_PLACE),
        (unsigned)motion_tuning_get_profile_scale_pct(MOTION_PROFILE_KIND_CORR),
        EOL);
}

static void tune_print_group_scale(motion_profile_kind_e kind){
    proto_reply_printf("TUNE SCALE group=%s pct=%u effective=%lu%s",
        motion_tuning_kind_to_str(kind),
        (unsigned)motion_tuning_get_profile_scale_pct(kind),
        (unsigned long)tune_effective_scale_pct(kind),
        EOL);
}

static void tune_print_global_scale(void){
    proto_reply_printf("TUNE SCALE group=GLOBAL pct=%u%s",
        (unsigned)motion_tuning_get_global_scale_pct(),
        EOL);
}

static void tune_print_profile_summary(const char* name,
                                       motion_profile_kind_e kind,
                                       motion_profile_s base_profile){
    motion_profile_s scaled = motion_tuning_apply_profile(&base_profile, kind);

    proto_reply_printf("TUNE PROFILE group=%s name=%s start=%lu max=%lu accel=%lu%s",
        motion_tuning_kind_to_str(kind),
        name,
        (unsigned long)scaled.start_step_rate_sps,
        (unsigned long)scaled.max_step_rate_sps,
        (unsigned long)scaled.accel_sps2,
        EOL);
}

static void tune_print_define_u32(const char* name, uint32_t value){
    proto_reply_printf("#define %s %luu%s", name, (unsigned long)value, EOL);
}

static void tune_export_profile_macros(motion_profile_kind_e kind,
                                       const char* start_name,
                                       const char* max_name,
                                       const char* accel_name,
                                       motion_profile_s base_profile){
    motion_profile_s scaled = motion_tuning_apply_profile(&base_profile, kind);

    tune_print_define_u32(start_name, scaled.start_step_rate_sps);
    tune_print_define_u32(max_name, scaled.max_step_rate_sps);
    tune_print_define_u32(accel_name, scaled.accel_sps2);
}

static void tune_show_move_profiles(void){
    tune_print_profile_summary("X", MOTION_PROFILE_KIND_MOVE,
        (motion_profile_s){ X_START_STEP_RATE_SPS, X_MAX_STEP_RATE_SPS, X_ACCEL_SPS2 });
    tune_print_profile_summary("Y", MOTION_PROFILE_KIND_MOVE,
        (motion_profile_s){ Y_START_STEP_RATE_SPS, Y_MAX_STEP_RATE_SPS, Y_ACCEL_SPS2 });
    tune_print_profile_summary("Z", MOTION_PROFILE_KIND_MOVE,
        (motion_profile_s){ Z_START_STEP_RATE_SPS, Z_MAX_STEP_RATE_SPS, Z_ACCEL_SPS2 });
    tune_print_profile_summary("PHI", MOTION_PROFILE_KIND_MOVE,
        (motion_profile_s){ PHI_START_STEP_RATE_SPS, PHI_MAX_STEP_RATE_SPS, PHI_ACCEL_SPS2 });
}

static void tune_show_home_profiles(void){
    tune_print_profile_summary("X_RELEASE", MOTION_PROFILE_KIND_HOME,
        (motion_profile_s){ HOME_X_RELEASE_START_STEP_RATE_SPS, HOME_X_RELEASE_MAX_STEP_RATE_SPS, HOME_X_RELEASE_ACCEL_SPS2 });
    tune_print_profile_summary("X_FAST", MOTION_PROFILE_KIND_HOME,
        (motion_profile_s){ HOME_X_FAST_START_STEP_RATE_SPS, HOME_X_FAST_MAX_STEP_RATE_SPS, HOME_X_FAST_ACCEL_SPS2 });
    tune_print_profile_summary("X_BACKOFF", MOTION_PROFILE_KIND_HOME,
        (motion_profile_s){ HOME_X_BACKOFF_START_STEP_RATE_SPS, HOME_X_BACKOFF_MAX_STEP_RATE_SPS, HOME_X_BACKOFF_ACCEL_SPS2 });
    tune_print_profile_summary("X_SLOW", MOTION_PROFILE_KIND_HOME,
        (motion_profile_s){ HOME_X_SLOW_START_STEP_RATE_SPS, HOME_X_SLOW_MAX_STEP_RATE_SPS, HOME_X_SLOW_ACCEL_SPS2 });
    tune_print_profile_summary("Y_RELEASE", MOTION_PROFILE_KIND_HOME,
        (motion_profile_s){ HOME_Y_RELEASE_START_STEP_RATE_SPS, HOME_Y_RELEASE_MAX_STEP_RATE_SPS, HOME_Y_RELEASE_ACCEL_SPS2 });
    tune_print_profile_summary("Y_FAST", MOTION_PROFILE_KIND_HOME,
        (motion_profile_s){ HOME_Y_FAST_START_STEP_RATE_SPS, HOME_Y_FAST_MAX_STEP_RATE_SPS, HOME_Y_FAST_ACCEL_SPS2 });
    tune_print_profile_summary("Y_BACKOFF", MOTION_PROFILE_KIND_HOME,
        (motion_profile_s){ HOME_Y_BACKOFF_START_STEP_RATE_SPS, HOME_Y_BACKOFF_MAX_STEP_RATE_SPS, HOME_Y_BACKOFF_ACCEL_SPS2 });
    tune_print_profile_summary("Y_SLOW", MOTION_PROFILE_KIND_HOME,
        (motion_profile_s){ HOME_Y_SLOW_START_STEP_RATE_SPS, HOME_Y_SLOW_MAX_STEP_RATE_SPS, HOME_Y_SLOW_ACCEL_SPS2 });
    tune_print_profile_summary("Z_RELEASE", MOTION_PROFILE_KIND_HOME,
        (motion_profile_s){ HOME_Z_RELEASE_START_STEP_RATE_SPS, HOME_Z_RELEASE_MAX_STEP_RATE_SPS, HOME_Z_RELEASE_ACCEL_SPS2 });
    tune_print_profile_summary("Z_FAST", MOTION_PROFILE_KIND_HOME,
        (motion_profile_s){ HOME_Z_FAST_START_STEP_RATE_SPS, HOME_Z_FAST_MAX_STEP_RATE_SPS, HOME_Z_FAST_ACCEL_SPS2 });
    tune_print_profile_summary("Z_BACKOFF", MOTION_PROFILE_KIND_HOME,
        (motion_profile_s){ HOME_Z_BACKOFF_START_STEP_RATE_SPS, HOME_Z_BACKOFF_MAX_STEP_RATE_SPS, HOME_Z_BACKOFF_ACCEL_SPS2 });
    tune_print_profile_summary("Z_SLOW", MOTION_PROFILE_KIND_HOME,
        (motion_profile_s){ HOME_Z_SLOW_START_STEP_RATE_SPS, HOME_Z_SLOW_MAX_STEP_RATE_SPS, HOME_Z_SLOW_ACCEL_SPS2 });
}

static void tune_show_pick_profiles(void){
    tune_print_profile_summary("Z_DOWN", MOTION_PROFILE_KIND_PICK,
        (motion_profile_s){ PICK_Z_DOWN_START_STEP_RATE_SPS, PICK_Z_DOWN_MAX_STEP_RATE_SPS, PICK_Z_DOWN_ACCEL_SPS2 });
    tune_print_profile_summary("Z_UP", MOTION_PROFILE_KIND_PICK,
        (motion_profile_s){ PICK_Z_UP_START_STEP_RATE_SPS, PICK_Z_UP_MAX_STEP_RATE_SPS, PICK_Z_UP_ACCEL_SPS2 });
}

static void tune_show_place_profiles(void){
    tune_print_profile_summary("Z_DOWN", MOTION_PROFILE_KIND_PLACE,
        (motion_profile_s){ PLACE_Z_DOWN_START_STEP_RATE_SPS, PLACE_Z_DOWN_MAX_STEP_RATE_SPS, PLACE_Z_DOWN_ACCEL_SPS2 });
    tune_print_profile_summary("Z_UP", MOTION_PROFILE_KIND_PLACE,
        (motion_profile_s){ PLACE_Z_UP_START_STEP_RATE_SPS, PLACE_Z_UP_MAX_STEP_RATE_SPS, PLACE_Z_UP_ACCEL_SPS2 });
}

static void tune_show_corr_profiles(void){
    tune_print_profile_summary("XY_CORR", MOTION_PROFILE_KIND_CORR,
        (motion_profile_s){ POS_CORR_START_STEP_RATE_SPS, POS_CORR_MAX_STEP_RATE_SPS, POS_CORR_ACCEL_SPS2 });
}

static void tune_show_group_profiles(motion_profile_kind_e kind){
    switch(kind){
    case MOTION_PROFILE_KIND_MOVE:
        tune_show_move_profiles();
        break;
    case MOTION_PROFILE_KIND_HOME:
        tune_show_home_profiles();
        break;
    case MOTION_PROFILE_KIND_PICK:
        tune_show_pick_profiles();
        break;
    case MOTION_PROFILE_KIND_PLACE:
        tune_show_place_profiles();
        break;
    case MOTION_PROFILE_KIND_CORR:
        tune_show_corr_profiles();
        break;
    default:
        break;
    }
}

static void tune_show_all_profiles(void){
    tune_show_move_profiles();
    tune_show_home_profiles();
    tune_show_pick_profiles();
    tune_show_place_profiles();
    tune_show_corr_profiles();
}

static void tune_export_move_profiles(void){
    tune_export_profile_macros(MOTION_PROFILE_KIND_MOVE,
        "X_START_STEP_RATE_SPS", "X_MAX_STEP_RATE_SPS", "X_ACCEL_SPS2",
        (motion_profile_s){ X_START_STEP_RATE_SPS, X_MAX_STEP_RATE_SPS, X_ACCEL_SPS2 });
    tune_export_profile_macros(MOTION_PROFILE_KIND_MOVE,
        "Y_START_STEP_RATE_SPS", "Y_MAX_STEP_RATE_SPS", "Y_ACCEL_SPS2",
        (motion_profile_s){ Y_START_STEP_RATE_SPS, Y_MAX_STEP_RATE_SPS, Y_ACCEL_SPS2 });
    tune_export_profile_macros(MOTION_PROFILE_KIND_MOVE,
        "Z_START_STEP_RATE_SPS", "Z_MAX_STEP_RATE_SPS", "Z_ACCEL_SPS2",
        (motion_profile_s){ Z_START_STEP_RATE_SPS, Z_MAX_STEP_RATE_SPS, Z_ACCEL_SPS2 });
    tune_export_profile_macros(MOTION_PROFILE_KIND_MOVE,
        "PHI_START_STEP_RATE_SPS", "PHI_MAX_STEP_RATE_SPS", "PHI_ACCEL_SPS2",
        (motion_profile_s){ PHI_START_STEP_RATE_SPS, PHI_MAX_STEP_RATE_SPS, PHI_ACCEL_SPS2 });
}

static void tune_export_home_profiles(void){
    tune_export_profile_macros(MOTION_PROFILE_KIND_HOME, "HOME_X_RELEASE_START_STEP_RATE_SPS", "HOME_X_RELEASE_MAX_STEP_RATE_SPS", "HOME_X_RELEASE_ACCEL_SPS2", (motion_profile_s){ HOME_X_RELEASE_START_STEP_RATE_SPS, HOME_X_RELEASE_MAX_STEP_RATE_SPS, HOME_X_RELEASE_ACCEL_SPS2 });
    tune_export_profile_macros(MOTION_PROFILE_KIND_HOME, "HOME_X_FAST_START_STEP_RATE_SPS", "HOME_X_FAST_MAX_STEP_RATE_SPS", "HOME_X_FAST_ACCEL_SPS2", (motion_profile_s){ HOME_X_FAST_START_STEP_RATE_SPS, HOME_X_FAST_MAX_STEP_RATE_SPS, HOME_X_FAST_ACCEL_SPS2 });
    tune_export_profile_macros(MOTION_PROFILE_KIND_HOME, "HOME_X_BACKOFF_START_STEP_RATE_SPS", "HOME_X_BACKOFF_MAX_STEP_RATE_SPS", "HOME_X_BACKOFF_ACCEL_SPS2", (motion_profile_s){ HOME_X_BACKOFF_START_STEP_RATE_SPS, HOME_X_BACKOFF_MAX_STEP_RATE_SPS, HOME_X_BACKOFF_ACCEL_SPS2 });
    tune_export_profile_macros(MOTION_PROFILE_KIND_HOME, "HOME_X_SLOW_START_STEP_RATE_SPS", "HOME_X_SLOW_MAX_STEP_RATE_SPS", "HOME_X_SLOW_ACCEL_SPS2", (motion_profile_s){ HOME_X_SLOW_START_STEP_RATE_SPS, HOME_X_SLOW_MAX_STEP_RATE_SPS, HOME_X_SLOW_ACCEL_SPS2 });
    tune_export_profile_macros(MOTION_PROFILE_KIND_HOME, "HOME_Y_RELEASE_START_STEP_RATE_SPS", "HOME_Y_RELEASE_MAX_STEP_RATE_SPS", "HOME_Y_RELEASE_ACCEL_SPS2", (motion_profile_s){ HOME_Y_RELEASE_START_STEP_RATE_SPS, HOME_Y_RELEASE_MAX_STEP_RATE_SPS, HOME_Y_RELEASE_ACCEL_SPS2 });
    tune_export_profile_macros(MOTION_PROFILE_KIND_HOME, "HOME_Y_FAST_START_STEP_RATE_SPS", "HOME_Y_FAST_MAX_STEP_RATE_SPS", "HOME_Y_FAST_ACCEL_SPS2", (motion_profile_s){ HOME_Y_FAST_START_STEP_RATE_SPS, HOME_Y_FAST_MAX_STEP_RATE_SPS, HOME_Y_FAST_ACCEL_SPS2 });
    tune_export_profile_macros(MOTION_PROFILE_KIND_HOME, "HOME_Y_BACKOFF_START_STEP_RATE_SPS", "HOME_Y_BACKOFF_MAX_STEP_RATE_SPS", "HOME_Y_BACKOFF_ACCEL_SPS2", (motion_profile_s){ HOME_Y_BACKOFF_START_STEP_RATE_SPS, HOME_Y_BACKOFF_MAX_STEP_RATE_SPS, HOME_Y_BACKOFF_ACCEL_SPS2 });
    tune_export_profile_macros(MOTION_PROFILE_KIND_HOME, "HOME_Y_SLOW_START_STEP_RATE_SPS", "HOME_Y_SLOW_MAX_STEP_RATE_SPS", "HOME_Y_SLOW_ACCEL_SPS2", (motion_profile_s){ HOME_Y_SLOW_START_STEP_RATE_SPS, HOME_Y_SLOW_MAX_STEP_RATE_SPS, HOME_Y_SLOW_ACCEL_SPS2 });
    tune_export_profile_macros(MOTION_PROFILE_KIND_HOME, "HOME_Z_RELEASE_START_STEP_RATE_SPS", "HOME_Z_RELEASE_MAX_STEP_RATE_SPS", "HOME_Z_RELEASE_ACCEL_SPS2", (motion_profile_s){ HOME_Z_RELEASE_START_STEP_RATE_SPS, HOME_Z_RELEASE_MAX_STEP_RATE_SPS, HOME_Z_RELEASE_ACCEL_SPS2 });
    tune_export_profile_macros(MOTION_PROFILE_KIND_HOME, "HOME_Z_FAST_START_STEP_RATE_SPS", "HOME_Z_FAST_MAX_STEP_RATE_SPS", "HOME_Z_FAST_ACCEL_SPS2", (motion_profile_s){ HOME_Z_FAST_START_STEP_RATE_SPS, HOME_Z_FAST_MAX_STEP_RATE_SPS, HOME_Z_FAST_ACCEL_SPS2 });
    tune_export_profile_macros(MOTION_PROFILE_KIND_HOME, "HOME_Z_BACKOFF_START_STEP_RATE_SPS", "HOME_Z_BACKOFF_MAX_STEP_RATE_SPS", "HOME_Z_BACKOFF_ACCEL_SPS2", (motion_profile_s){ HOME_Z_BACKOFF_START_STEP_RATE_SPS, HOME_Z_BACKOFF_MAX_STEP_RATE_SPS, HOME_Z_BACKOFF_ACCEL_SPS2 });
    tune_export_profile_macros(MOTION_PROFILE_KIND_HOME, "HOME_Z_SLOW_START_STEP_RATE_SPS", "HOME_Z_SLOW_MAX_STEP_RATE_SPS", "HOME_Z_SLOW_ACCEL_SPS2", (motion_profile_s){ HOME_Z_SLOW_START_STEP_RATE_SPS, HOME_Z_SLOW_MAX_STEP_RATE_SPS, HOME_Z_SLOW_ACCEL_SPS2 });
}

static void tune_export_pick_profiles(void){
    tune_export_profile_macros(MOTION_PROFILE_KIND_PICK, "PICK_Z_DOWN_START_STEP_RATE_SPS", "PICK_Z_DOWN_MAX_STEP_RATE_SPS", "PICK_Z_DOWN_ACCEL_SPS2", (motion_profile_s){ PICK_Z_DOWN_START_STEP_RATE_SPS, PICK_Z_DOWN_MAX_STEP_RATE_SPS, PICK_Z_DOWN_ACCEL_SPS2 });
    tune_export_profile_macros(MOTION_PROFILE_KIND_PICK, "PICK_Z_UP_START_STEP_RATE_SPS", "PICK_Z_UP_MAX_STEP_RATE_SPS", "PICK_Z_UP_ACCEL_SPS2", (motion_profile_s){ PICK_Z_UP_START_STEP_RATE_SPS, PICK_Z_UP_MAX_STEP_RATE_SPS, PICK_Z_UP_ACCEL_SPS2 });
}

static void tune_export_place_profiles(void){
    tune_export_profile_macros(MOTION_PROFILE_KIND_PLACE, "PLACE_Z_DOWN_START_STEP_RATE_SPS", "PLACE_Z_DOWN_MAX_STEP_RATE_SPS", "PLACE_Z_DOWN_ACCEL_SPS2", (motion_profile_s){ PLACE_Z_DOWN_START_STEP_RATE_SPS, PLACE_Z_DOWN_MAX_STEP_RATE_SPS, PLACE_Z_DOWN_ACCEL_SPS2 });
    tune_export_profile_macros(MOTION_PROFILE_KIND_PLACE, "PLACE_Z_UP_START_STEP_RATE_SPS", "PLACE_Z_UP_MAX_STEP_RATE_SPS", "PLACE_Z_UP_ACCEL_SPS2", (motion_profile_s){ PLACE_Z_UP_START_STEP_RATE_SPS, PLACE_Z_UP_MAX_STEP_RATE_SPS, PLACE_Z_UP_ACCEL_SPS2 });
}

static void tune_export_corr_profiles(void){
    tune_export_profile_macros(MOTION_PROFILE_KIND_CORR, "POS_CORR_START_STEP_RATE_SPS", "POS_CORR_MAX_STEP_RATE_SPS", "POS_CORR_ACCEL_SPS2", (motion_profile_s){ POS_CORR_START_STEP_RATE_SPS, POS_CORR_MAX_STEP_RATE_SPS, POS_CORR_ACCEL_SPS2 });
}

static void tune_export_group_profiles(motion_profile_kind_e kind){
    switch(kind){
    case MOTION_PROFILE_KIND_MOVE:
        tune_export_move_profiles();
        break;
    case MOTION_PROFILE_KIND_HOME:
        tune_export_home_profiles();
        break;
    case MOTION_PROFILE_KIND_PICK:
        tune_export_pick_profiles();
        break;
    case MOTION_PROFILE_KIND_PLACE:
        tune_export_place_profiles();
        break;
    case MOTION_PROFILE_KIND_CORR:
        tune_export_corr_profiles();
        break;
    default:
        break;
    }
}

static void tune_export_all_profiles(void){
    tune_export_move_profiles();
    tune_export_home_profiles();
    tune_export_pick_profiles();
    tune_export_place_profiles();
    tune_export_corr_profiles();
}

// -----------------------------------------------------------------------------
// Command handlers
// -----------------------------------------------------------------------------
// Jeder Handler bekommt argc/argv (argv[0] ist der Command-Name).
typedef bool (*cmd_handler_fp_t)(int argc, char** argv);

typedef struct {
    const char* name;  // Befehl in UPPERCASE
    cmd_handler_fp_t fn;
} cmd_entry_s;

// --- PING ---
// Minimaler Sync-Test: antwortet sofort mit OK.
static bool cmd_ping(int argc, char** argv){
    (void)argc;
    (void)argv;
    send_ok("PING");
    return true;
}

// --- STATUS ---
// Gibt den aktuellen globalen Status inkl. interner Position aus.
// Position wird human-readable formatiert, intern bleibt alles Fixed-Point.
static bool cmd_status(int argc, char** argv){
    (void)argc; (void)argv;

    proto_reply_printf("STATUS state=%s (%d) homed=%s part=%s estop=%s err=%s POS ",
        state_to_str(g_status.state),
        (int)g_status.state,
        yesno(g_status.homed),
        yesno(g_status.has_part),
        yesno(g_status.estop),
        err_to_str(g_status.last_err));

    print_mm3("x", g_status.pos_internal.x_mm_scaled);
    print_mm3("y", g_status.pos_internal.y_mm_scaled);
    print_mm3("z", g_status.pos_internal.z_mm_scaled);
    print_deg2("phi", g_status.pos_internal.phi_deg_scaled);
#if POSITION_ENABLE
    proto_reply_printf(" POS_MEASURED ");
    print_mm3("x", g_status.pos_measured.x_mm_scaled);
    print_mm3("y", g_status.pos_measured.y_mm_scaled);
#endif
    proto_reply_printf(" TUNE ");
    proto_reply_printf("global=%u move=%u home=%u pick=%u place=%u corr=%u",
        (unsigned)motion_tuning_get_global_scale_pct(),
        (unsigned)motion_tuning_get_profile_scale_pct(MOTION_PROFILE_KIND_MOVE),
        (unsigned)motion_tuning_get_profile_scale_pct(MOTION_PROFILE_KIND_HOME),
        (unsigned)motion_tuning_get_profile_scale_pct(MOTION_PROFILE_KIND_PICK),
        (unsigned)motion_tuning_get_profile_scale_pct(MOTION_PROFILE_KIND_PLACE),
        (unsigned)motion_tuning_get_profile_scale_pct(MOTION_PROFILE_KIND_CORR));

    proto_reply_printf("%s", EOL);
    return true;
}

// --- POS ---
// Kurzform von STATUS: nur Position ausgeben.
static bool cmd_pos(int argc, char** argv){
    (void)argc; (void)argv;

    proto_reply_printf("POS ");
    print_mm3("x", g_status.pos_internal.x_mm_scaled);
    print_mm3("y", g_status.pos_internal.y_mm_scaled);
    print_mm3("z", g_status.pos_internal.z_mm_scaled);
    print_deg2("phi", g_status.pos_internal.phi_deg_scaled);
#if POSITION_ENABLE
    proto_reply_printf(" POS_MEASURED ");
    print_mm3("x", g_status.pos_measured.x_mm_scaled);
    print_mm3("y", g_status.pos_measured.y_mm_scaled);
#endif
    proto_reply_printf("%s", EOL);

    return true;
}

// --- HOME (asynchron) ---
// Startet Homing über Bot-Queue. Keine Parameter erlaubt.
// Finales OK/ERR kommt aus bot_step().
static bool cmd_home(int argc, char** argv){
    (void)argv;
    if(argc != 1){
        send_err("HOME", "SYNTAX");
        return false;
    }
    if(g_status.estop){
        send_err("HOME", "ESTOP");
        return false;
    }
    //prüfen ob pos 0 reicht oder obs negativ sein muss.
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


    if(!bot_enqueue(&a)){
        send_err("HOME", "INTERNAL");
        return false;
    }

    send_queued("HOME", a.request_id);
    return true;
}

// --- MAGNET ON|OFF (asynchron) ---
// Schaltet Magnet über Bot-Queue. Argument muss ON oder OFF sein.
static bool cmd_magnet(int argc, char** argv){
    if(argc != 2){
        send_err("MAGNET", "SYNTAX");
        return false;
    }
    bool on;
    if(strcmp(argv[1], "ON") == 0)
        on = true;
    else if(strcmp(argv[1], "OFF") == 0)
        on = false;
    else{
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


    if(!bot_enqueue(&a)){
        send_err("MAGNET", "INTERNAL");
        return false;
    }

    send_queued("MAGNET", a.request_id);
    return true;
}

// --- MOVE x= y= z= phi=  (asynchron) ---
// Erlaubt Float-Text als Input, intern wird direkt auf Fixed-Point geparst.
// Optional-Parameter: nicht angegebene Achsen bleiben auf aktueller Position.
static bool cmd_move(int argc, char** argv){


    if(argc < 2){ send_err("MOVE", "SYNTAX");  return false; }
    if(g_status.estop){ send_err("MOVE", "ESTOP");   return false; }

    if(REQUIRE_HOME_FOR_MOVE && !g_status.homed){ send_err("MOVE", "NO_HOME"); return false; }

    // Defaults = aktuelle Position (Fixed-Point), damit Teil-MOVEs präzise bleiben.
    int32_t x_s = g_status.pos_internal.x_mm_scaled;
    int32_t y_s = g_status.pos_internal.y_mm_scaled;
    int32_t z_s = g_status.pos_internal.z_mm_scaled;
    int32_t ph_s = g_status.pos_internal.phi_deg_scaled;

    // MOVE: mindestens 1 Key, erlaubt x/y/z/phi in beliebiger Kombination.
    err_e e = parse_pos_tokens_mask(
        argc, argv, 1, &x_s, &y_s, &z_s, &ph_s,
        /*require_mask=*/0,
        /*allowed_mask=*/KV_X | KV_Y | KV_Z | KV_PHI,
        /*seen_out=*/NULL
    );
    if(e != ERR_NONE){ send_err("MOVE", err_to_str(e)); return false; }

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



    if(!bot_enqueue(&a)){ send_err("MOVE", "QUEUE_FULL"); return false; }

    send_queued("MOVE", a.request_id);
    return true;
}


// --- PICK (asynchron) ---
// Nimmt ein Teil an Ziel XY auf. Protokoll erlaubt kein z= (Z läuft intern).
// Pflicht: x,y. Optional: phi.
static bool cmd_pick(int argc, char** argv){
    if(g_status.estop){ send_err("PICK", "ESTOP");   return false; }
    if(REQUIRE_HOME_FOR_MOVE && !g_status.homed){ send_err("PICK", "NO_HOME"); return false; }
    if(g_status.has_part){ send_err("PICK", "HAS_PART");   return false; }

    int32_t x_s = 0, y_s = 0, z_s = 0, ph_s = 0;

    // PICK: x,y Pflicht / z,phi verboten (Whitelist!)
    err_e e = parse_pos_tokens_mask(
        argc, argv, 1,
        &x_s, &y_s, &z_s, &ph_s,
        /*require_mask=*/KV_X | KV_Y,
        /*allowed_mask=*/KV_X | KV_Y,
        /*seen_out=*/NULL
    );
    if(e != ERR_NONE){ send_err("PICK", err_to_str(e)); return false; }

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



    if(!bot_enqueue(&a)){ send_err("PICK", "QUEUE_FULL"); return false; }
    send_queued("PICK", a.request_id);
    return true;
}


// --- PLACE (asynchron) ---
// Legt ein Teil an Ziel XY/PHI ab. Protokoll erlaubt kein z=.
// Pflicht: x,y,phi.
static bool cmd_place(int argc, char** argv){
    if(g_status.estop){ send_err("PLACE", "ESTOP");   return false; }
    if(REQUIRE_HOME_FOR_MOVE && !g_status.homed){ send_err("PLACE", "NO_HOME"); return false; }
    if(!g_status.has_part){ send_err("PLACE", "NO_PART"); return false; }

    int32_t x_s = 0, y_s = 0, z_s = 0, ph_s = 0;

    // PLACE: x,y,phi Pflicht / z verboten (Whitelist!)
    err_e e = parse_pos_tokens_mask(
        argc, argv, 1,
        &x_s, &y_s, &z_s, &ph_s,
        /*require_mask=*/KV_X | KV_Y | KV_PHI,
        /*allowed_mask=*/KV_X | KV_Y | KV_PHI,
        /*seen_out=*/NULL
    );
    if(e != ERR_NONE){ send_err("PLACE", err_to_str(e)); return false; }

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



    if(!bot_enqueue(&a)){ send_err("PLACE", "QUEUE_FULL"); return false; }
    send_queued("PLACE", a.request_id);
    return true;
}



// --- RESET (synchron) ---
static bool cmd_reset(int argc, char** argv){
    (void)argc;
    (void)argv;

    bot_clear_queue();

    g_status.estop = false;
    g_status.state = STATE_IDLE;
    g_status.last_err = ERR_NONE;

    g_status.has_part = false;
    g_status.homed = false;

    g_status.pos_internal.x_mm_scaled = 0;
    g_status.pos_internal.y_mm_scaled = 0;
    g_status.pos_internal.z_mm_scaled = 0;
    g_status.pos_internal.phi_deg_scaled = 0;

    // pos_measured bewusst NICHT künstlich nullen.
    // Falls Encoder aktiv sind, bleibt dort der aktuell gemessene Wert stehen.
    // Falls keine Encoder aktiv sind, wird position_poll() measured wieder aus internal ableiten.

    send_ok("RESET");
    return true;
}

static bool cmd_clear_estop(int argc, char** argv){
    (void)argc;
    (void)argv;

    if(!g_status.estop){
        send_err("CLEAR_ESTOP", "NO_ESTOP");
        return false;
    }

    bot_clear_queue();

    g_status.estop = false;
    g_status.state = STATE_IDLE;
    g_status.last_err = ERR_NONE;

#if POSITION_ENABLE
    g_status.pos_internal = g_status.pos_measured;
#endif

    send_ok("CLEAR_ESTOP");
    return true;
}

static bool cmd_set_pos(int argc, char** argv){
    if(g_status.estop){ send_err("SET_POS", "ESTOP");   return false; }
    if(REQUIRE_HOME_FOR_MOVE && g_status.homed){ send_err("SET_POS", "POS_ALREADY_KNOWN"); return false; }

    int32_t x_s = 0, y_s = 0, z_s = 0, ph_s = 0;

    // PICK: x,y,z Pflicht / phi verboten (Whitelist!)
    err_e e = parse_pos_tokens_mask(
        argc, argv, 1,
        &x_s, &y_s, &z_s, &ph_s,
        /*require_mask=*/KV_X | KV_Y | KV_Z,
        /*allowed_mask=*/KV_X | KV_Y | KV_Z | KV_PHI,
        /*seen_out=*/NULL
    );
    if(e != ERR_NONE){ send_err("SET_POS", err_to_str(e)); return false; }

    g_status.pos_internal.x_mm_scaled = x_s;
    g_status.pos_internal.y_mm_scaled = y_s;
    g_status.pos_internal.z_mm_scaled = z_s;
    g_status.pos_internal.phi_deg_scaled = ph_s;
#if POSITION_ENABLE
    position_set_xy_mm_scaled(x_s, y_s);
    g_status.pos_internal.z_mm_scaled = z_s;
    g_status.pos_internal.phi_deg_scaled = ph_s;
#else
    g_status.pos_measured = g_status.pos_internal;
#endif
    g_status.homed = true;
    g_status.state = STATE_IDLE;
    g_status.last_err = ERR_NONE;

    send_ok("SET_POS");
    return true;
}

static bool cmd_tune(int argc, char** argv){
    motion_profile_kind_e kind;
    uint16_t scale_pct;

    if(argc == 1){
        tune_print_scales();
        return true;
    }

    if(strcmp(argv[1], "GET") == 0){
        if(argc == 2 || tune_token_is_all(argv[2])){
            tune_print_scales();
            return true;
        }
        if(tune_token_is_global(argv[2])){
            tune_print_global_scale();
            return true;
        }
        if(parse_tune_kind(argv[2], &kind)){
            tune_print_group_scale(kind);
            return true;
        }
        send_err("TUNE", "SYNTAX");
        return false;
    }

    if(strcmp(argv[1], "SET") == 0){
        if(argc != 4){
            send_err("TUNE", "SYNTAX");
            return false;
        }
        if(!parse_scale_pct(argv[3], &scale_pct)){
            send_err("TUNE", "SYNTAX");
            return false;
        }
        if(tune_token_is_global(argv[2])){
            if(!motion_tuning_set_global_scale_pct(scale_pct)){
                send_err("TUNE", "RANGE");
                return false;
            }
            proto_reply_printf("OK TUNE SET group=GLOBAL pct=%u%s",
                (unsigned)motion_tuning_get_global_scale_pct(),
                EOL);
            return true;
        }
        if(parse_tune_kind(argv[2], &kind)){
            if(!motion_tuning_set_profile_scale_pct(kind, scale_pct)){
                send_err("TUNE", "RANGE");
                return false;
            }
            proto_reply_printf("OK TUNE SET group=%s pct=%u effective=%lu%s",
                motion_tuning_kind_to_str(kind),
                (unsigned)motion_tuning_get_profile_scale_pct(kind),
                (unsigned long)tune_effective_scale_pct(kind),
                EOL);
            return true;
        }
        send_err("TUNE", "SYNTAX");
        return false;
    }

    if(strcmp(argv[1], "SHOW") == 0){
        if(argc == 2 || tune_token_is_all(argv[2])){
            tune_show_all_profiles();
            return true;
        }
        if(tune_token_is_global(argv[2])){
            tune_print_global_scale();
            tune_print_scales();
            return true;
        }
        if(parse_tune_kind(argv[2], &kind)){
            tune_show_group_profiles(kind);
            return true;
        }
        send_err("TUNE", "SYNTAX");
        return false;
    }

    if(strcmp(argv[1], "EXPORT") == 0){
        if(argc == 2 || tune_token_is_all(argv[2])){
            tune_export_all_profiles();
            return true;
        }
        if(parse_tune_kind(argv[2], &kind)){
            tune_export_group_profiles(kind);
            return true;
        }
        send_err("TUNE", "SYNTAX");
        return false;
    }

    send_err("TUNE", "SYNTAX");
    return false;
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
        { "TUNE", cmd_tune },
        { "RESET", cmd_reset },
        { "CLEAR_ESTOP", cmd_clear_estop },
        { "CESTOP", cmd_clear_estop },
        { "UNLATCH", cmd_clear_estop },
        { "SET_POS", cmd_set_pos}
};

// -----------------------------------------------------------------------------
// Dispatch & line collection
// -----------------------------------------------------------------------------
// Macht ein String in-place uppercase (für case-insensitive Parsing).
static void str_upper(char* s){
    while(*s){
        *s = (char)toupper((unsigned char)*s);
        ++s;
    }
}

// Zerlegt eine komplette Zeile in Tokens und ruft den passenden Handler.
// Unbekannte Befehle liefern "ERR <cmd> UNKNOWN".
void cmd_dispatch_line(char* line){
    char* argv[12];
    int argc = 0;

    for(char* tok = strtok(line, " \t\r\n");
        tok && argc < (int)(sizeof(argv) / sizeof(argv[0]));
        tok = strtok(NULL, " \t\r\n")){
        argv[argc++] = tok;
    }
    if(argc == 0)
        return;

    for(int i = 0; i < argc; i++){
        str_upper(argv[i]);	//case insensitive
    }
    for(size_t i = 0; i < sizeof(s_cmds) / sizeof(s_cmds[0]); ++i){
        if(strcmp(argv[0], s_cmds[i].name) == 0){
            (void)s_cmds[i].fn(argc, argv);
            return;
        }
    }
    proto_reply_printf("ERR %s UNKNOWN%s", argv[0], EOL);
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------
// Init meldet dem Pi, dass der Parser bereit ist.
void cmd_init(void){
    proto_reply_raw("CMD_READY\n");
}

// Non-blocking Zeilensammler.
// - Liest Bytes von UART bis \n oder \r.
// - Bei kompletter Zeile -> dispatch_line().
// - Bei Overflow -> Zeile verwerfen + ERR LINE OVERFLOW.
void cmd_poll(void){
    static char s_line[CMD_LINE_MAX];
    static size_t s_len = 0;

    int ch;
    while((ch = serial_getchar_nonblock()) >= 0){
        if(ch == '\n' || ch == '\r'){
            if(s_len > 0){
                s_line[s_len] = '\0';
                cmd_dispatch_line(s_line);
                s_len = 0;
            }
        }
        else{
            if(s_len < (CMD_LINE_MAX - 1)){
                s_line[s_len++] = (char)ch;
            }
            else{
                s_len = 0;
                proto_reply_printf("ERR LINE OVERFLOW%s", EOL);
            }
        }
    }
}
