# PREN Puzzleroboter - UART Driver + Speed Tuning

Dieser Stand kombiniert zwei Feature-Zweige:

- `test/uart-stepper-driver`: TMC2209-Treiberkonfiguration ueber UART0
- `feature/uart_speed_tuning`: Runtime-Speed-Tuning ueber das UART-Protokoll

Ziel ist eine Firmware, bei der die TMC2209-Schrittmotortreiber beim Start
konfiguriert werden und Bewegungsprofile danach zur Laufzeit ueber UART
langsamer oder schneller getestet werden koennen.

## Branch-Fokus

- TMC2209-UART-Treiber unter `source/TMC2209/`
- Vier adressierte Treiber fuer `X`, `Y`, `Z` und `PHI`
- Run-/Hold-Strom in Ampere RMS ueber `source/config/tmc2209_config.h`
- Microstep-Umschaltung fuer normale Motion und Korrekturfahrten
- Runtime-Speed-Tuning mit dem Kommando `TUNE`
- Globale Skalierung und Skalierung pro Bewegungsgruppe:
  - `MOVE`
  - `HOME`
  - `PICK`
  - `PLACE`
  - `CORR`
- `TUNE EXPORT MOVE` und `TUNE EXPORT HOME` geben reale Master-Config-Makros
  aus, z. B. `X_MAX_SPEED_MM_S` oder `HOME_XY_FAST_MAX_MM_S`

## Hardware-UARTs

| Zweck | UART | Pins | Bemerkung |
| --- | --- | --- | --- |
| TMC2209-Treiberbus | UART0 | PTA1 RX, PTA2 TX | Single-Wire-Bus fuer PDN_UART |
| Bot-Protokoll | UART1 | PTC3/PTC4 oder PTE0/PTE1 | Umschaltbar mit `UART1_USE_HARDWARE_PINS` |

Die TMC2209-Adressen sind in `source/config/tmc2209_config.h` definiert:

```c
#define TMC2209_ADDR_X    0u
#define TMC2209_ADDR_Y    1u
#define TMC2209_ADDR_Z    2u
#define TMC2209_ADDR_PHI  3u
```

## Wichtige Dateien

```text
source/TMC2209/                 TMC2209-UART-Treiber
source/config/tmc2209_config.h  Treiberadressen, Strom, Microsteps, Defaults
source/motion/motion_tuning.*   Runtime-Speed-Tuning
source/motion/motion.c          Motion Engine mit Tuning und TMC-Microsteps
source/proto/cmd.c              UART-Kommandos inkl. TUNE
source/job/                     HOME, MOVE, PICK, PLACE und Korrekturfahrten
board/peripherals.c             UART0-Initialisierung fuer TMC2209
board/pin_mux.c                 UART0/UART1/GPIO Pin-Muxing
```

## Startablauf

Beim Start passiert in `init_all()`:

1. Pins, Clocks und Peripherals initialisieren
2. TMC2209-Treiber ueber UART0 initialisieren
3. Bot-Protokoll ueber UART1 mit `115200` Baud starten
4. FTM3-Motion-Tick konfigurieren
5. Encoder-Position initialisieren
6. Motion Engine initialisieren
7. Speed-Tuning auf `100 %` initialisieren
8. Job-System und Command-Frontend initialisieren

## TMC2209-Strom und Microsteps

Die Stromwerte werden direkt in Ampere RMS eingetragen:

```c
#define TMC2209_HOLDCURR_X_A  0.10f
#define TMC2209_RUNCURR_X_A   0.42f
```

Die Umrechnung in TMC2209-Registerwerte macht `tmc2209.c` anhand von
`TMC2209_RSENSE_OHM` und `TMC2209_VSENSE_LOW_CURRENT`.

Normale Bewegungen verwenden `TMC2209_MICROSTEPS_MOVE`. Fuer geschlossene
Korrekturfahrten kann temporaer auf `TMC2209_MICROSTEPS_CORRECTION` gewechselt
werden; danach wird wieder auf die normalen Motion-Microsteps zurueckgestellt.

## Speed-Tuning

Tuning-Werte sind Prozentwerte:

```text
10 ... 200
```

Beispiele:

| Wert | Wirkung |
| --- | --- |
| `50` | halbe Geschwindigkeit und halbe Beschleunigung |
| `100` | Normalwert aus den Config-Dateien |
| `150` | 1.5-fache Geschwindigkeit und Beschleunigung |
| `200` | doppelte Geschwindigkeit und Beschleunigung |

Die effektive Skalierung ist:

```text
effective = GLOBAL * GROUP / 100
```

## TUNE-Kommandos

Aktuelle Skalierung anzeigen:

```text
TUNE
TUNE GET
TUNE GET ALL
```

Globale Skalierung setzen:

```text
TUNE SET GLOBAL 50
TUNE SET MASTER 100
```

Profilgruppe setzen:

```text
TUNE SET MOVE 80
TUNE SET HOME 60
TUNE SET PICK 70
TUNE SET PLACE 70
TUNE SET CORR 50
```

Wirksame Profile anzeigen:

```text
TUNE SHOW
TUNE SHOW MOVE
TUNE SHOW HOME
TUNE SHOW PICK
TUNE SHOW PLACE
TUNE SHOW CORR
```

Wirksame Profile als Config-Makros exportieren:

```text
TUNE EXPORT
TUNE EXPORT MOVE
TUNE EXPORT HOME
TUNE EXPORT PICK
TUNE EXPORT PLACE
TUNE EXPORT CORR
```

Hinweis: `MOVE` und `HOME` exportieren reale Einheiten passend zum aktuellen
Master. `PICK`, `PLACE` und `CORR` exportieren weiterhin Step-Makros, weil diese
Profile aktuell noch als `motion_profile_s` definiert sind.

## Normale UART-Kommandos

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

Bewegungsbefehle werden asynchron in die Queue gelegt und antworten zuerst mit:

```text
QUEUED <CMD> id=<n>
```

## Bring-up

1. TMC2209-Adresspins fuer X/Y/Z/PHI auf `0/1/2/3` setzen.
2. PDN_UART-Bus mit UART0 verbinden:
   - PTA2 TX ueber Serienwiderstand auf PDN_UART
   - PTA1 RX auf denselben Busknoten
3. UART1 fuer das Bot-Protokoll verbinden.
4. Firmware bauen und flashen.
5. `PING` senden und `OK PING` erwarten.
6. Mit `STATUS` Systemzustand und Tuning-Werte pruefen.
7. Fuer einen kontrollierten ersten Test `SET_POS x=0 y=0 z=0 phi=0` setzen.
8. Mit kleinem Speed starten:

```text
TUNE SET GLOBAL 40
MOVE x=20 y=0 z=0 phi=0
```

9. Schrittweise erhoehen und gute Werte mit `TUNE EXPORT ...` sichern.

## Build

```powershell
cmake --preset Debug
cmake --build --preset Debug
```

Ziel:

```text
PREN_Puzzleroboter.elf
```

## Hinweise

- Tuning-Werte sind nur zur Laufzeit im RAM gespeichert.
- `TUNE SET ...` beeinflusst neu gestartete Bewegungen, nicht bereits laufende.
- Zu hohe Tuning-Werte koennen Schrittverluste verursachen.
- Wenn `IFCNT` nach TMC2209-Writes nicht steigt, zuerst Adresse,
  Single-Wire-Verkabelung und UART0-Pins pruefen.
