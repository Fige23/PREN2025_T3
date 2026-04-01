# Implementation Guide: Neue Features

## Übersicht

Dieser Guide beschreibt die Implementation von 4 neuen Features:
1. **ABORT Command** - Schneller Stop ohne ESTOP
2. **Motion Profiles** - Umschaltbare Geschwindigkeitsprofile
3. **Verbessertes Error Handling** - Recovery und bessere Fehlermeldungen
4. **Self-Test** - Hardware-Validierung

**Empfohlene Reihenfolge:** 1 → 2 → 4 → 3 (Error Handling letztens, ist am komplexesten)

---

# 1. ABORT Command

**Aufwand:** ~30 Minuten | **Schwierigkeit:** ⭐ Einfach

## Ziel
Stop current action + clear queue, OHNE ESTOP auszulösen. Roboter bleibt operational.

## Implementation

### Schritt 1: Command Parser erweitern
**Datei:** `source/proto/cmd.c`

Füge nach dem `RESET` Command hinzu:

```c
// Irgendwo bei den anderen Commands (nach RESET)
if (strcmp(cmd, "ABORT") == 0) {
    bot_abort();
    proto_reply_printf("OK ABORT%s", EOL);
    return;
}
```

### Schritt 2: Abort Funktion in bot_engine
**Datei:** `source/app/bot_engine.h`

Füge Deklaration hinzu:
```c
void bot_abort(void);
```

**Datei:** `source/app/bot_engine.c`

Füge nach `bot_queue_clear()` hinzu:
```c
void bot_abort(void)
{
    // Clear queue
    bot_queue_clear();
    
    // Stop current motion (if any)
    motion_abort();
    
    // Reset state to IDLE (not ERROR, not ESTOP)
    g_status.state = STATE_IDLE;
    g_status.last_err = ERR_NONE;
}
```

### Schritt 3: Motion Abort implementieren
**Datei:** `source/motion/motion.h`

Füge hinzu:
```c
void motion_abort(void);
```

**Datei:** `source/motion/motion.c`

Füge hinzu:
```c
void motion_abort(void)
{
    // Stop all axes immediately
    for (int i = 0; i < AX_N; i++) {
        m.steps_left[i] = 0;
        m.pulse_left[i] = 0;
        step_set((axis_e)i, false);
    }
    
    m.any_active = false;
}
```

### Schritt 4: Job Abort
**Datei:** `source/job/job.h`

Füge hinzu:
```c
void job_abort(void);
```

**Datei:** `source/job/job.c`

Füge hinzu:
```c
void job_abort(void)
{
    j.active = false;
    j.type = JOB_TYPE_NONE;
    j.last_err = ERR_NONE;
}
```

Und in `bot_abort()` auch `job_abort()` aufrufen:
```c
void bot_abort(void)
{
    bot_queue_clear();
    motion_abort();
    job_abort();  // ← NEU
    g_status.state = STATE_IDLE;
    g_status.last_err = ERR_NONE;
}
```

### Testing
```bash
# Via UART/Console:
> MOVE x=100 y=100
> ABORT
OK ABORT
> STATUS
STATE=IDLE
```

**Fertig!** ✅

---

# 2. Motion Profiles

**Aufwand:** ~2-3 Stunden | **Schwierigkeit:** ⭐⭐ Mittel

## Ziel
Umschaltbare Profile: FAST (schnell), PRECISE (langsam/präzise), DEFAULT (normal)

## Implementation

### Schritt 1: Profile-Enum definieren
**Datei:** `source/config/motion_config.h`

Füge am Anfang hinzu:
```c
/* ============================================================================
 * MOTION PROFILES
 * ========================================================================== */

typedef enum {
    MOTION_PROFILE_DEFAULT = 0,
    MOTION_PROFILE_FAST,
    MOTION_PROFILE_PRECISE
} motion_profile_preset_e;

// Profile Presets für XY Bewegungen
// DEFAULT: Normale Geschwindigkeit
#define PROFILE_DEFAULT_START_SPS       200u
#define PROFILE_DEFAULT_MAX_SPS         2000u
#define PROFILE_DEFAULT_ACCEL_SPS2      4000u

// FAST: Schneller, weniger sanft
#define PROFILE_FAST_START_SPS          300u
#define PROFILE_FAST_MAX_SPS            3000u
#define PROFILE_FAST_ACCEL_SPS2         6000u

// PRECISE: Langsam und präzise
#define PROFILE_PRECISE_START_SPS       100u
#define PROFILE_PRECISE_MAX_SPS         1000u
#define PROFILE_PRECISE_ACCEL_SPS2      2000u
```

