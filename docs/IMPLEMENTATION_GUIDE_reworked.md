# Implementation Guide: Finalisierung der Bot-Logik

## Ziel dieses Dokuments

Der Bot ist funktional bereits weit fortgeschritten.  
Homing, Move, Pick, Place, Queue und Protokoll sind vorhanden. Die nächsten Schritte betreffen deshalb nicht mehr die Grundfunktion, sondern die **saubere Finalisierung des Gesamtsystems**.

Der Fokus liegt auf:

1. **Queue-konsistenter Command-Annahme**
2. **Sauberem Abort-/Reset-Verhalten**
3. **Klarer Betriebssemantik für Informatik-Schnittstelle**
4. **Bereinigung kleiner Inkonsistenzen**
5. **Definition offener Entscheidungen**

Dieses Dokument ersetzt ältere Vorschläge wie Motion Profiles oder Self-Test als unmittelbare Priorität. Solche Features können später ergänzt werden, sind aber **nicht entscheidend**, um den Bot als zuverlässig bedienbar zu machen.

---

# 1. Zielbild des Systems

## Gewünschtes Verhalten

Die Informatiker sollen dem Bot einen **ganzen Block von Instruktionen** schicken können, z. B.:

```text
PICK x=...
PLACE x=... y=... phi=...
PICK x=...
PLACE x=... y=... phi=...
...
```

Der Bot soll diese Befehle **bereits beim Enqueue logisch prüfen**, und zwar **nicht nur gegen den aktuellen Ist-Zustand**, sondern gegen den **erwarteten zukünftigen Zustand nach Abarbeitung aller bereits akzeptierten Jobs**.

Damit gilt:

- ein `PICK` darf nur angenommen werden, wenn der Bot **zum Ausführungszeitpunkt voraussichtlich kein Teil hat**
- ein `PLACE` darf nur angenommen werden, wenn der Bot **zum Ausführungszeitpunkt voraussichtlich ein Teil hat**

So können sinnvolle Blöcke vorgängig geschickt werden, während unsinnige Sequenzen bereits beim Enqueue abgelehnt werden.

## Beispiel

### Soll erlaubt sein
```text
PICK x=10 y=10
PLACE x=100 y=100 phi=90
PICK x=20 y=20
PLACE x=120 y=100 phi=0
```

### Soll abgelehnt werden
```text
PICK x=10 y=10
PICK x=20 y=20
```

weil nach dem ersten akzeptierten `PICK` der Bot in der geplanten Zukunft bereits ein Teil hat.

Ebenso:

```text
PLACE x=100 y=100 phi=90
```

wenn aktuell und auch in der geplanten Zukunft kein Teil vorhanden ist.

---

# 2. Höchste Priorität: Queue-konsistente Zustandsprüfung

## Problem

Aktuell prüfen `cmd_pick()` und `cmd_place()` nur den momentanen Zustand `g_status.has_part`.

Das ist für ein asynchrones Queue-System nicht ausreichend.  
Ein Befehl kann beim Enqueue gültig wirken, aber zum Zeitpunkt seiner späteren Ausführung ungültig sein.

## Ziel

Die Annahme eines neuen Commands muss auf einem **virtuellen Zustand** basieren:

- aktueller Ist-Zustand
- aktiver Job
- bereits in Queue akzeptierte Jobs
- neues Command

## Empfehlung

Eine kleine zentrale Hilfsfunktion einführen, z. B.:

```c
bool bot_predict_has_part_after_queue(bool *out_has_part);
```

oder allgemeiner:

```c
typedef struct {
    bool has_part;
    bool busy;
} bot_future_state_s;

bool bot_predict_future_state(bot_future_state_s *out);
```

Noch besser ist eine kleine Prüffunktion pro neuer Action:

```c
bool bot_can_accept_action(const bot_action_s *candidate, err_e *out_err);
```

Diese Funktion simuliert logisch:

1. Start bei aktuellem Zustand:
   - `has_part = g_status.has_part`
2. Falls ein aktiver Job läuft:
   - wenn aktiver Job `ACT_PICK` ist, danach `has_part = true`
   - wenn aktiver Job `ACT_PLACE` ist, danach `has_part = false`
3. Alle Queue-Einträge der Reihe nach durchlaufen:
   - `ACT_PICK` setzt virtuell `has_part = true`
   - `ACT_PLACE` setzt virtuell `has_part = false`
4. Dann prüfen, ob der neue Kandidat in diesen virtuellen Zustand passt

## Wichtige Regel

Nicht einfach nur „liegt schon ein PICK in der Queue?“ prüfen.

Warum?

Weil das zu grob ist. Beispiel:

```text
PICK
PLACE
PICK
PLACE
```

Hier liegt zwar bereits ein `PICK` in der Queue, aber die Gesamtabfolge ist logisch korrekt.

Darum soll nicht nach Schlagworten gesucht werden, sondern der zukünftige Zustand logisch simuliert werden.

