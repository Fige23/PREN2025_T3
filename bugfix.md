# PREN PuzzleBot: Fix für hängendes Pick/Place nach XY + saubere ABORT/RESET-Semantik

## Kontext

Beim Test mit finaler Hardware trat folgendes Verhalten auf:

```text
PICK/PLACE startet
XY fährt sauber an Zielposition
Roboter stoppt danach
Z fährt nicht weiter
neue Befehle liefern nur noch "QUEUED ..."
RESET setzt zwar STATUS auf IDLE, aber weitere Befehle werden trotzdem nicht ausgeführt
```

Der erste sichtbare Fehler kam teilweise von der Encoder-Korrektur, weil die Toleranz nicht erreicht wurde. Nach Erhöhen der Toleranz verschwand dieser Error, der Roboter blieb aber trotzdem nach XY stehen.

Die wahrscheinlichste Ursache ist kein Hardwareproblem, sondern ein Firmware-Deadlock in der Job-State-Machine.

---

## Hauptursache

Nach einer XY-Bewegung wechseln `PICK` und `PLACE` in einen kurzen Settle-Wait-State, bevor Z weiterfährt.

Typischer Ablauf:

```text
XY-Motion starten
XY-Motion fertig
Encoder-/Motion-Finish prüfen
in XY_SETTLE wechseln
kurz warten
Z runterfahren
Magnet schalten
Z wieder hochfahren
Job abschliessen
```

Die Wartezeit wird aktuell über `motion_get_isr_tick_count()` gemessen.

Problem:

```text
motion_get_isr_tick_count()
```

zählt nur weiter, solange die Motion Engine aktiv ist.

Nach der XY-Bewegung ist aber keine Motion mehr aktiv:

```text
m.active == false
```

Dadurch bleibt der Tick-Counter stehen. Der Settle-Wait wird nie fertig. Der Pick-/Place-Job bleibt intern aktiv. Die Bot Engine bleibt `busy`. Neue Befehle werden zwar in die Queue gelegt, aber nie gestartet.

---

## Warum `QUEUED` irreführend ist

`QUEUED` bedeutet nur:

```text
Der Befehl wurde erfolgreich in die Queue gelegt.
```

Es bedeutet nicht:

```text
Der Befehl wurde gestartet.
```

Wenn die Bot Engine intern noch mit einem hängenden Job beschäftigt ist, werden neue Befehle weiterhin angenommen, aber nicht abgearbeitet.

---

## Fix 1: Motion-Tick muss auch ohne aktive Bewegung weiterlaufen

### Ziel

Der globale Motion-/System-Tick, der für kurze Wartezeiten in Jobs verwendet wird, darf nicht an `m.active` gekoppelt sein.

### Aktuelles Problem in `motion.c`

Sinngemäss ist die Logik aktuell so aufgebaut:

```c
static void motion_tick_dispatch(void)
{
#if ESTOP_POLL_IN_MOTION_ISR
    static uint32_t estop_poll_div = 0u;
    estop_poll_div++;
    if (estop_poll_div >= ESTOP_POLL_ISR_DIVIDER) {
        estop_poll_div = 0u;
        estop_poll();
    }
#endif

    if (!m.active) {
        return;
    }

    motion_tick_isr();
}
```

Und in `motion_tick_isr()`:

```c
static void motion_tick_isr(void)
{
    isr_tick_count++;
    ...
}
```

Dadurch wird `isr_tick_count` nur erhöht, wenn eine Bewegung aktiv ist.

### Vorgeschlagene Anpassung

`isr_tick_count++` soll in `motion_tick_dispatch()` verschoben werden, also vor den `m.active`-Check.

