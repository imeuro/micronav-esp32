# MicroNav ESP32-C3 Speedcam Detection

Versione basic per ESP32-C3-MINI-1U con GPS ATGM336H e display GC9A01 240x240 onboard.

## Hardware

- **ESP32-C3-MINI-1U**: MCU principale
- **GPS ATGM336H**: Modulo GPS (compatibile NMEA)
- **Display**: GC9A01 1.28" 240x240 TFT (onboard)
- **Touch**: CST816S capacitivo (onboard, I2C SDA=4 / SCL=5, indirizzo 0x15)

## Funzionalità

- ✅ Rilevazione speedcam basata su posizione GPS, **direzionale**: cono di marcia, POI superati e carreggiata opposta esclusi (vedi `openspec/changes/refine-speedcam-detection`)
- ✅ UI su **LVGL 8.3** (backend di flush LovyanGFX): boot logo con fade-in, schermata idle, alert VELOX / semaforo (T RED) con font Geist Black
- ✅ Indicatore e stato GPS (fix, satelliti) sempre visibili, anche durante l'alert
- ✅ Alert con flash rosso iniziale (3 lampeggi), poi sfondo nero/arancio/rosso in base alla velocità GPS rispetto al limite (arancio fino a +10 km/h) e velocità istantanea in alto; pallino GPS accanto al testo di stato
- ✅ **Touch + impostazioni a swipe**: anello verticale idle → luminosità → raggio di detection → idle; chiusura automatica dopo 5 s di inattività
- ✅ Luminosità (backlight PWM) e raggio di detection (200-2000 m) regolabili da touch e **salvati in NVS**
- ✅ Database speedcam locale (JSON su LittleFS), con direzione del POI (`richtung`) e box geografico centrato sulla posizione, ricalcolato ogni 30 min
- ✅ Modalità fake GPS per test senza hardware (+ scenari in `data/fake_scenarios/`)
- ✅ Log `[RoadTest]` per la taratura su strada (`SPEEDCAM_ROAD_TEST_LOG`)
- ✅ Log eventi su LittleFS (`ROAD_LOG_ENABLED`), scaricabile con `./dump_roadlog.sh` (cancellazione: `./dump_roadlog.sh --clear`; `upload_littlefs.sh` lo cancella)
- ✅ Test su host della geometria e della state machine (`tests/host/`)
- ✅ Script automatizzati per build, upload e monitor; debug seriale completo

## Struttura Progetto

```
micronav_esp32/
├── micronav_esp32.ino      # Entry point (setup/loop)
├── lv_conf.h               # Configurazione LVGL (usata dallo sketch)
├── src/
│   ├── config.h            # Configurazioni
│   ├── gps_controller.*    # GPS ATGM336H / fake, heading (course valido, hold, bearing di ripiego)
│   ├── speedcam_controller.*  # Detection speedcam + caricamento database
│   ├── speedcam_tracker.h  # State machine candidato/superato (senza Arduino, testabile su host)
│   ├── geo_utils.h, heading_tracker.h  # Geometria e heading (senza Arduino)
│   ├── display_controller.*   # Display, LVGL, touch, impostazioni
│   ├── ui_micronav.*       # Schermate LVGL (idle, alert, impostazioni a swipe)
│   ├── geist_black_*.c, ui_img_boot_logo_png.c  # Font e boot logo in formato LVGL
│   ├── lgfx_panel_config.h # Config pannello GC9A01 per LovyanGFX
│   ├── settings_store.*    # Impostazioni persistenti (NVS)
│   ├── json_parser.*       # Parser JSON speedcam (streaming)
│   └── utils.*             # Utility
├── data/                   # File per LittleFS
│   ├── speedcams.json      # Database speedcam (con richtung)
│   ├── fake_gps.json       # Percorso fake per test GPS
│   └── fake_scenarios/     # Scenari fake (frontale, passato, opposto, parallelo, ...)
├── tests/host/             # Test su host della detection
├── openspec/               # Specifiche e change (OpenSpec)
├── lovyangfx_boot_test/    # Sketch di riferimento: test minimo pannello LovyanGFX
├── *.sh                    # Script automatizzati (build, upload, monitor)
├── partitions.csv          # Schema partizioni flash (custom)
└── README.md               # Questo file
```

