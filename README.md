# PREN Puzzleroboter

Firmware fuer den PREN-Puzzleroboter auf Basis des NXP Kinetis MK22FN512
(TinyK22/MCUXpresso-Projekt). Der `master`-Branch beschreibt den aktuellen
Hauptstand der Robotik-Firmware mit Step/Dir-Ansteuerung, Homing, Pick/Place,
UART-Protokoll, Encoder-Positionsmessung und debugbaren Entwicklungsmodi.

## Funktionsumfang

- Step/Dir-Ansteuerung fuer die Achsen `X`, `Y`, `Z` und `PHI`
- Bewegungsplanung mit symmetrischem Beschleunigungsprofil
- Physikalische Motion-Konfiguration in `mm/s`, `mm/s^2`, `deg/s` und
  `deg/s^2`
- Homing ueber Limit-Switches
- Pick-/Place-Ablauf mit Magnetausgang
- UART-Protokoll fuer externe Steuerung, z. B. Raspberry Pi
- Command Queue fuer asynchrone Jobs
- X/Y-Encoder ueber FTM1/FTM2 Quadrature Decoder
- E-Stop- und Limit-Switch-Polling mit Filterung
- Debug-Tools fuer Kalibration, Positionsdebug und Demo-Zeichnungen
- SEGGER SystemView/RTT-Unterstuetzung im Debug-Build

## Projektstruktur

```text
board/                  Pin-Mux, Clock- und Peripheral-Initialisierung
CMSIS/                  ARM CMSIS Header
device/, drivers/       MCUXpresso SDK / Kinetis-Treiber
linker/                 Linker-Skripte
startup/                Startup-Code fuer MK22
source/app/             Initialisierung, Polling, Bot Engine
source/com/             UART0/UART1 Low-Level-Treiber
source/config/          Zentrale Build-, Geometrie- und Motion-Konfiguration
source/io/              GPIO-Abstraktion fuer Step, Dir, Magnet, Limits, E-Stop
source/job/             HOME, MOVE, PICK, PLACE Job-Logik
source/motion/          Step/Dir Motion Engine und Limit-Switch Handling
source/position/        Encoder-Positionsmessung
source/proto/           UART-Protokoll und Command Parser
source/debug_tools/     Kalibration, Positionsdebug, Demo-Modi
source/utils/           Hilfsfunktionen, Parser, FTM3-Tick
docs/                   Architektur- und Finalisierungshinweise
```

## Build

Voraussetzungen:

- CMake
- Ninja
- ARM GCC Toolchain
- MCUXpresso SDK fuer MK22FN512
- gesetzte Umgebungsvariablen `ARMGCC_DIR`, `SdkRootDirPath` und je nach Setup
  `POSTPROCESS_UTILITY`

Build mit CMake Presets:

```powershell
cmake --preset Debug
cmake --build --preset Debug
```

Das Ziel heisst:

```text
PREN_Puzzleroboter.elf
```

Je nach Setup landet das Artefakt unter `cmake-build/Debug/` oder im
`Debug/`-Ausgabeordner.

## Zentrale Konfiguration

Alle Teilkonfigurationen werden ueber `source/config/robot_config.h`
eingebunden.

Wichtige Dateien:

- `source/config/build_config.h`: Release/Debug, SystemView, Encoder,
  Closed-Loop und Debug-Features
- `source/config/communication_config.h`: UART-Protokoll, Queue-Laenge,
  Debug-Ausgabe und Console-Simulation
- `source/config/geometry_config.h`: Mechanik, Microstepping, Arbeitsraum und
  Step/mm-Ableitung
- `source/config/motion_config.h`: Motion-Timer, Beschleunigungsprofile,
  E-Stop- und Limit-Switch-Filterung
- `source/config/home_config.h`: Homing-Verhalten
- `source/config/pick_config.h` und `source/config/place_config.h`:
  Pick-/Place-Sequenzen
- `source/config/encoder_config.h`: X/Y-Encoder-Skalierung und Invertierung
- `source/config/calibration_config.h`: Kalibrationsparameter

Im aktuellen Master ist `RELEASE` auf `0` gesetzt. Damit sind Debug-Modus,
SystemView und Encoder-Messung aktiv, Closed-Loop-Korrektur fuer MOVE aber noch
deaktiviert.