```c
static void motion_tick_dispatch(void)
{
    /*
     * Globaler Tick für Job-Wartezeiten.
     * Muss auch weiterlaufen, wenn keine Motion aktiv ist,
     * sonst bleiben PICK/PLACE in SETTLE-States hängen.
     */
    isr_tick_count++;

#if ESTOP_POLL_IN_MOTION_ISR
    static uint32_t estop_poll_div = 0u;
    estop_poll_div++;
    if (estop_poll_div >= ESTOP_POLL_ISR_DIVIDER) {
        estop_poll_div = 0u;
        estop_poll();
    }
#endif

    if (!m.active) {
        return;
    }

    motion_tick_isr();
}
```

Danach in `motion_tick_isr()` diese Zeile entfernen:

```c
isr_tick_count++;
```

Sonst würde der Counter während aktiver Bewegung doppelt zählen.

### Erwarteter Effekt

Danach laufen diese Wait-States wieder korrekt weiter:

```text
PICK_STATE_XY_SETTLE
PLACE_STATE_XY_PHI_SETTLE
weitere kurze Job-Wartezeiten
```

---

## Fix 2: Aktiven Job sauber abbrechen können

### Aktuelles Problem

`RESET` setzt aktuell offenbar sichtbare Statusfelder und leert die Queue, aber der intern laufende Job und das `busy`-Flag der Bot Engine werden nicht zwingend zurückgesetzt.

Problematisches Muster:

```c
void bot_step(void)
{
    static bool busy = false;
    static bot_action_s cur;
    ...
}
```

Weil `busy` lokal-statisch in `bot_step()` liegt, kann ein externer Befehl wie `RESET` diesen Zustand nicht sauber zurücksetzen.

Wenn der Bot in einem Job hängt, bleibt intern:

```text
busy == true
```

Auch wenn `STATUS` danach wieder `STATE_IDLE` zeigt.

Das führt zu diesem verwirrenden Zustand:

```text
STATUS: IDLE
neue Befehle: QUEUED
aber nichts startet
```

---

## Fix 3: Bot-Engine-State aus `bot_step()` herausziehen

### Ziel

Die Bot Engine soll von aussen kontrolliert zurückgesetzt oder abgebrochen werden können.

### Vorschlag für `bot_engine.c`

Statt lokaler statischer Variablen in `bot_step()`:

```c
void bot_step(void)
{
    static bool busy = false;
    static bot_action_s cur;
    ...
}
```

besser file-scope:

```c
static bool s_bot_busy = false;
static bot_action_s s_bot_cur;
```

Dann in `bot_step()` überall `busy` durch `s_bot_busy` und `cur` durch `s_bot_cur` ersetzen.

Beispiel:

```c
void bot_step(void)
{
    if (g_status.estop) {
        bot_queue_clear();
        g_status.state = STATE_EMERGENCY_STOP;
        g_status.last_err = ERR_ESTOP;

        if (!s_bot_busy) {
            return;
        }
    }

    if (!s_bot_busy) {
        if (!bot_dequeue(&s_bot_cur)) {
            return;
        }

        if (s_bot_cur.type == ACT_MAGNET) {
            magnet_on_off(s_bot_cur.magnet_on);
            g_status.last_err = ERR_NONE;
            reply_ok_action(&s_bot_cur);
            return;
        }

        g_status.state = busy_state_from_action(s_bot_cur.type);

        err_e e = job_start(&s_bot_cur);
        if (e != ERR_NONE) {
            if (e == ERR_ESTOP) {
                g_status.state = STATE_EMERGENCY_STOP;
                g_status.estop = true;
                bot_queue_clear();
            } else {
                g_status.state = STATE_ERROR;
            }

            g_status.last_err = e;
            reply_err_action(&s_bot_cur, e);
            return;
        }

        s_bot_busy = true;
        return;
    }

    err_e je = ERR_NONE;
    if (!job_step(&je)) {
        return;
    }

    if (je != ERR_NONE) {
        if (je == ERR_ESTOP) {
            g_status.state = STATE_EMERGENCY_STOP;
            g_status.estop = true;
            bot_queue_clear();
        } else {
            g_status.state = STATE_ERROR;
        }

        g_status.last_err = je;
        reply_err_action(&s_bot_cur, je);
        s_bot_busy = false;
        return;
    }

    if (s_bot_cur.type == ACT_PICK) {
        g_status.has_part = true;
    } else if (s_bot_cur.type == ACT_PLACE) {
        g_status.has_part = false;
    }

    g_status.state = STATE_IDLE;
    g_status.last_err = ERR_NONE;
    reply_ok_action(&s_bot_cur);
    s_bot_busy = false;
}
```

