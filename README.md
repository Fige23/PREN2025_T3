# PREN Puzzleroboter - Branch `feature/uart_speed_tuning`

Dieser Branch erweitert die PREN-Puzzleroboter-Firmware um Runtime-Speed-Tuning
ueber das UART-Protokoll. Ziel: Bewegungsprofile direkt am laufenden Roboter
langsamer oder schneller testen, ohne jedes Mal `motion_config.h`,
`home_config.h`, `pick_config.h` oder `place_config.h` umzubauen und neu zu
flashen.

## Branch-Fokus

- Neuer Runtime-Tuning-Layer in `source/motion/motion_tuning.c`
- Neues UART-Kommando `TUNE`
- Globale Speed-Skalierung fuer alle Bewegungen
- Zusaetzliche Skalierung pro Profilgruppe:
  - `MOVE`
  - `HOME`
  - `PICK`
  - `PLACE`
  - `CORR`
- `STATUS` zeigt die aktuellen Tuning-Werte an
- `TUNE SHOW` zeigt die aktuell wirksamen Profile
- `TUNE EXPORT` gibt passende `#define`-Zeilen aus, um getestete Werte spaeter
  fest in die Config zu uebernehmen

## Was wird skaliert?

Die Config-Dateien auf dem neuen `master` werden in realen Einheiten gepflegt,
z. B. `mm/s`, `mm/s^2`, `deg/s` und `deg/s^2`. Dieser Branch respektiert das:
`TUNE EXPORT MOVE` und `TUNE EXPORT HOME` geben reale Config-Makros aus.

Intern rechnet die Motion Engine weiterhin mit `motion_profile_s`, also mit den
aus der Geometrie abgeleiteten Stepwerten:

```c
start_step_rate_sps
max_step_rate_sps
accel_sps2
```

Das Runtime-Tuning skaliert diese abgeleiteten Werte beim Start einer Bewegung.
Die Config bleibt trotzdem auf realen Einheiten.

Die Strecke, Zielposition, Geometrie, Step/mm-Werte und Limit-Switch-Logik
werden nicht veraendert.

## Gueltiger Wertebereich

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

Beim Start wird alles auf `100` gesetzt. Die Werte sind nur zur Laufzeit im RAM
gespeichert und gehen nach Reset/Power-Cycle verloren.

## TUNE-Kommandos

Alle Kommandos laufen ueber UART1 mit `115200 8N1`. Die Eingabe ist
case-insensitive.

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

`POSCORR` ist als Alias fuer `CORR` erlaubt:

```text
TUNE SET POSCORR 50
```

Einzelne Werte abfragen:

```text
TUNE GET GLOBAL
TUNE GET MOVE
TUNE GET HOME
TUNE GET PICK
TUNE GET PLACE
TUNE GET CORR
```

Wirksame Profile anzeigen:

```text
TUNE SHOW
TUNE SHOW ALL
TUNE SHOW MOVE
TUNE SHOW HOME
TUNE SHOW PICK
TUNE SHOW PLACE
TUNE SHOW CORR
```

Wirksame Profile als Config-Makros ausgeben:

```text
TUNE EXPORT
TUNE EXPORT MOVE
TUNE EXPORT HOME
TUNE EXPORT PICK
TUNE EXPORT PLACE
TUNE EXPORT CORR
```

Hinweis: `MOVE` und `HOME` exportieren reale Einheiten fuer die aktuellen
Master-Configs. `PICK`, `PLACE` und `CORR` exportieren weiterhin Step-Makros,
weil diese Configs aktuell noch als `motion_profile_s` definiert sind.

## Skalierungslogik

Es gibt zwei Ebenen:

1. `GLOBAL` gilt fuer alle Bewegungsgruppen
2. die jeweilige Gruppe gilt nur fuer ihren Profiltyp

Die effektive Skalierung ist:

```text
effective = GLOBAL * GROUP / 100
```

Beispiel:

```text
TUNE SET GLOBAL 80
TUNE SET MOVE 50
```

Dann laufen `MOVE`-Bewegungen effektiv mit:

