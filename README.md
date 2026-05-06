# PREN Puzzleroboter

Firmware fuer den PREN-Puzzleroboter auf Basis des NXP Kinetis MK22FN512
(TinyK22/MCUXpresso-Projekt). `master` ist der integrierte Hauptstand mit
Step/Dir-Motion, Bot-Protokoll, Homing, Pick/Place, Encoder-Positionierung,
optionalem TMC2209-UART-Treiber und optionalem UART-Speed-Tuning.

## Funktionsumfang

- Step/Dir-Ansteuerung fuer die Achsen `X`, `Y`, `Z` und `PHI`
- Bewegungsplanung mit symmetrischem Beschleunigungsprofil
- Motion-Konfiguration in physikalischen Einheiten (`mm/s`, `mm/s^2`,
  `deg/s`, `deg/s^2`) mit Ableitung nach Steps/s
- Homing ueber Limit-Switches
- Pick-/Place-Ablauf mit Magnetausgang
- UART1-Protokoll fuer externe Steuerung, z. B. Raspberry Pi
- Command Queue fuer asynchrone Jobs
- `ABORT`, `RESET` und `RESET HARD` fuer sichere Job-/Motion-Ruecksetzung
- X/Y-Encoder ueber FTM1/FTM2 Quadrature Decoder
- E-Stop- und Limit-Switch-Polling mit Filterung
- Optionaler TMC2209-UART-Treiber auf UART0
- Optionales UART-Speed-Tuning ueber `TUNE`

## Projektstruktur

```text
board/                  Pin-Mux, Clock- und Peripheral-Initialisierung
CMSIS/                  ARM CMSIS Header
device/, drivers/       MCUXpresso SDK / Kinetis-Treiber
linker/                 Linker-Skripte
startup/                Startup-Code fuer MK22
source/app/             Initialisierung, Polling, Bot Engine
source/com/             UART1 Low-Level-Treiber
source/config/          Zentrale Build-, Geometrie- und Motion-Konfiguration
source/io/              GPIO-Abstraktion fuer Step, Dir, Magnet, Limits, E-Stop
source/job/             HOME, MOVE, PICK, PLACE und Finish-/Korrektur-Logik
source/motion/          Step/Dir Motion Engine, Limit-Switch Handling, Tuning
source/position/        Encoder-Positionsmessung
source/proto/           UART-Protokoll und Command Parser
source/TMC2209/         TMC2209-UART-Treiber
source/debug_tools/     Kalibration, Positionsdebug, Demo- und TMC-Testmodi
docs/                   Zusaetzliche Implementationsnotizen
```

## Zentrale Konfiguration

Alle Teilkonfigurationen werden ueber `source/config/robot_config.h`
eingebunden und dort mit Compile-Time-Checks validiert.

Wichtige Dateien:

- `source/config/build_config.h`: Release/Debug, SystemView, Encoder,
  Closed-Loop, Speed-Tuning und Debug-Features
- `source/config/communication_config.h`: UART1-Pins, Debug-Ausgabe und
  Console-Simulation
- `source/config/geometry_config.h`: Mechanik, Microstepping, Arbeitsraum und
  Step/mm-Ableitung
- `source/config/motion_config.h`: Motion-Timer, Beschleunigungsprofile,
  E-Stop- und Limit-Switch-Filterung
- `source/config/home_config.h`: Homing-Verhalten und Homing-Profile
- `source/config/pick_config.h` und `source/config/place_config.h`:
  Pick-/Place-Sequenzen
- `source/config/encoder_config.h`: X/Y-Encoder-Skalierung und Korrekturgrenzen
- `source/config/tmc2209_config.h`: TMC2209-UART, Strom, Microsteps und
  Treiber-Defaults

Aktuelle wichtige Defaults:

```c
#define RELEASE                         0
#define POSITION_ENABLE                 1
#define POSITION_CLOSED_LOOP_ENABLE     1
#define MOTION_TUNING_ENABLE            0
#define TMC2209_UART_ENABLE             0
#define TMC2209_MICROSTEPS_MOVE         8u
#define TMC2209_MICROSTEPS_CORRECTION   64u
```

`TMC2209_UART_ENABLE = 0` bedeutet: Die Step/Dir-Motion laeuft normal, aber
UART0 wird nicht fuer die TMC2209 initialisiert und die TMC2209-UART-Funktionen
sind No-Ops. Zum Testen/Betreiben der Treiber-UART-Konfiguration auf `1`
setzen.

`MOTION_TUNING_ENABLE = 0` bedeutet: Die normalen Compile-Time-Profile laufen.
Bei `1` wird das `TUNE`-Command aktiviert und Profile koennen zur Laufzeit
skaliert werden.

## Hardware-UARTs