---

## Fix 4: `job_abort()` einführen

### Ziel

Das Job-System soll einen laufenden Job gezielt abbrechen können.

In `job.h` ergänzen:

```c
void job_abort(void);
```

In `job.c` implementieren, abhängig davon wie der globale Job-State aktuell heisst.

Beispiel-Schema:

```c
void job_abort(void)
{
    /*
     * Aktiven Job verwerfen.
     * Wichtig: keine finale OK/ERR-Antwort mehr senden,
     * wenn der Abbruch durch ABORT/RESET ausgelöst wurde.
     */
    j.active = false;
    j.last_err = ERR_NONE;
    j.type = JOB_TYPE_NONE;

    /* Falls vorhanden: Sub-State-Machines zurücksetzen */
    job_pick_reset();
    job_place_reset();
    job_home_reset();
    job_motion_finish_reset();
}
```

Falls es diese Sub-Reset-Funktionen noch nicht gibt, reicht für den ersten Schritt eventuell auch:

```c
void job_abort(void)
{
    job_init();
}
```

Das ist weniger elegant, aber für den Wettbewerb wahrscheinlich robust genug.

---

## Fix 5: `motion_abort()` einführen

### Ziel

Ein Abbruch soll auch eine aktuell laufende Motion sofort stoppen.

In `motion.h` ergänzen:

```c
void motion_abort(void);
```

In `motion.c`:

```c
void motion_abort(void)
{
    /* Motion sofort stoppen */
    m.active = false;
    m.done = true;
    m.err = ERR_ABORTED;   /* falls ERR_ABORTED existiert */

    for (int i = 0; i < AX_N; i++) {
        m.pulse_left[i] = 0u;
        step_set((axis_e)i, false);
    }
}
```

Falls `ERR_ABORTED` noch nicht existiert, gibt es zwei Möglichkeiten:

### Variante A: neuen Error ergänzen

In der Error-Enum:

```c
ERR_ABORTED,
```

und in `err_to_str()`:

```c
case ERR_ABORTED: return "ERR_ABORTED";
```

### Variante B: ohne neuen Error

Für einen bewusst ausgelösten Abbruch kann `motion_abort()` auch intern nur stoppen und den Bot-State danach direkt auf IDLE setzen.

```c
void motion_abort(void)
{
    m.active = false;
    m.done = true;
    m.err = ERR_NONE;

    for (int i = 0; i < AX_N; i++) {
        m.pulse_left[i] = 0u;
        step_set((axis_e)i, false);
    }
}
```

Für Debugging ist Variante A sauberer.

---

## Fix 6: `bot_abort_current()` einführen

### Ziel

Zentraler Abbruchpunkt für:

```text
laufende Motion stoppen
laufenden Job abbrechen
Queue optional leeren
Bot Engine auf nicht-busy setzen
Status sinnvoll setzen
```

In `bot_engine.h` ergänzen:

```c
void bot_abort_current(bool clear_queue);
void bot_reset_engine(void);
```

In `bot_engine.c`:

```c
void bot_abort_current(bool clear_queue)
{
    motion_abort();
    job_abort();

    if (clear_queue) {
        bot_queue_clear();
    }

    s_bot_busy = false;
    g_status.state = STATE_IDLE;
    g_status.last_err = ERR_NONE;
}
```

Dafür muss `bot_engine.c` zusätzlich `motion.h` inkludieren:

```c
#include "motion.h"
```

---

## ABORT vs RESET: empfohlene Semantik

Ja, es ist sinnvoll, `ABORT` und `RESET` zu trennen.

Der Grund: Der Reset-Knopf am PuzzleBot bedeutet für den Benutzer nicht zwingend dasselbe wie ein technischer Firmware-Abbruch.

---

## Vorgeschlagene Bedeutung von `ABORT`

`ABORT` ist ein technischer Sofortabbruch.

Verwendung:

```text
aktueller Befehl hängt
falscher Befehl wurde gesendet
Roboter soll sofort aufhören
Debugging
Raspberry Pi möchte laufenden Job abbrechen
```

`ABORT` sollte:

```text
aktuelle Motion stoppen
aktuellen Job abbrechen
Queue leeren
Magnet optional unverändert lassen oder sicher ausschalten
Bot Engine auf IDLE setzen
Homing-Status NICHT löschen
Position möglichst NICHT löschen
has_part möglichst NICHT automatisch ändern
```

Empfohlene Antwort:

```text
OK ABORT
```

Danach soll der Roboter wieder Befehle annehmen.

### Wichtig

`ABORT` sollte nicht automatisch `homed = false` setzen. Sonst wäre jeder kleine Abbruch im Wettbewerb teuer, weil danach wieder Homing nötig wäre.

---

## Vorgeschlagene Bedeutung von `RESET`

`RESET` ist ein logischer Neustart des Puzzle-Ablaufs.

Verwendung:

```text
Puzzle neu beginnen
Raspberry Pi / GUI Reset-Knopf
neuer Versuch am Wettbewerb
alter Ablauf verwerfen
Startzustand wiederherstellen
```

`RESET` sollte intern zuerst dasselbe tun wie `ABORT`, aber zusätzlich logische Spiel-/Ablaufzustände zurücksetzen.

`RESET` sollte:

```text
aktuelle Motion stoppen
aktuellen Job abbrechen
Queue leeren
Bot Engine auf IDLE setzen
last_err löschen
has_part = false setzen
Magnet ausschalten
optional: aktuelle Plan-/Command-ID zurücksetzen
optional: Puzzle-/Run-State zurücksetzen
```

Nicht zwingend zurücksetzen:

```text
homed
pos_internal
pos_measured
```

Denn wenn der Roboter mechanisch nicht verschoben wurde, ist es für den Wettbewerb besser, die bekannte Position zu behalten.

---

## Empfohlene praktische Semantik für euren Wettbewerb

### `ABORT`

```text
Stoppe, was du gerade tust, aber behalte deine Weltkenntnis.
```

Also:

```text
Position bleibt gültig
homed bleibt gültig
has_part bleibt wie es ist oder wird bewusst nicht verändert
Queue wird gelöscht
Bot geht auf IDLE
```

### `RESET`

```text
Starte den Puzzle-Ablauf neu, aber ohne zwingend neu zu homen.
```

Also:

```text
macht intern ABORT
Queue wird gelöscht
Magnet OFF
has_part = false
last_err = ERR_NONE
state = IDLE
homed bleibt erhalten
Position bleibt erhalten
```

### `CLEAR_ESTOP`

Bleibt separat:

```text
Nur E-Stop-Latch lösen, falls Hardware-E-Stop wieder frei ist.
```

---

## Beispiel: Implementierung in `cmd.c`

### Neuer Befehl `ABORT`

In der Command-Erkennung ergänzen:

```c
if (streq(cmd, "ABORT")) {
    bot_abort_current(true);
    proto_reply("OK ABORT\n");
    return;
}
```

### Angepasster Befehl `RESET`

```c
if (streq(cmd, "RESET")) {
    bot_reset_engine();
    proto_reply("OK RESET\n");
    return;
}
```

---

## Beispiel: `bot_reset_engine()`