Il contesto di progetto e le decisioni di rendering sono in `../NOTE_IMPLEMENTAZIONE.md`.

## Setup

### Metodo Consigliato: Arduino CLI (Script Automatizzati)

Questo è il metodo principale consigliato per lo sviluppo. Utilizza script bash che automatizzano build, upload e monitor.

**Prerequisiti:**
1. Installa Arduino CLI: `./setup_arduino_cli.sh` (se non già installato)
2. Installa core ESP32: `arduino-cli core install esp32:esp32`
3. Installa librerie necessarie (vedi sezione Librerie)

**Script Disponibili:**

```bash
# Compila il progetto
./build.sh

# Carica il firmware su ESP32-C3
./upload.sh

# Compila e carica in un unico comando
./build_and_upload.sh

# Carica file su LittleFS (speedcams.json, fake_gps.json)
./upload_littlefs.sh

# Apri monitor seriale (115200 baud)
./monitor.sh
```

**Procedura Completa:**

1. **Prepara i file dati:**
   ```bash
   # Copia speedcams.json dal progetto Raspberry Pi (con il campo richtung)
   python3 convert_assets.py
   # oppure a mano:
   # cp ../../micronav-pi/micronav-assets/speedcams/json/SCDB-Northern-Italy_cleaned.json data/speedcams.json
   ```

2. **Compila e carica firmware:**
   ```bash
   ./build_and_upload.sh
   ```

3. **Carica file su LittleFS:**
   ```bash
   ./upload_littlefs.sh
   ```

4. **Monitor seriale:**
   ```bash
   ./monitor.sh
   ```