## Hardware-Pins

Die Pinbelegung wird in `board/pin_mux.c` und `board/pin_mux.h` erzeugt.

| Signal | Pin/Funktion |
| --- | --- |
| STEP_X/Y/Z/PHI | PTD0 / PTD1 / PTD2 / PTD3 |
| DIR_X/Y/Z/PHI | PTC8 / PTC9 / PTC10 / PTC11 |
| ENABLE_PIN | PTA5, aktiv-low Ausgang |
| Magnet | PTA13 |
| E-Stop | PTA12, aktiv-low mit Pull-up |
| Limit X/Y/Z | PTD4 / PTD5 / PTD6, aktiv-low mit Pull-up |
| Encoder X | FTM1 Quadrature auf PTB0/PTB1 |
| Encoder Y | FTM2 Quadrature auf PTB18/PTB19 |
| UART1 Hardware | PTE0 TX / PTE1 RX |

Fuer Entwicklung kann UART1 in `source/com/uart1.c` auf die USB-C/Programmer-
Pins PTC3/PTC4 gelegt werden. Das wird ueber `UART1_USE_HARDWARE_PINS` in
`source/config/communication_config.h` gesteuert.

## Laufzeitablauf

Der Einstiegspunkt ist `source/main.c`.

Beim Start passiert im Wesentlichen:

1. Pins, Clocks und Peripherals initialisieren
2. UART-Protokoll mit `115200` Baud starten
3. FTM3 als periodischen Motion-Tick konfigurieren
4. Encoder-Position initialisieren
5. Motion Engine und Job-System initialisieren
6. Command-Frontend initialisieren
7. Stepper-Enable aktivieren
8. Im Main Loop `poll_all()` und `bot_step()` ausfuehren

## UART-Protokoll

Das externe Protokoll ist zeilenbasiert und laeuft ueber UART1 mit `115200 8N1`.
Commands sind case-insensitive. Antworten enden mit `\n`.

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
CESTOP
UNLATCH
```

Synchrone Befehle antworten direkt mit `OK ...` oder `ERR ...`.
Bewegungs- und Job-Befehle werden in die Queue gelegt und antworten zuerst mit:

```text
QUEUED <CMD> id=<n>
```

Das finale `OK` oder `ERR` kommt spaeter aus der Bot Engine, wenn der Job
abgeschlossen ist.

## Koordinaten und Einheiten

- `x`, `y`, `z` werden im Protokoll in Millimeter angegeben
- `phi` wird in Grad angegeben
- Intern verwendet die Firmware Fixed-Point:
  - `SCALE_MM = 1000` fuer 0.001 mm
  - `SCALE_DEG = 100` fuer 0.01 deg
- Mechanikwerte wie Travel pro Umdrehung und Microstepping werden in
  `geometry_config.h` gepflegt
- Motion-Werte fuer Geschwindigkeit und Beschleunigung stehen in
  `motion_config.h` in physikalischen Einheiten

## Bring-up

1. Hardware gemaess Pinbelegung anschliessen.
2. `UART1_USE_HARDWARE_PINS` passend zur Verbindung setzen.
3. Firmware bauen und flashen.
4. Serielle Verbindung mit `115200 8N1` oeffnen.
5. Nach Start `CMD_READY` erwarten.
6. Mit `PING` pruefen, ob die Kommunikation steht.
7. Mit `STATUS` den aktuellen Zustand ansehen.
8. Zum ersten Test ohne Homing `SET_POS x=0 y=0 z=0 phi=0` setzen.
9. Kleine Einzelbewegungen mit `MOVE ...` testen.
10. Danach Homing, Pick und Place schrittweise testen.

## Debug-Modi

Debug- und Testfeatures werden in `source/config/build_config.h` geschaltet:

- `CALIBRATION_MODE`: automatische Kalibration beim Start
- `DEMO_DRAW_MODE`: Demo-Zeichnung in die Queue laden
- `POSITION_DEBUG`: blockierende Live-Ausgabe der Encoderwerte
- `SYSTEMVIEW`: SEGGER SystemView-Tracing aktivieren

`POSITION_DEBUG` blockiert die normale Anwendung und ist nur fuer
Encoder-Diagnose gedacht.