```c
void bot_reset_engine(void)
{
    bot_abort_current(true);

    magnet_on_off(false);

    g_status.has_part = false;
    g_status.last_err = ERR_NONE;
    g_status.state = STATE_IDLE;

    /*
     * Empfehlung:
     * homed und Position NICHT löschen.
     * RESET bedeutet Puzzle-Ablauf neu starten,
     * nicht zwingend Roboter-Koordinatensystem verlieren.
     */
}
```

Falls ihr einen wirklich harten Reset wollt, wäre ein separater Befehl sinnvoll:

```text
RESET_ALL
```

oder

```text
SOFT_REBOOT
```

Dieser könnte dann zusätzlich `homed = false` setzen oder sogar einen MCU-Neustart auslösen.

---

## Empfohlene Befehlstabelle

| Befehl | Zweck | Löscht Queue | Stoppt Motion | Löscht Fehler | Löscht Homing | Löscht Position | Magnet |
|---|---|---:|---:|---:|---:|---:|---|
| `ABORT` | laufenden Job abbrechen | ja | ja | ja | nein | nein | unverändert oder OFF |
| `RESET` | Puzzle-Ablauf neu starten | ja | ja | ja | nein | nein | OFF |
| `CLEAR_ESTOP` | E-Stop entriegeln | optional | nein | ja, falls frei | nein | nein | unverändert |
| `HOME` | Referenzfahrt | nein | startet Job | ja bei Erfolg | setzt true | setzt Position | unverändert |
| `SET_POS` | Position manuell setzen | nein | nein | nein | optional true | ja | unverändert |

Empfehlung für Magnet bei `ABORT`:

Für Debugging kann `ABORT` den Magnet unverändert lassen. Für Wettbewerbssicherheit kann `ABORT` den Magnet ausschalten.

Ich würde für euren Bot eher sagen:

```text
ABORT: Magnet unverändert
RESET: Magnet OFF
ESTOP: Magnet OFF, falls sicherheitstechnisch gewünscht
```

Grund: Wenn während einem Pick etwas schiefgeht, kann ein automatisches `MAGNET OFF` das Teil fallen lassen. Bei `RESET` ist das egal oder sogar gewünscht.

---

## Weitere Robustheitsverbesserungen

### 1. Watchdog für Job-States

Jeder Pick-/Place-State sollte optional ein Timeout haben.

Beispiel:

```text
XY_SETTLE darf maximal 500 ms dauern
Z_DOWN darf maximal x Sekunden dauern
MOTION_FINISH darf maximal x Sekunden dauern
```

Falls ein State zu lange hängt:

```text
ERR_JOB_TIMEOUT
```

Vorteil:

```text
Roboter hängt nie still und schweigend.
```

---

### 2. Debug-Status um aktiven Job erweitern

`STATUS` sollte nicht nur `STATE_IDLE`, `STATE_PICKING`, etc. anzeigen, sondern auch:

```text
busy=0/1
queue_count=n
active_job=PICK/PLACE/MOVE/HOME/NONE
job_state=XY_SETTLE/Z_DOWN/...
motion_active=0/1
motion_done=0/1
isr_tick=...
```

Gerade `job_state` hätte den aktuellen Fehler sofort sichtbar gemacht.

Beispiel-Ausgabe:

```text
STATUS state=PICKING busy=1 job=PICK job_state=XY_SETTLE motion_active=0 queue=0 err=NONE
```

---

### 3. `QUEUED` aussagekräftiger machen

Aktuell kann `QUEUED` so wirken, als ob der Befehl bald läuft. Besser:

```text
QUEUED MOVE id=12 queue=2 busy=1
```

Dann sieht man sofort:

```text
Der Befehl liegt nur in der Queue, aber die Bot Engine ist noch beschäftigt.
```

---

### 4. RESET darf nicht nur sichtbaren Status setzen

Wichtige Regel:

```text
Kein Befehl darf nur g_status.state ändern, wenn interne State-Machines weiterlaufen.
```