| Zweck | UART | Pins | Aktivierung |
| --- | --- | --- | --- |
| Externes Bot-Protokoll | UART1 | PTC3/PTC4 oder PTE0/PTE1 | immer, Pins ueber `UART1_USE_HARDWARE_PINS` |
| TMC2209-Treiberbus | UART0 | PTA1 RX, PTA2 TX | nur mit `TMC2209_UART_ENABLE = 1` |

Die TMC2209-Adressen sind in `source/config/tmc2209_config.h` definiert:

```c
#define TMC2209_ADDR_X    0u
#define TMC2209_ADDR_Y    1u
#define TMC2209_ADDR_Z    2u
#define TMC2209_ADDR_PHI  3u
```

Die Hardware-Adresspins der Treiber muessen dazu passen.

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

## Initialisierung

Der Einstiegspunkt ist `source/main.c`.

Beim Start passiert im Wesentlichen:

1. Pins, Clocks und Peripherals initialisieren
2. TMC2209 ueber UART0 initialisieren, falls `TMC2209_UART_ENABLE = 1`
3. UART1 fuer das Bot-Protokoll starten
4. FTM3 als periodischen Motion-Tick konfigurieren
5. Encoder-Position initialisieren
6. Motion Engine, optionales Motion-Tuning und Job-System initialisieren
7. Command-Frontend initialisieren
8. Im Main Loop `poll_all()` und `bot_step()` ausfuehren

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
ABORT
RESET
RESET HARD
CLEAR_ESTOP
CESTOP
UNLATCH
```

Synchrone Befehle antworten direkt mit `OK ...` oder `ERR ...`.
Bewegungs- und Job-Befehle werden in die Queue gelegt und antworten zuerst mit:

```text
QUEUED <CMD> id=<n>
```

Das finale `OK` oder `ERR` kommt spaeter aus der Bot Engine.

Wenn `MOTION_TUNING_ENABLE` auf `1` steht, ist zusaetzlich `TUNE` aktiv:

```text
TUNE GET
TUNE GET MOVE
TUNE SET GLOBAL 80
TUNE SET MOVE 120
TUNE SET HOME 70
TUNE SHOW ALL
TUNE EXPORT MOVE
```

Skalierbare Gruppen sind `GLOBAL`, `MOVE`, `HOME`, `PICK`, `PLACE` und
`CORR`. Erlaubt sind `10` bis `200` Prozent.

## TMC2209-Testhinweise

Zum reinen UART-Sanity-Check `TMC2209_UART_ENABLE = 1` und optional
`TMC2209_UART_TEST_MODE = 1` setzen. Diagnosefunktionen im Treiber sind unter
anderem:

- `tmc2209_read_ifcnt()`
- `tmc2209_read_gstat()`
- `tmc2209_read_drv_status()`
- `tmc2209_read_sg_result()`
- `tmc2209_clear_gstat()`

Nach erfolgreichen Writes sollte `IFCNT` steigen. Wenn Writes `OK` liefern,
`IFCNT` aber nicht steigt, sind typische Ursachen: falsche Adresse, PDN_UART
nicht verbunden, Single-Wire-Verschaltung falsch oder RX/TX vertauscht.

## Schneller Bring-Up

1. Step/Dir/Enable gemaess `board/pin_mux.c` anschliessen.
2. UART1-Verbindung passend zu `UART1_USE_HARDWARE_PINS` anschliessen.
3. Firmware bauen und flashen.
4. Ueber UART1 mit `115200 8N1` verbinden.
5. Nach Start `CMD_READY` erwarten.
6. `PING` senden und `OK PING` erwarten.
7. Mit `STATUS` den Systemzustand pruefen.
8. Fuer einen kontrollierten Test zuerst `SET_POS x=0 y=0 z=0 phi=0` setzen.
9. Kleine Bewegungen mit `MOVE ...` testen.
10. Danach Homing, Pick und Place schrittweise testen.

Bei TMC2209-UART-Betrieb zusaetzlich:

1. TMC2209-Adresspins fuer X/Y/Z/PHI auf `0/1/2/3` setzen.
2. PDN_UART-Bus mit UART0 verbinden:
   - PTA2 TX ueber Serienwiderstand auf PDN_UART
   - PTA1 RX auf denselben Busknoten
3. `TMC2209_UART_ENABLE = 1` setzen.
4. Optional `TMC2209_UART_TEST_MODE = 1` fuer einen Start-Sanity-Check setzen.

## Offene Punkte

- Es gibt noch keinen eigenen UART-Command fuer TMC2209-Diagnosewerte. Fuer
  `IFCNT`, `GSTAT` oder `DRV_STATUS` muss aktuell ein Debug-Hook oder Testcode
  verwendet werden.
- `has_part` ist weiterhin ein softwareseitig angenommener Zustand, keine echte
  Greif-/Ablageverifikation.