## Gewünschte Command-Semantik

### Für `PICK`
Annahme nur wenn virtueller Zustand sagt:
- `has_part == false`

Danach virtuell:
- `has_part = true`

### Für `PLACE`
Annahme nur wenn virtueller Zustand sagt:
- `has_part == true`

Danach virtuell:
- `has_part = false`

## Vorteil

Damit können die Informatiker direkt Blöcke schicken, ohne dass ein unsinniger Zustand in der Queue landet.

---

# 3. ABORT und RESET sauber definieren

## Problem

Ein Reset im laufenden Betrieb ist gefährlich, wenn Motion, Job-State und Engine-State nicht gemeinsam sauber beendet werden.

## Ziel

Klare Trennung zwischen:

### ABORT
- stoppt aktuelle Aktion
- leert die Queue
- bleibt betriebsbereit
- löst **kein ESTOP** aus

### RESET
- setzt Softwarezustand vollständig zurück
- nur in definiertem Zustand erlaubt
- soll nicht als „harmloser Stop“ missbraucht werden

## Empfehlung

### ABORT einführen
`ABORT` soll der normale Bedien-Stop sein.

Gewünschtes Verhalten:
- aktuelle Motion stoppen
- aktiven Job verwerfen
- Queue leeren
- Zustand sauber auf `IDLE`
- kein ESTOP
- `last_err` definieren, z. B. `ERR_ABORTED` oder `ERR_NONE`

### Wichtige Architektur-Empfehlung
`ABORT` nicht einfach direkt überall hart eingreifen lassen, sondern zentral in `bot_step()` verarbeiten.

Beispielidee:
- `cmd_abort()` setzt `abort_requested = true`
- `bot_step()` erkennt dies
- `bot_step()` räumt zentral auf:
  - `motion_abort()`
  - `job_abort()`
  - Queue clear
  - `busy = false`
  - `state = IDLE`

So bleibt die Engine konsistent.

## RESET
Empfehlung:
- `RESET` nur erlauben, wenn kein aktiver Job läuft
- oder intern zuerst `ABORT` durchführen und danach resetten

Die einfache und sichere Variante ist:
- `RESET` nur in `IDLE`

---

# 4. Protokoll-Semantik klar festlegen

## PICK
`PICK` ist eine Aufnahme in fester Referenzorientierung.

Darum gilt:

```text
PICK x=... y=...
```

- `x` und `y` sind Pflicht
- `phi` ist verboten
- `z` ist verboten

Intern wird `phi = 0` verwendet.

## PLACE
`PLACE` legt an definierter Zielorientierung ab.

```text
PLACE x=... y=... phi=...
```

- `x`, `y`, `phi` sind Pflicht
- `z` ist verboten

## Warum diese Trennung sinnvoll ist

Die Bildverarbeitung bzw. Informatik kann dann sauber rechnen:

- Teil wird in definierter Referenzlage aufgenommen
- gewünschte Endorientierung wird vollständig in `PLACE` kodiert

Es muss kein wachsender Winkeloffset des Greifers mitgeführt werden.

---

# 5. `has_part` bewusst als Open-Loop-Zustand behandeln

## Aktueller Stand

Nach erfolgreichem `PICK` wird softwareseitig angenommen:
- Teil vorhanden

Nach erfolgreichem `PLACE` wird softwareseitig angenommen:
- Teil nicht mehr vorhanden

## Einschätzung

Das ist für einen ersten funktionsfähigen Stand in Ordnung, solange klar ist:

`has_part` ist aktuell ein **angenommener** Zustand, kein verifizierter Messwert.

## Empfehlung

Im Code und in der Doku klar benennen:
- open-loop part tracking
- keine Greif-/Ablageverifikation

## Später optional
Falls später Sensorik dazukommt:
- `ERR_PICK_FAIL`
- `ERR_PLACE_FAIL`
- echte Verifikation nach Z-Hub / Sensor / Endschalter / Encoder / Strommessung

Aktuell ist das nicht zwingend für einen funktionsfähigen Bot.

---

# 6. Konsistenz zwischen MOVE und PICK/PLACE verbessern

## Problem

`MOVE` nutzt die saubere Abschlusslogik mit `job_motion_finish`.

`PICK` und `PLACE` prüfen ihre Bewegungen bisher direkter.

## Empfehlung

Kurzfristig:
- so lassen, wenn es stabil läuft

Mittelfristig:
- XY- und eventuell PHI-Phasen von `PICK`/`PLACE` ebenfalls über dieselbe Abschlusslogik führen wie `MOVE`

Das ist nicht zwingend der nächste Schritt, aber architektonisch sauberer.

---

# 7. Konfiguration für finalen Betrieb definieren

## Problem

Der Code kann logisch gut sein, aber die aktuell eingecheckte Konfiguration muss trotzdem zu einem realen Finalbetrieb passen.