### Schritt 2: Aktives Profil speichern
**Datei:** `source/motion/motion.c`

Füge globale Variable hinzu:
```c
// Am Anfang der Datei, bei anderen globals
static motion_profile_preset_e g_active_profile = MOTION_PROFILE_DEFAULT;
```

Füge Funktion hinzu:
```c
void motion_set_profile(motion_profile_preset_e profile)
{
    g_active_profile = profile;
}

motion_profile_preset_e motion_get_profile(void)
{
    return g_active_profile;
}

// Helper: Gibt motion_profile_s basierend auf aktuellem Preset
motion_profile_s motion_get_current_xy_profile(void)
{
    motion_profile_s profile;
    
    switch (g_active_profile) {
        case MOTION_PROFILE_FAST:
            profile.start_step_rate_sps = PROFILE_FAST_START_SPS;
            profile.max_step_rate_sps = PROFILE_FAST_MAX_SPS;
            profile.accel_sps2 = PROFILE_FAST_ACCEL_SPS2;
            break;
            
        case MOTION_PROFILE_PRECISE:
            profile.start_step_rate_sps = PROFILE_PRECISE_START_SPS;
            profile.max_step_rate_sps = PROFILE_PRECISE_MAX_SPS;
            profile.accel_sps2 = PROFILE_PRECISE_ACCEL_SPS2;
            break;
            
        case MOTION_PROFILE_DEFAULT:
        default:
            profile.start_step_rate_sps = PROFILE_DEFAULT_START_SPS;
            profile.max_step_rate_sps = PROFILE_DEFAULT_MAX_SPS;
            profile.accel_sps2 = PROFILE_DEFAULT_ACCEL_SPS2;
            break;
    }
    
    return profile;
}
```

**Datei:** `source/motion/motion.h`

Füge hinzu:
```c
#include "motion_config.h"

void motion_set_profile(motion_profile_preset_e profile);
motion_profile_preset_e motion_get_profile(void);
motion_profile_s motion_get_current_xy_profile(void);
```

### Schritt 3: Profile in MOVE verwenden
**Datei:** `source/job/job_move.c`

Ersetze die hardcoded Profile durch dynamische:

**ALT:**
```c
static const motion_profile_s g_xy_profile = {
    STEP_RATE_START_SPS,
    STEP_RATE_MAX_SPS,
    STEP_ACCEL_SPS2
};
```

**NEU:**
```c
// Entferne die statische Definition, nutze stattdessen:
// (Bei jedem move_step() Aufruf)

err_e move_step(void) {
    // ...
    case MOVE_STATE_MOVING:
        motion_profile_s xy_profile = motion_get_current_xy_profile();
        err_e e = motion_start_coordinated(
            delta_x, delta_y, delta_z, delta_phi,
            &xy_profile,  // ← Aktuelles Profil verwenden
            &g_z_profile,
            &g_phi_profile,
            limit_none
        );
    // ...
}
```

### Schritt 4: PROFILE Command
**Datei:** `source/proto/cmd.c`

Füge hinzu:
```c
// Nach ABORT Command
if (strcmp(cmd, "PROFILE") == 0) {
    char *arg = strtok(NULL, " \t");
    if (!arg) {
        proto_reply_printf("ERR SYNTAX PROFILE <DEFAULT|FAST|PRECISE>%s", EOL);
        return;
    }
    
    motion_profile_preset_e profile;
    if (strcmp(arg, "DEFAULT") == 0) {
        profile = MOTION_PROFILE_DEFAULT;
    } else if (strcmp(arg, "FAST") == 0) {
        profile = MOTION_PROFILE_FAST;
    } else if (strcmp(arg, "PRECISE") == 0) {
        profile = MOTION_PROFILE_PRECISE;
    } else {
        proto_reply_printf("ERR SYNTAX unknown profile: %s%s", arg, EOL);
        return;
    }
    
    motion_set_profile(profile);
    proto_reply_printf("OK PROFILE %s%s", arg, EOL);
    return;
}
```