```text
80 * 50 / 100 = 40 %
```

`HOME`, `PICK`, `PLACE` und `CORR` bleiben bei 80 %, solange ihre jeweilige
Gruppenskalierung auf 100 steht.

## Beispiel-Workflow zum Speed-Tuning

1. Firmware flashen und UART verbinden.
2. Verbindung pruefen:

```text
PING
```

3. Roboterposition initialisieren oder homen:

```text
SET_POS x=0 y=0 z=0 phi=0
```

4. Erst langsam testen:

```text
TUNE SET GLOBAL 40
MOVE x=20 y=0 z=0 phi=0
```

5. Schrittweise schneller werden:

```text
TUNE SET GLOBAL 60
MOVE x=0 y=0 z=0 phi=0
TUNE SET GLOBAL 80
MOVE x=20 y=0 z=0 phi=0
```

6. Wenn nur normale Fahrbewegungen schneller/langsamer werden sollen:

```text
TUNE SET GLOBAL 100
TUNE SET MOVE 75
```

7. Aktuelle Werte ansehen:

```text
STATUS
TUNE SHOW MOVE
```

8. Gute Werte fuer die Config exportieren:

```text
TUNE EXPORT MOVE
```

Die ausgegebenen `#define`-Zeilen koennen anschliessend als feste Werte in die
passenden Config-Dateien uebernommen werden. Fuer `MOVE` sind das z. B.
`X_MAX_SPEED_MM_S` oder `PHI_ACCEL_DEG_S2`, nicht mehr die abgeleiteten
Step/s-Makros.

## Betroffene Dateien

Branch-spezifisch wichtig:

- `source/motion/motion_tuning.h`
- `source/motion/motion_tuning.c`
- `source/motion/motion.h`
- `source/motion/motion.c`
- `source/proto/cmd.c`
- `source/app/init.c`
- `source/job/*.c`

Die Basisprofile kommen weiterhin aus:

- `source/config/motion_config.h`
- `source/config/home_config.h`
- `source/config/pick_config.h`
- `source/config/place_config.h`
- `source/config/encoder_config.h` fuer `CORR`

## Integration im Code

Beim Start ruft `init_all()`:

```c
motion_tuning_init();
```

Damit werden alle Skalierungen auf `100 %` gesetzt.

`motion_start()` bekommt zusaetzlich einen `motion_profile_kind_e`. Dadurch kann
jede Bewegung beim Start passend skaliert werden:

```c
MOTION_PROFILE_KIND_MOVE
MOTION_PROFILE_KIND_HOME
MOTION_PROFILE_KIND_PICK
MOTION_PROFILE_KIND_PLACE
MOTION_PROFILE_KIND_CORR
```

Die Job-Dateien geben diese Gruppe beim Starten ihrer Bewegungen mit.

## Normale UART-Kommandos

Die bestehenden Roboterbefehle bleiben erhalten:

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

Bewegungsbefehle werden weiterhin asynchron in die Queue gelegt und antworten
zuerst mit:

```text
QUEUED <CMD> id=<n>
```

## Build

Voraussetzungen:

- CMake
- Ninja
- ARM GCC Toolchain
- MCUXpresso SDK fuer MK22FN512
- gesetzte Umgebungsvariablen `ARMGCC_DIR`, `SdkRootDirPath` und je nach Setup
  `POSTPROCESS_UTILITY`

Build:

```powershell
cmake --preset Debug
cmake --build --preset Debug
```

Ziel:

```text
PREN_Puzzleroboter.elf
```

## Hinweise und Grenzen

- Tuning-Werte sind nicht persistent.
- `TUNE SET ...` beeinflusst neu gestartete Bewegungen. Bereits laufende
  Bewegungen behalten ihr beim Start berechnetes Profil.
- Zu hohe Werte koennen zu Schrittverlusten oder mechanisch harten Bewegungen
  fuehren. Fuer Bring-up zuerst mit kleinen Prozentwerten testen.
- Dieser Branch enthaelt keine TMC2209-UART-Treiberlogik; er tuned die
  Step/Dir-Motion-Profile der bestehenden Firmware.