Sonst entstehen Fake-Zustände:

```text
STATUS sagt IDLE
intern ist der Bot aber busy
```

Darum sollten Befehle wie `RESET`, `ABORT`, eventuell `ESTOP` immer zentrale Engine-Funktionen aufrufen.

---

### 5. Einheitliche State-Machine-Reset-Funktionen

Für jedes grössere Modul lohnt sich eine Reset-/Abort-Funktion:

```c
void motion_abort(void);
void job_abort(void);
void bot_abort_current(bool clear_queue);
void bot_reset_engine(void);
```

Optional später:

```c
void pick_reset(void);
void place_reset(void);
void home_reset(void);
void motion_finish_reset(void);
```

Das macht Debugging massiv einfacher.

---

## Minimaler Patch für den Wettbewerb

Falls wenig Zeit bleibt, reichen wahrscheinlich diese drei Änderungen:

### 1. `isr_tick_count++` in `motion_tick_dispatch()` verschieben

Damit Pick/Place nicht mehr im Settle-State hängen bleibt.

### 2. `busy` in `bot_engine.c` file-scope machen

Damit es von Reset-/Abort-Funktionen zurückgesetzt werden kann.

### 3. `ABORT` und `RESET` zentral implementieren

Minimal:

```c
void bot_abort_current(bool clear_queue)
{
    motion_abort();
    job_init();

    if (clear_queue) {
        bot_queue_clear();
    }

    s_bot_busy = false;
    g_status.state = STATE_IDLE;
    g_status.last_err = ERR_NONE;
}

void bot_reset_engine(void)
{
    bot_abort_current(true);
    magnet_on_off(false);
    g_status.has_part = false;
}
```

---

## Testplan nach Anpassung

### Test 1: Tick läuft ohne Motion

Nach Boot:

```text
STATUS
warten
STATUS
```

Falls `isr_tick` in STATUS eingebaut wird, muss der Wert steigen, auch wenn keine Bewegung läuft.

---

### Test 2: Pick ohne Encoder-Korrektur

Closed-loop temporär deaktivieren oder Toleranz gross setzen.

```text
SET_POS x=0 y=0 z=0 phi=0
PICK x=50 y=50
```

Erwartung:

```text
XY fährt
kurzer Wait
Z fährt runter
Magnet ON
Z fährt hoch
OK PICK
```

---

### Test 3: ABORT während Pick

```text
PICK x=50 y=50
ABORT
STATUS
MOVE x=10 y=10 z=0 phi=0
```

Erwartung:

```text
OK ABORT
STATUS state=IDLE busy=0
MOVE wird danach wirklich ausgeführt
```

---

### Test 4: RESET während hängendem Zustand

Falls ein Job absichtlich blockiert wird:

```text
RESET
STATUS
MOVE x=10 y=10 z=0 phi=0
```

Erwartung:

```text
OK RESET
STATUS state=IDLE busy=0 has_part=0
MOVE startet wirklich
```

---

## Fazit

Der konkrete Fehler ist sehr wahrscheinlich:

```text
Pick/Place wartet nach XY auf einen Tick,
der nur während aktiver Motion weiterzählt.
```

Dadurch bleibt der Job intern aktiv, obwohl der Roboter mechanisch steht.

Die wichtigste Änderung ist deshalb:

```text
Der Tick-Counter muss auch ohne aktive Motion weiterlaufen.
```

Zusätzlich sollte `RESET` nicht nur den sichtbaren Status ändern, sondern die internen State-Machines sauber zurücksetzen. Für die Bedienung ist eine Trennung sinnvoll:

```text
ABORT = aktueller technischer Abbruch
RESET = Puzzle-Ablauf neu starten
```

Für den Wettbewerb ist die beste Semantik wahrscheinlich:

```text
ABORT: Stoppen, Weltwissen behalten
RESET: Ablauf neu starten, Position/Homing behalten, Magnet aus, has_part false
```