### Schritt 5: Profile im STATUS anzeigen
**Datei:** `source/proto/cmd.c`

Im STATUS Command, füge hinzu:
```c
// Nach den anderen STATUS Ausgaben:
const char *profile_name = "DEFAULT";
switch (motion_get_profile()) {
    case MOTION_PROFILE_FAST: profile_name = "FAST"; break;
    case MOTION_PROFILE_PRECISE: profile_name = "PRECISE"; break;
    default: break;
}
proto_reply_printf("PROFILE=%s%s", profile_name, EOL);
```

### Testing
```bash
> PROFILE FAST
OK PROFILE FAST
> STATUS
PROFILE=FAST
> MOVE x=100 y=100
# → Bewegt sich schneller

> PROFILE PRECISE
OK PROFILE PRECISE
> MOVE x=0 y=0
# → Bewegt sich langsamer
```

**Fertig!** ✅

---

# 3. Self-Test

**Aufwand:** ~1-2 Stunden | **Schwierigkeit:** ⭐⭐ Mittel

## Ziel
Hardware-Test: Endschalter, Encoder, Magnet, Mini-Bewegungen

## Implementation

### Schritt 1: Selftest Module erstellen
**Neue Datei:** `source/debug_tools/selftest/selftest.h`

```c
#ifndef DEBUG_TOOLS_SELFTEST_H_
#define DEBUG_TOOLS_SELFTEST_H_

#include <stdbool.h>

typedef enum {
    SELFTEST_PASS = 0,
    SELFTEST_FAIL,
    SELFTEST_WARN
} selftest_result_e;

typedef struct {
    selftest_result_e limit_switches;
    selftest_result_e encoder_x;
    selftest_result_e encoder_y;
    selftest_result_e magnet;
    selftest_result_e motion_x;
    selftest_result_e motion_y;
    selftest_result_e motion_z;
    selftest_result_e motion_phi;
} selftest_results_s;

void selftest_run(selftest_results_s *results);
const char* selftest_result_to_str(selftest_result_e result);

#endif
```

**Neue Datei:** `source/debug_tools/selftest/selftest.c`

```c
#include "robot_config.h"
#include "selftest.h"
#include "io.h"
#include "position.h"
#include "motion.h"
#include "debug.h"

const char* selftest_result_to_str(selftest_result_e result)
{
    switch (result) {
        case SELFTEST_PASS: return "PASS";
        case SELFTEST_WARN: return "WARN";
        case SELFTEST_FAIL: return "FAIL";
        default: return "UNKNOWN";
    }
}

static selftest_result_e test_limit_switches(void)
{
    debug_printf("Testing limit switches...\r\n");
    
    // Poll switches
    poll_limit_switch();
    
    // Check if any switch is pressed (should not be in normal position)
    if (g_status.limits.x_now || g_status.limits.y_now || g_status.limits.z_now) {
        debug_printf("  WARN: Limit switch already pressed\r\n");
        return SELFTEST_WARN;
    }
    
    debug_printf("  PASS: Limit switches OK\r\n");
    return SELFTEST_PASS;
}

static selftest_result_e test_encoder(axis_e axis)
{
    debug_printf("Testing encoder %c...\r\n", 'X' + axis);
    
    int32_t start, end;
    
    if (axis == AX_X) {
        start = position_get_x_counts();
    } else if (axis == AX_Y) {
        start = position_get_y_counts();
    } else {
        return SELFTEST_PASS; // Z/Phi haben keine Encoder
    }
    
    // TODO: Mini-Bewegung und Check ob Encoder sich ändert
    // Für jetzt: Prüfe nur ob Encoder lesbar ist
    
    if (axis == AX_X) {
        end = position_get_x_counts();
    } else {
        end = position_get_y_counts();
    }
    
    // Wenn Position sich nicht wild geändert hat → OK
    if (abs(end - start) < 100) {
        debug_printf("  PASS: Encoder %c readable\r\n", 'X' + axis);
        return SELFTEST_PASS;
    }
    
    debug_printf("  WARN: Encoder %c unstable\r\n", 'X' + axis);
    return SELFTEST_WARN;
}

static selftest_result_e test_magnet(void)
{
    debug_printf("Testing magnet...\r\n");
    
    // Turn on
    magnet_on_off(true);
    
    // Wait a bit
    for (volatile int i = 0; i < 100000; i++);
    
    // Turn off
    magnet_on_off(false);
    
    debug_printf("  PASS: Magnet toggle OK\r\n");
    return SELFTEST_PASS;
}

static selftest_result_e test_motion_axis(axis_e axis)
{
    debug_printf("Testing motion %c...\r\n", 'X' + axis);
    
    // TODO: Mini-Bewegung (10 steps vor + zurück)
    // Für jetzt: Nur Step-Pin toggle test
    
    // Toggle step pin
    step_set(axis, true);
    for (volatile int i = 0; i < 10000; i++);
    step_set(axis, false);
    
    debug_printf("  PASS: Motion %c step toggle OK\r\n", 'X' + axis);
    return SELFTEST_PASS;
}

void selftest_run(selftest_results_s *results)
{
    debug_printf("\r\n=== SELF-TEST START ===\r\n\r\n");
    
    results->limit_switches = test_limit_switches();
    results->encoder_x = test_encoder(AX_X);
    results->encoder_y = test_encoder(AX_Y);
    results->magnet = test_magnet();
    results->motion_x = test_motion_axis(AX_X);
    results->motion_y = test_motion_axis(AX_Y);
    results->motion_z = test_motion_axis(AX_Z);
    results->motion_phi = test_motion_axis(AX_PHI);
    
    debug_printf("\r\n=== SELF-TEST COMPLETE ===\r\n");
}
```

