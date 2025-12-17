# MicroNav ESP32-C3 Speedcam Detection

Versione basic per ESP32-C3-MINI-1U con GPS ATGM336H e display GC9A01 240x240 onboard.

## Hardware

- **ESP32-C3-MINI-1U**: MCU principale
- **GPS ATGM336H**: Modulo GPS (compatibile NMEA)
- **Display**: GC9A01 1.28" 240x240 TFT (onboard)

## Funzionalità

- ✅ Rilevazione speedcam basata su posizione GPS
- ✅ Visualizzazione alert su display con stile identico al progetto Raspberry Pi
- ✅ Boot logo all'avvio con fade-in
- ✅ Schermata idle con status GPS
- ✅ Database speedcam locale (JSON su LittleFS)
- ✅ Modalità fake GPS per test senza hardware GPS
- ✅ Script automatizzati per build, upload e monitor
- ✅ Debug seriale completo con output formattato

## Struttura Progetto

```
micronav-esp32/
├── src/                    # Codice sorgente
│   ├── main.cpp           # Entry point principale
│   ├── config.h           # Configurazioni
│   ├── gps_controller.*   # Gestione GPS ATGM336H
│   ├── speedcam_controller.*  # Logica detection speedcam
│   ├── display_controller.*   # Gestione display e rendering
│   ├── json_parser.*      # Parser JSON speedcam
│   └── utils.*            # Utility (calcolo distanza, ecc.)
├── data/                  # File dati (LittleFS)
│   ├── speedcams.json     # Database speedcam
│   ├── fake_gps.json      # Coordinate fake per test GPS
│   └── boot_logo.png      # Logo boot (convertito in boot_logo.h)
├── *.sh                   # Script automatizzati (build, upload, monitor)
├── partitions.csv         # Schema partizioni flash (custom)
└── README.md              # Questo file
```

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
   # Copia speedcams.json dal progetto Raspberry Pi
   cp ../micronav-pi/micronav-assets/speedcams/json/SCDB-Northern-Italy_cleaned.json \
      data/speedcams.json
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
- Upload speed: 921600 baud
- USB CDC On Boot: Enabled (necessario per Serial output)

### Configurazione Pin

Modifica `src/config.h` con i pin corretti per il tuo hardware:

```cpp
// GPS
#define GPS_SERIAL_RX_PIN 4
#define GPS_SERIAL_TX_PIN 5

// Display (onboard - verificare documentazione ESP32-C3-MINI-1U)
#define DISPLAY_CS_PIN 10
#define DISPLAY_DC_PIN 2
#define DISPLAY_RST_PIN -1
#define DISPLAY_BL_PIN 3
#define DISPLAY_MOSI_PIN 7
#define DISPLAY_SCK_PIN 6
```

### Opzione Alternativa: Arduino IDE

Vedi [README_ARDUINO_IDE.md](README_ARDUINO_IDE.md) per istruzioni complete.

**Quick Start Arduino IDE:**
1. Installa ESP32 board support in Arduino IDE
2. Installa librerie: Adafruit GFX, Adafruit GC9A01A, ArduinoJson, TinyGPSPlus
3. Installa ESP32 LittleFS Data Upload tool
4. Apri `micronav_esp32.ino`
5. Configura scheda: ESP32C3 Dev Module
6. Compila e carica

## Librerie Necessarie

Installa le seguenti librerie con Arduino CLI:

```bash
arduino-cli lib install "Adafruit GFX Library"
arduino-cli lib install "Adafruit GC9A01A"
arduino-cli lib install "ArduinoJson"
arduino-cli lib install "TinyGPSPlus"
```

**Librerie richieste:**
- **Adafruit GFX Library** (v1.11.9+): Grafica base per display
- **Adafruit GC9A01A** (v1.0.0+): Driver display GC9A01
- **ArduinoJson** (v7.0.0+): Parsing JSON per database speedcam
- **TinyGPSPlus** (v1.0.3+): Parsing NMEA da GPS

**Nota:** Se usi Arduino IDE, installa le stesse librerie tramite Library Manager.

## Configurazione

Modifica `src/config.h` per personalizzare:

### Speedcam Detection
- **Raggio detection**: `SPEEDCAM_DETECTION_RADIUS` (default: 1000m)
- **Intervallo check**: `SPEEDCAM_CHECK_INTERVAL` (default: 5000ms)
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
- **Durata fade**: `BOOT_LOGO_FADE_DURATION` (default: 500ms)

### Debug
- **Debug abilitato**: `DEBUG_ENABLED` (default: true)
- **Baudrate seriale**: `SERIAL_DEBUG_BAUD` (default: 115200)

## Utilizzo

1. Accendi il dispositivo
2. Attendi boot logo (3 secondi)
3. Il sistema cerca fix GPS
4. Quando GPS ha fix, inizia la rilevazione speedcam
5. Gli alert vengono mostrati automaticamente quando una speedcam è nel raggio

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
- **Database speedcam**: Il JSON cleaned è ~2MB. Considera ottimizzazioni (formato binario, pre-filtraggio geografico) se necessario.
- **Pre-filtraggio geografico**: Abilitato di default per ridurre memoria (bounding box Italia settentrionale)

### Performance
- **CPU**: ESP32-C3 single core @ 160MHz
- **Best practice**: Evitare operazioni blocking nel loop principale
- **Intervalli**: GPS update ogni 1s, speedcam check ogni 5s

### Display
- **Risoluzione**: 240x240 (vs 320x240 originale Raspberry Pi)
- **Layout**: Scalato proporzionalmente
- **Driver**: GC9A01 con Adafruit GFX Library
- **Boot logo**: Compilato nel firmware (boot_logo.h)

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

### Display nero o non funziona
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
- [ ] Caricamento boot logo da LittleFS (invece di compilato)
- [ ] Font personalizzati (conversione TTF)
- [ ] Icone speedcam (semaforo, autovelox, ecc.)
- [ ] Formato binario database (ottimizzazione memoria)
- [ ] Configurazione via seriale/web
- [ ] OTA updates (Over-The-Air)
- [ ] Statistiche utilizzo (km percorsi, speedcam rilevate)

## Licenza

Vedi licenza progetto principale.
