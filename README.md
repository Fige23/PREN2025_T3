# PREN Puzzleroboter - Branch `test/uart-stepper-driver`

Firmware fuer den PREN-Puzzleroboter auf Basis des NXP Kinetis MK22FN512
(TinyK22/MCUXpresso-Projekt). Dieser Branch testet die UART-Konfiguration der
TMC2209-Schrittmotortreiber und bindet sie in die bestehende Step/Dir-Motion
Engine ein.

## Fokus dieses Branches

- Neuer TMC2209-UART-Treiber unter `source/TMC2209/`
- Vier adressierte Treiber fuer `X`, `Y`, `Z` und `PHI`
- Registerzugriff mit CRC, Timeout-Handling und Statuscodes
- Default-Konfiguration fuer Strom, Microstepping, StealthChop, PWM und
  StallGuard-nahe Diagnosewerte
- Motion Engine verwendet die aktuell gesetzten TMC2209-Microsteps zur
  Skalierung der Schrittwerte
- UART0 fuer den TMC2209-Treiberbus, UART1 weiterhin fuer das Bot-Protokoll

## Projektstruktur

```text
board/                  Pin-Mux, Clock- und Peripheral-Initialisierung
device/, drivers/       MCUXpresso SDK / Kinetis-Treiber
source/app/             Systeminitialisierung, Polling, Bot Engine
source/config/          Zentrale Konfigurationen
source/TMC2209/         TMC2209-UART-Treiber
source/motion/          Step/Dir Motion Engine, Limit Switch Handling
source/proto/           UART-Protokoll und Command Parser
source/job/             HOME, MOVE, PICK, PLACE Ablauflogik
source/debug_tools/     Kalibration, Positionsdebug, Demo-Modi
docs/                   Zusaetzliche Implementationsnotizen
```

## Hardware-UARTs

In diesem Branch sind zwei UART-Strecken relevant:

| Zweck | UART | Pins | Bemerkung |
| --- | --- | --- | --- |
| TMC2209-Treiberbus | UART0 | PTA1 RX, PTA2 TX | Single-Wire-Bus fuer PDN_UART |
| Externes Bot-Protokoll | UART1 | PTC3/PTC4 oder PTE0/PTE1 | Umschaltbar mit `UART1_USE_HARDWARE_PINS` |

Die TMC2209-Adressen sind in `source/config/tmc2209_config.h` definiert:

```c
#define TMC2209_ADDR_X    0u
#define TMC2209_ADDR_Y    1u
#define TMC2209_ADDR_Z    2u
#define TMC2209_ADDR_PHI  3u
```

Die Hardware-Adresspins der Treiber muessen dazu passen.

## Wichtige Konfiguration

Die zentrale Include-Datei ist `source/config/robot_config.h`. Sie zieht die
Teilkonfigurationen ein und enthaelt Compile-Time-Checks.

TMC2209:

- `source/config/tmc2209_config.h`
- `TMC2209_ENABLE` aktiviert/deaktiviert den Treiber
- `TMC2209_UART_SINGLE_WIRE` steuert den Single-Wire-Betrieb
- `TMC2209_MICROSTEPS_MOVE` ist aktuell `8`
- `TMC2209_MICROSTEPS_CORRECTION` ist aktuell `64`
- Run-/Hold-Stroeme werden pro Achse in Ampere konfiguriert
- StealthChop ist standardmaessig aktiv

Motion:

- `source/config/motion_config.h`
- `STEP_TICK_HZ` ist aktuell `50000`
- Maximalgeschwindigkeit, Startgeschwindigkeit und Beschleunigung sind in
  physikalischen Einheiten definiert und werden daraus in Steps/s abgeleitet
- E-Stop wird zusaetzlich im Motion-ISR-Pfad gepollt
- Limit-Switch-Filterung ist aktiv

Kommunikation:

- `source/config/communication_config.h`
- `UART1_USE_HARDWARE_PINS = 0` nutzt die USB-C/Programmer-Pins PTC3/PTC4
- `UART1_USE_HARDWARE_PINS = 1` nutzt die externen Hardware-Pins PTE0/PTE1
- Das Bot-Protokoll laeuft mit `serial_init(115200)`