### Schritt 2: SELFTEST Command
**Datei:** `source/proto/cmd.c`

Füge Include hinzu:
```c
#include "selftest.h"
```

Füge Command hinzu:
```c
if (strcmp(cmd, "SELFTEST") == 0) {
    selftest_results_s results;
    selftest_run(&results);
    
    // Send results
    proto_reply_printf("SELFTEST%s", EOL);
    proto_reply_printf("  LIMITS=%s%s", 
        selftest_result_to_str(results.limit_switches), EOL);
    proto_reply_printf("  ENCODER_X=%s%s", 
        selftest_result_to_str(results.encoder_x), EOL);
    proto_reply_printf("  ENCODER_Y=%s%s", 
        selftest_result_to_str(results.encoder_y), EOL);
    proto_reply_printf("  MAGNET=%s%s", 
        selftest_result_to_str(results.magnet), EOL);
    proto_reply_printf("  MOTION_X=%s%s", 
        selftest_result_to_str(results.motion_x), EOL);
    proto_reply_printf("  MOTION_Y=%s%s", 
        selftest_result_to_str(results.motion_y), EOL);
    proto_reply_printf("  MOTION_Z=%s%s", 
        selftest_result_to_str(results.motion_z), EOL);
    proto_reply_printf("  MOTION_PHI=%s%s", 
        selftest_result_to_str(results.motion_phi), EOL);
    proto_reply_printf("OK SELFTEST%s", EOL);
    return;
}
```

### Schritt 3: Erweiterte Tests (Optional)
Später kannst du die Tests erweitern:
- Mini-Bewegung (10 Steps) und prüfen ob Encoder sich ändert
- Prüfe ob Magnet Strom zieht
- Home-Test (fahre zu Endschalter und zurück)

### Testing
```bash
> SELFTEST
SELFTEST
  LIMITS=PASS
  ENCODER_X=PASS
  ENCODER_Y=PASS
  MAGNET=PASS
  MOTION_X=PASS
  MOTION_Y=PASS
  MOTION_Z=PASS
  MOTION_PHI=PASS
OK SELFTEST
```

**Fertig!** ✅

---

# 4. Verbessertes Error Handling

**Aufwand:** ~1-2 Stunden | **Schwierigkeit:** ⭐⭐ Mittel

## Ziel
- Bessere Error-Messages mit Details
- Recovery-Mechanismus
- Error-History

## Implementation

### Schritt 1: Erweiterte Error Info
**Datei:** `source/proto/protocol.h`

Erweitere Error Enum:
```c
typedef enum {
    ERR_NONE = 0,
    ERR_SYNTAX,
    ERR_RANGE,
    ERR_NO_HOME,
    ERR_NO_PART,
    ERR_PLACE_FAIL,
    ERR_MOTOR,
    ERR_ESTOP,
    ERR_INTERNAL,
    ERR_NOT_IMPLEMENTED,
    ERR_QUEUE_FULL,        // NEU
    ERR_POSITION_DRIFT,    // NEU
    ERR_TIMEOUT,           // NEU
    ERR_LIMIT_SWITCH       // NEU
} err_e;
```