**Note Importanti:**
- Gli script rilevano automaticamente la porta seriale USB
- Verificano che la porta non sia occupata prima dell'upload
- Usano configurazione custom con `partitions.csv` (LittleFS a 0x190000)
- Su Mac con Apple Silicon serve **Rosetta** (`ctags` di Arduino e' x86_64): `softwareupdate --install-rosetta`
- `build.sh` passa i flag LVGL (`LV_COLOR_DEPTH=16` ecc.); lo sketch usa il `lv_conf.h` nella sua cartella
- Chiudere `monitor.sh` prima di `upload.sh` (porta occupata)
- Upload speed: 921600 baud
- USB CDC On Boot: Enabled (necessario per Serial output)

### Configurazione Pin

Modifica `src/config.h` con i pin corretti per il tuo hardware:

```cpp
// GPS: connettore SH1.0-4P onboard (GND, 3.3V, TX, RX) = GPIO21 (TX) / GPIO20 (RX).
// Cablaggio: TX del GPS -> RX della board, RX del GPS -> TX della board.
#define GPS_SERIAL_RX_PIN 20
#define GPS_SERIAL_TX_PIN 21

// Touch CST816S (onboard)
#define TOUCH_SDA_PIN 4
#define TOUCH_SCL_PIN 5

// Display (onboard - verificare documentazione ESP32-C3-MINI-1U)
#define DISPLAY_CS_PIN 10
#define DISPLAY_DC_PIN 2
#define DISPLAY_RST_PIN -1
#define DISPLAY_BL_PIN 3
#define DISPLAY_MOSI_PIN 7
#define DISPLAY_SCK_PIN 6
```

### Opzione Alternativa: Arduino IDE

Vedi [ARDUINO_IDE_SETUP.md](../ARDUINO_IDE_SETUP.md) per istruzioni complete.

**Quick Start Arduino IDE:**
1. Installa ESP32 board support in Arduino IDE
2. Installa librerie: LovyanGFX, LVGL 8.3.3, ArduinoJson, TinyGPSPlus
3. Installa ESP32 LittleFS Data Upload tool
4. Apri `micronav_esp32.ino`
5. Configura scheda: ESP32C3 Dev Module
6. Compila e carica

## Librerie Necessarie

Installa le seguenti librerie con Arduino CLI:

```bash
arduino-cli lib install "LovyanGFX@1.2.33"
arduino-cli lib install "lvgl@8.3.3"
arduino-cli lib install "ArduinoJson"
arduino-cli lib install "TinyGPSPlus"
```

**Librerie richieste:**
- **LovyanGFX** (v1.2.33+, la 1.1.5 non compila con il core ESP32 Arduino attuale): Driver display GC9A01
- **LVGL** (v8.3.3): UI, schermate, gesti touch
- **ArduinoJson** (v7.0.0+): Parsing JSON per database speedcam
- **TinyGPSPlus** (v1.0.3+): Parsing NMEA da GPS

**Nota:** Se usi Arduino IDE, installa le stesse librerie tramite Library Manager.

## Configurazione

Modifica `src/config.h` per personalizzare:

### Speedcam Detection
- **Raggio detection**: `SPEEDCAM_DETECTION_RADIUS` (default: 1000m)
- **Raggio detection**: regolabile da touch (200-2000 m, salvato in NVS); `SPEEDCAM_DETECTION_RADIUS` e' il default
- **Intervallo check**: `SPEEDCAM_CHECK_INTERVAL` (default: 1000ms)
- **Cono di marcia e soglie**: `SPEEDCAM_CONE_HALF_ANGLE_DEG`, `SPEEDCAM_NEAR_FIELD_M`, `SPEEDCAM_PASSED_*`, `SPEEDCAM_DIR_*` (vedi `src/config.h` e `openspec/changes/refine-speedcam-detection`)
- **Offset direzione POI**: `SPEEDCAM_DIR_OFFSET_DEG` (0 o 180, da verificare su strada)
- **Log taratura su strada**: `SPEEDCAM_ROAD_TEST_LOG` (1 = attivo)
- **Path database**: `SPEEDCAM_JSON_PATH` (default: "/speedcams.json")

### GPS
- **Baudrate seriale**: `GPS_SERIAL_BAUD` (default: 9600)
- **Timeout fix**: `GPS_FIX_TIMEOUT` (default: 45000ms)
- **Min satelliti**: `GPS_MIN_SATELLITES` (default: 4)
- **Max HDOP**: `GPS_MAX_HDOP` (default: 5.0)

### Modalità Fake GPS (per test senza hardware)
- **Abilita fake mode**: `GPS_FAKE_MODE` (default: true)
- **Path file fake**: `GPS_FAKE_JSON_PATH` (default: "/fake_gps.json")
- **Intervallo aggiornamento**: `GPS_FAKE_UPDATE_INTERVAL` (default: 2000ms)

**Nota:** Con `GPS_FAKE_MODE = true`, il sistema usa coordinate da `fake_gps.json` invece del GPS reale. Utile per test senza hardware GPS o per sviluppo indoor.

### Display
- **Tempo boot logo**: `BOOT_LOGO_DISPLAY_TIME` (default: 3000ms)
- **Fade-in logo**: `BOOT_LOGO_FADE_ENABLED` (default: true)
- **Durata fade**: `BOOT_LOGO_FADE_DURATION` (default: 500ms; il fade agisce sul backlight)
- **Luminosita'**: default `BACKLIGHT_DEFAULT_PERCENT` (80%), regolabile da touch
- **Timeout impostazioni**: `SETTINGS_IDLE_TIMEOUT_MS` (default: 5000ms)

### Debug
- **Debug abilitato**: `DEBUG_ENABLED` (default: true)
- **Baudrate seriale**: `SERIAL_DEBUG_BAUD` (default: 115200)

## Utilizzo

1. Accendi il dispositivo
2. Attendi boot logo (3 secondi)
3. Il sistema cerca fix GPS
4. Quando GPS ha fix, inizia la rilevazione speedcam
5. Gli alert vengono mostrati automaticamente quando una speedcam è nel raggio, nel cono di marcia e non ancora superata

**Impostazioni (touch):** swipe dal basso verso l'alto apre Luminosità, un altro swipe Raggio, un terzo torna alla schermata principale; swipe verso il basso va nel verso opposto. Funziona anche durante un alert. Trascinare l'arco cambia il valore; dopo 5 s senza toccare si torna alla schermata principale e i valori vengono salvati.

## Monitor Console e Debug

### Monitor Seriale

Il sistema produce output di debug dettagliato sulla porta seriale USB (115200 baud).

**Con Arduino CLI (script):**
```bash
./monitor.sh
```

**Con Arduino CLI (comando diretto):**
```bash
arduino-cli monitor -p /dev/cu.usbserial-* -c baudrate=115200
```

**Con screen (macOS/Linux):**
```bash
screen /dev/cu.usbserial-* 115200
```

**Output Tipico:**
```
========================================
MicroNav ESP32-C3 Speedcam Detection
========================================
Baudrate: 115200
Free heap: 123456
========================================

[Setup] Creazione oggetti controller...
[Setup] Display inizializzato con successo!
[GPS] Controller inizializzato
[GPS] RX Pin: 4
[Speedcam] Database caricato: 1234 speedcam
...
```

### Configurazione Debug

Il debug è abilitato di default in `config.h`:

```cpp
#define DEBUG_ENABLED true
#define SERIAL_DEBUG_BAUD 115200
```

**Importante:** Per ESP32-C3, assicurati che **USB CDC On Boot** sia abilitato nelle impostazioni della board. Questo è necessario per vedere l'output Serial su USB.

### Verifica Porta Seriale

**macOS:**
```bash
ls /dev/cu.* | grep -i usb
```

**Linux:**
```bash
ls /dev/ttyUSB* /dev/ttyACM*
```

### Troubleshooting Monitor

**Nessun output:**
- Verifica che `DEBUG_ENABLED = true` in `config.h`
- Controlla che USB CDC On Boot sia abilitato
- Prova a premere il pulsante RESET dell'ESP32-C3
- Verifica che la porta seriale sia corretta

**Porta occupata:**
- Chiudi Arduino IDE Serial Monitor
- Chiudi altri programmi che usano la porta seriale
- Verifica con: `lsof /dev/cu.usbserial-*`

## Note Tecniche

### Memoria e Storage
- **Flash**: 4MB totale
  - App: 0x10000 - 0x190000 (1.5MB)
  - LittleFS: 0x190000 - 0x400000 (2.5MB)
- **RAM**: ~400KB disponibile
- **Database speedcam**: Il JSON è ~2MB (con `richtung`). Il caricamento in streaming dura qualche secondo; durante il caricamento display e touch restano attivi (callback di progresso).
- **Pre-filtraggio geografico**: box di ±0.5° centrato sulla posizione, ricalcolato ogni 30 min (ripiego su Milano senza fix, ricarica al primo fix)

### Performance
- **CPU**: ESP32-C3 single core @ 160MHz
- **Best practice**: Evitare operazioni blocking nel loop principale
- **Intervalli**: GPS update ogni 1s, speedcam check ogni 1s; `MAIN_LOOP_DELAY` 5ms per tenere reattivi LVGL e touch
- **Log seriale**: `Serial.setTxTimeoutMs(0)` evita blocchi senza monitor aperto, ma con buffer pieno alcune righe possono essere scartate

### Display
- **Risoluzione**: 240x240 (vs 320x240 originale Raspberry Pi)
- **UI**: LVGL 8.3 con flush via LovyanGFX (GC9A01, `cfg.invert = true`)
- **Font**: Geist Black convertito per LVGL (`lv_font_conv`), solo i glifi necessari
- **Boot logo**: immagine LVGL compilata nel firmware (`ui_img_boot_logo_png.c`)

### Partizioni Flash
Schema custom definito in `partitions.csv`:
```
nvs:      0x9000  - 0xF000   (24KB)  - NVS storage
phy_init: 0xF000  - 0x10000  (4KB)   - PHY init data
factory:  0x10000 - 0x190000 (1.5MB) - App firmware
littlefs: 0x190000 - 0x400000 (2.5MB) - LittleFS filesystem
```

## Troubleshooting

### Upload Firmware Fallisce
- **Porta seriale occupata**: Chiudi Serial Monitor, Arduino IDE o altri programmi
- **Porta non trovata**: Verifica connessione USB, prova a scollegare/ricollecare
- **Errore durante upload**: Prova a premere il pulsante **BOOT** durante l'upload
- **Errore "Failed to connect"**: Verifica che la porta sia corretta, prova velocità upload più bassa

### GPS non ottiene fix
- Verifica connessioni hardware (RX/TX corretti)
- Controlla che GPS sia all'aperto con vista cielo
- Verifica baudrate (default: 9600) in `config.h`
- Controlla output seriale per messaggi di errore GPS
- **Test con fake GPS**: Abilita `GPS_FAKE_MODE = true` per testare senza hardware

### Database speedcam non caricato
- Verifica che `speedcams.json` sia presente in `data/`
- Esegui `./upload_littlefs.sh` per caricare su LittleFS
- Controlla dimensione file (max ~2.5MB per LittleFS)
- Verifica output seriale per errori di parsing JSON
- Controlla che il file sia valido JSON

### Test su host (detection)
```bash
g++ -std=c++17 -DMICRONAV_HOST_TEST -I src tests/host/test_detection.cpp -o /tmp/test_detection && /tmp/test_detection
```
Su macOS con SDK 27 il link puo' fallire: aggiungere `-isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk`.

### Display nero o non funziona
- Dopo una sequenza anomala di USB/reset puo' servire un power-cycle completo (scollegare il cavo per ~10 s)
- Verifica pin display in `config.h` (corretti per ESP32-C3-MINI-1U)
- Controlla connessioni hardware SPI
- Verifica inizializzazione display nell'output seriale
- Prova a disabilitare display temporaneamente: `#define DISPLAY_ENABLED 0`

### Monitor Seriale non mostra output
- Verifica che `DEBUG_ENABLED = true` in `config.h`
- Controlla che USB CDC On Boot sia abilitato nelle impostazioni board
- Prova a premere RESET sull'ESP32-C3
- Verifica baudrate (115200)
- Prova porta seriale diversa se disponibile

### LittleFS Upload Fallisce
- Verifica che `mklittlefs` sia installato (viene con core ESP32)
- Controlla che `esptool` sia disponibile (installato nel venv o sistema)
- Verifica che la porta seriale non sia occupata
- Controlla dimensione totale file in `data/` (max 2.5MB)

## Sviluppo Futuro

- [x] Script automatizzati per build/upload/monitor
- [x] Modalità fake GPS per test senza hardware
- [x] Pre-filtraggio geografico speedcam
- [x] Boot logo con fade-in
- [x] UI LVGL con touch e font personalizzati (Geist Black)
- [x] Impostazioni da touch (luminosità, raggio) persistenti in NVS
- [x] Detection direzionale (cono di marcia, POI superati)
- [ ] Modulo GPS reale: scelta, collegamento al connettore SH1.0-4P (GPIO20/21) e prova su strada con taratura di cono, soglie e `SPEEDCAM_DIR_OFFSET_DEG`
- [ ] Formato binario database / pre-filtraggio offline (caricamento piu' rapido)
- [ ] Icone speedcam (semaforo, autovelox, ecc.)
- [ ] OTA updates (Over-The-Air)
- [ ] Statistiche utilizzo (km percorsi, speedcam rilevate)

## Licenza

Vedi licenza progetto principale.