## Vor dem finalen Einsatz klären

- Muss `HOME` vor jeder Bewegung zwingend erfolgt sein?
- Ist Z-Homing final aktiv?
- Welche Profile gelten für X/Y/Z/PHI?
- Welche Safe-Z-Höhe ist global gültig?
- Welche Release-Makros sind final gesetzt?

## Empfehlung

Eine klare Final-Konfiguration definieren und dokumentieren:
- Debug-/Entwicklungsmodus
- Release-/Demo-/Finalmodus

---

# 8. Konkrete nächste Implementationsschritte

## Schritt 1: Queue-konsistente Annahme einbauen
Neue zentrale Prüffunktion erstellen, z. B.:

```c
bool bot_can_accept_action(const bot_action_s *candidate, err_e *out_err);
```

Diese Funktion soll:
- aktuellen Zustand lesen
- aktiven Job logisch berücksichtigen
- Queue logisch durchsimulieren
- neuen Kandidaten prüfen

Danach in `cmd_pick()` und `cmd_place()` nicht mehr direkt nur `g_status.has_part` prüfen, sondern diese Funktion verwenden.

## Schritt 2: ABORT sauber einführen
Benötigt:
- Command in `cmd.c`
- `abort_requested` oder ähnliches in der Engine
- `motion_abort()`
- `job_abort()`
- zentrale Behandlung in `bot_step()`

## Schritt 3: RESET absichern
Entscheidung:
- nur in `IDLE`
- oder `RESET = ABORT + state reset`

Empfehlung:
- zuerst einfach nur in `IDLE` erlauben

## Schritt 4: Protokollkommentare und Help-Texte bereinigen
Alles angleichen auf:
- `PICK x y`
- `PLACE x y phi`

## Schritt 5: Safe-Z und ähnliche Doppeldefinitionen bereinigen
Globale und job-spezifische Konfigwerte vergleichen und vereinheitlichen.

---

# 9. Offene Designfragen, die bewusst entschieden werden müssen

Diese Punkte sollen nicht „zufällig im Code entstehen“, sondern aktiv entschieden werden.

## Queue / Informatik-Schnittstelle
1. Wie gross darf ein Instruktionsblock maximal sein?
2. Darf die Informatik mehrere Blöcke hintereinander senden, bevor der erste komplett fertig ist?
3. Wird bei Queue-Full sofort abgelehnt?
4. Soll es ein `READY`/`CAN_ACCEPT`/`QUEUE_STATUS`-Kommando geben?

## Abort / Reset
5. Was genau ist der Unterschied zwischen `ABORT`, `RESET` und `ESTOP`?
6. Bleibt nach `ABORT` die Position gültig oder muss neu gehomt werden?
7. Setzt `ABORT` `has_part` auf unbekannt, false oder lässt es unverändert?
8. Was passiert, wenn beim Abort der Magnet gerade aktiv ist?

## Teilzustand
9. Soll `has_part` nach `ABORT` bewusst auf „unknown“ gehen?
10. Braucht es langfristig statt `bool has_part` eher:
   - `PART_NONE`
   - `PART_HELD`
   - `PART_UNKNOWN`

## Fehlerverhalten
11. Welche Fehler sind latched?
12. Welche Fehler blockieren weitere Commands?
13. Soll `ERR_ABORTED` eingeführt werden?
14. Braucht es ein `LAST_ERROR`-Kommando?

## Recovery
15. Wann ist erneutes Homing nach Fehlern Pflicht?
16. Wann darf ohne Homing weitergefahren werden?
17. Was ist die Minimal-Recovery nach einem harmlosen Abort?

---

# 10. Empfehlung zur Reihenfolge

## Priorität 1
**Queue-konsistente Command-Annahme**

Das ist der wichtigste letzte Logikpunkt für sinnvolle Block-Kommunikation.

## Priorität 2
**ABORT sauber einführen**

Damit das System bedienbar und robust wird.

## Priorität 3
**RESET-Semantik absichern**

Damit keine inkonsistenten Zustände entstehen.

## Priorität 4
**Kommentare / Help / Doku bereinigen**

Damit Interface und Firmware wirklich dasselbe meinen.

## Priorität 5
**Optionale Verbesserungen**

- Self-Test
- Motion Profiles
- erweitertes Error Handling
- verifiziertes Pick/Place

Diese Punkte sind nützlich, aber nicht nötig, um den Bot jetzt als funktional einsatzfähig zu betrachten.

---

# 11. Fazit

Der Bot ist nahe an einem funktionsfähigen Endstand.  
Die wichtigsten verbleibenden Aufgaben betreffen nicht mehr die Grundbewegung, sondern die **konsistente Systemlogik rund um Queue, Abort und Zustandsverwaltung**.

Wenn diese Punkte sauber gelöst sind, ist der Bot nicht nur „läuft irgendwie“, sondern auch von aussen robust und sinnvoll bedienbar.