Füge Error-Detail Struktur hinzu:
```c
typedef struct {
    err_e code;
    char message[64];  // Zusätzliche Details
    uint32_t timestamp_ms;
    axis_e axis;       // Bei Achsen-spezifischen Fehlern
} error_info_s;

#define ERROR_HISTORY_SIZE 8

typedef struct {
    error_info_s history[ERROR_HISTORY_SIZE];
    uint8_t count;
    uint8_t write_idx;
} error_history_s;
```

Erweitere `bot_status_s`:
```c
typedef struct {
    bot_state_e state;
    bool homed;
    bool has_part;
    bool estop;
    err_e last_err;
    error_info_s last_error_info;  // NEU: Detaillierte Info
    error_history_s error_history;  // NEU: History
    
    robot_pos_s pos_internal;
    robot_pos_s pos_measured;
    limits_s limits;
} bot_status_s;
```

### Schritt 2: Error Recording
**Datei:** `source/proto/protocol.c`

Füge Helper-Funktionen hinzu:
```c
void record_error(err_e code, const char *message, axis_e axis)
{
    // Update last_error_info
    g_status.last_error_info.code = code;
    g_status.last_error_info.axis = axis;
    strncpy(g_status.last_error_info.message, message, 
            sizeof(g_status.last_error_info.message) - 1);
    g_status.last_error_info.timestamp_ms = get_uptime_ms(); // Du brauchst uptime counter
    
    // Add to history
    error_history_s *hist = &g_status.error_history;
    hist->history[hist->write_idx] = g_status.last_error_info;
    hist->write_idx = (hist->write_idx + 1) % ERROR_HISTORY_SIZE;
    if (hist->count < ERROR_HISTORY_SIZE) {
        hist->count++;
    }
    
    // Update legacy field
    g_status.last_err = code;
}

void clear_error(void)
{
    g_status.last_err = ERR_NONE;
    g_status.last_error_info.code = ERR_NONE;
    g_status.last_error_info.message[0] = '\0';
}
```

**Datei:** `source/proto/protocol.h`

```c
void record_error(err_e code, const char *message, axis_e axis);
void clear_error(void);
```

### Schritt 3: err_to_str erweitern
**Datei:** `source/proto/protocol.c`

Erweitere `err_to_str()`:
```c
const char *err_to_str(err_e e)
{
    switch (e) {
        case ERR_NONE: return "NONE";
        case ERR_SYNTAX: return "SYNTAX";
        case ERR_RANGE: return "RANGE";
        case ERR_NO_HOME: return "NO_HOME";
        case ERR_NO_PART: return "NO_PART";
        case ERR_PLACE_FAIL: return "PLACE_FAIL";
        case ERR_MOTOR: return "MOTOR";
        case ERR_ESTOP: return "ESTOP";
        case ERR_INTERNAL: return "INTERNAL";
        case ERR_NOT_IMPLEMENTED: return "NOT_IMPLEMENTED";
        case ERR_QUEUE_FULL: return "QUEUE_FULL";       // NEU
        case ERR_POSITION_DRIFT: return "POSITION_DRIFT"; // NEU
        case ERR_TIMEOUT: return "TIMEOUT";               // NEU
        case ERR_LIMIT_SWITCH: return "LIMIT_SWITCH";     // NEU
        default: return "UNKNOWN";
    }
}
```

### Schritt 4: RECOVER Command
**Datei:** `source/proto/cmd.c`

```c
if (strcmp(cmd, "RECOVER") == 0) {
    // Check if recovery is possible
    if (g_status.estop) {
        proto_reply_printf("ERR CANNOT_RECOVER estop_active%s", EOL);
        return;
    }
    
    err_e last = g_status.last_err;
    
    // Check if error is recoverable
    if (last == ERR_LIMIT_SWITCH || last == ERR_MOTOR || 
        last == ERR_POSITION_DRIFT || last == ERR_TIMEOUT) {
        
        // Clear error state
        clear_error();
        g_status.state = STATE_IDLE;
        bot_queue_clear();
        
        // Reset limit switches if that was the issue
        if (last == ERR_LIMIT_SWITCH) {
            reset_limit_switch(limit_x | limit_y | limit_z);
        }
        
        proto_reply_printf("OK RECOVER from %s%s", err_to_str(last), EOL);
        return;
    }
    
    proto_reply_printf("ERR CANNOT_RECOVER error_not_recoverable%s", EOL);
    return;
}
```