## Build

Voraussetzungen:

- CMake
- Ninja
- ARM GCC Toolchain
- MCUXpresso SDK fuer MK22FN512
- gesetzte Umgebungsvariablen `ARMGCC_DIR`, `SdkRootDirPath` und je nach Setup
  `POSTPROCESS_UTILITY`

Build mit Preset:

```powershell
cmake --preset Debug
cmake --build --preset Debug
```

Das Ziel heisst:

```text
PREN_Puzzleroboter.elf
```

Je nach CMake-/IDE-Setup landet das Artefakt unter `cmake-build/Debug/` oder
im `Debug/`-Ausgabeordner.

## Initialisierung zur Laufzeit

Der Einstiegspunkt ist `source/main.c`.

Beim Start passiert im Wesentlichen:

1. Pins, Clocks und Peripherals initialisieren
2. TMC2209 ueber UART0 initialisieren und Default-Register schreiben
3. UART1 fuer das Bot-Protokoll starten
4. FTM3 als periodischen Motion-Tick konfigurieren
5. Position, Motion Engine, Job-System und Command-Frontend initialisieren
6. Stepper-Enable aktivieren
7. Im Main Loop: `poll_all()` und `bot_step()`

## Bot-Protokoll

Das Protokoll wird zeilenbasiert ueber UART1 gelesen. Commands sind
case-insensitive, Antworten enden mit `\n`.

Nuetzliche Befehle:

```text
PING
STATUS
POS
HOME
SET_POS x=0 y=0 z=0 phi=0
MOVE x=100 y=50 z=0 phi=90
PICK x=10 y=20
PLACE x=100 y=120 phi=45
MAGNET ON
MAGNET OFF
RESET
CLEAR_ESTOP
```

Asynchrone Befehle antworten zuerst mit `QUEUED ... id=...`. Das finale
`OK`/`ERR` kommt spaeter aus der Bot Engine.

## TMC2209-Testhinweise

Zum reinen UART-Sanity-Check eignet sich `tmc2209_read_ifcnt()`:

- Nach erfolgreichen Writes sollte `IFCNT` steigen
- Wenn Writes `OK` liefern, `IFCNT` aber nicht steigt, sind typische Ursachen:
  falsche Adresse, PDN_UART nicht verbunden, Single-Wire-Verschaltung falsch
  oder RX/TX vertauscht

Weitere Diagnosefunktionen:

- `tmc2209_read_gstat()`
- `tmc2209_read_drv_status()`
- `tmc2209_read_sg_result()`
- `tmc2209_clear_gstat()`

## Schneller Bring-up-Ablauf

1. TMC2209-Adresspins fuer X/Y/Z/PHI auf `0/1/2/3` setzen.
2. PDN_UART-Bus mit UART0 verbinden:
   - PTA2 TX ueber Serienwiderstand auf PDN_UART
   - PTA1 RX auf denselben Busknoten
3. Step/Dir/Enable gemaess `board/pin_mux.c` anschliessen.
4. Firmware bauen und flashen.
5. Ueber UART1 mit `115200 8N1` verbinden.
6. `PING` senden und `OK PING` erwarten.
7. Mit `STATUS` den Systemzustand pruefen.
8. Fuer einen kontrollierten Test zuerst `SET_POS x=0 y=0 z=0 phi=0` setzen.
9. Kleine Bewegungen mit `MOVE ...` testen.

## Offene Punkte

- Es gibt noch keinen eigenen UART-Command fuer TMC2209-Diagnosewerte. Fuer
  `IFCNT`, `GSTAT` oder `DRV_STATUS` muss aktuell ein Debug-Hook oder Testcode
  verwendet werden.
- `has_part` ist weiterhin ein softwareseitig angenommener Zustand, keine echte
  Greif-/Ablageverifikation.
- Die Dokumentation in `docs/IMPLEMENTATION_GUIDE_reworked.md` beschreibt
  weitere sinnvolle Finalisierungsschritte fuer Queue-, Abort- und Reset-
  Semantik.