### Schritt 5: ERROR Command (Error History anzeigen)
**Datei:** `source/proto/cmd.c`

```c
if (strcmp(cmd, "ERRORS") == 0) {
    error_history_s *hist = &g_status.error_history;
    
    proto_reply_printf("ERROR_HISTORY count=%u%s", (unsigned)hist->count, EOL);
    
    for (uint8_t i = 0; i < hist->count; i++) {
        uint8_t idx = (hist->write_idx - hist->count + i + ERROR_HISTORY_SIZE) 
                      % ERROR_HISTORY_SIZE;
        error_info_s *err = &hist->history[idx];
        
        proto_reply_printf("  %u: %s axis=%c msg=\"%s\" t=%lu%s",
            i,
            err_to_str(err->code),
            (err->axis < AX_N) ? ('X' + err->axis) : '-',
            err->message,
            (unsigned long)err->timestamp_ms,
            EOL);
    }
    
    proto_reply_printf("OK ERRORS%s", EOL);
    return;
}
```

### Schritt 6: Fehler besser aufzeichnen
**Datei:** `source/motion/motion.c`

Ersetze Fehler-Returns durch `record_error()`:

**ALT:**
```c
if (limit_hit) {
    return ERR_MOTOR;
}
```

**NEU:**
```c
if (limit_hit) {
    char msg[64];
    snprintf(msg, sizeof(msg), "Limit switch hit during motion");
    record_error(ERR_LIMIT_SWITCH, msg, (axis_e)i);
    return ERR_LIMIT_SWITCH;
}
```

Mache das für alle wichtigen Fehler-Stellen.

### Schritt 7: Queue Full Error
**Datei:** `source/app/bot_engine.c`

Bei Queue-Overflow:
```c
// In cmd.c, wenn bot_enqueue() fehlschlägt:
if (!bot_enqueue(&action)) {
    record_error(ERR_QUEUE_FULL, "Action queue full", AX_N);
    proto_reply_printf("ERR QUEUE_FULL%s", EOL);
    return;
}
```

### Testing
```bash
> MOVE x=9999 y=9999
ERR RANGE
> STATUS
ERROR=RANGE
> ERRORS
ERROR_HISTORY count=1
  0: RANGE axis=- msg="Out of bounds" t=12345
OK ERRORS
> RECOVER
OK RECOVER from RANGE
> STATUS
STATE=IDLE ERROR=NONE
```

**Fertig!** ✅

---

# Zusammenfassung & Tipps

## Reihenfolge
1. **ABORT** - Schnell & Einfach, sofort nützlich
2. **Motion Profiles** - Spaßig zu testen
3. **Self-Test** - Nützlich vor Demos
4. **Error Handling** - Komplex, aber am wertvollsten

## Testing-Strategie
1. Nach jedem Feature: Compilieren und Basic-Test
2. Kombiniere Features: z.B. `PROFILE FAST` + `ABORT` testen
3. Edge Cases: Was passiert bei ABORT während PICK?
4. Error Recovery: Provoziere Fehler und teste RECOVER

## Potenzielle Fallstricke
- **Includes:** Nach Umstrukturierung müssen evtl. Include-Pfade angepasst werden
- **Timing:** Motion Profiles können bei sehr schnellen Settings instabil werden
- **Memory:** Error History nimmt ~512 Bytes RAM (8 * 64 bytes)
- **Concurrency:** ABORT während PICK/PLACE → teste alle State-Kombinationen

## Erweiterungsmöglichkeiten
- Motion Profiles: Per Achse (z.B. nur Z langsamer)
- Self-Test: Erweiterte Tests mit echten Bewegungen
- Error Handling: Auto-Recovery für bestimmte Fehler
- Logging: Error History auf SD-Card schreiben

## Code-Konventionen
- Behalte deinen Stil bei (Kommentare, Formatierung)
- Alle neuen Structs mit `_s` Suffix
- Alle neuen Enums mit `_e` Suffix
- Error-Messages in SCREAMING_SNAKE_CASE

Viel Erfolg! 🚀
