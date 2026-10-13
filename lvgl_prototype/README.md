# Prototipo LVGL per ESP32-C3 (pre-integrazione)

> **Stato**: prototipo di valutazione, non ancora integrato nel firmware principale
> (`micronav_esp32.ino`), che usa la stessa classe `LGFX` (LovyanGFX) condivisa
> da `../src/lgfx_panel_config.h` per il rendering non-LVGL. Vedi
> `NOTE_IMPLEMENTAZIONE.md` nella cartella principale per il contesto della
> migrazione a LVGL.

Dimostra l'uso di LVGL 8.3.3 con ESP32-C3 DevKitM-1, display GC9A01 240x240
onboard e **touchscreen capacitivo CST816S (I2C)**, utilizzando Arduino CLI.
Il flush del display verso LVGL usa **LovyanGFX** come backend SPI (la
stessa classe `LGFX` usata dal firmware principale, non Adafruit_GC9A01A:
quella libreria non inizializzava correttamente questo pannello dopo uno
spegnimento/riaccensione completo — vedi `NOTE_IMPLEMENTAZIONE.md`). La UI
(2 schermate) è generata con **SquareLine Studio** (`ui.c`, `ui_Screen1.*`,
`ui_Screen2.*`).

## Requisiti

- Arduino CLI installato
- Scheda ESP32-C3 DevKitM-1
- Display GC9A01 240x240 (onboard)
- Touchscreen capacitivo CST816S via I2C
- Connessione USB per programmazione e Serial Monitor

## Configurazione Hardware

Il progetto usa i seguenti pin per il display GC9A01 (stessi del progetto principale):

- **CS (Chip Select)**: Pin 10
- **DC (Data/Command)**: Pin 2
- **RST (Reset)**: Pin -1 (non usato)
- **BL (Backlight)**: Pin 3
- **MOSI**: Pin 7
- **SCK**: Pin 6

## Setup Iniziale

### 1. Installa le dipendenze

Esegui lo script di setup per installare tutte le librerie necessarie:

```bash
./setup.sh
```

Questo script:
- Verifica che Arduino CLI sia installato
- Configura ESP32 board manager
- Installa ESP32 core
- Installa LovyanGFX (>=1.2.33)
- Tenta di installare LVGL 8.3.3

### 2. Installazione Manuale LVGL 8.3.3 (se necessario)

Se Arduino CLI non riesce a installare LVGL 8.3.3 automaticamente, puoi installarlo manualmente:

1. **Scarica LVGL 8.3.3:**
   ```bash
   cd ~/Arduino/libraries
   git clone https://github.com/lvgl/lvgl.git
   cd lvgl
   git checkout v8.3.3
   ```

2. **Verifica installazione:**
   ```bash
   arduino-cli lib list | grep lvgl
   ```

## Compilazione e Upload

### Compila il progetto

```bash
./build.sh
```

### Carica il firmware

```bash
./upload.sh
```

Lo script rileva automaticamente la porta seriale USB.

### Monitor Serial

Per vedere i log di debug:

```bash
arduino-cli monitor -p /dev/cu.usbserial-* -c baudrate=115200
```

Sostituisci `/dev/cu.usbserial-*` con la porta corretta del tuo ESP32-C3.

## Struttura del Progetto

```
lvgl_prototype/
├── lvgl_prototype.ino      # Sketch principale (init display/touch/LVGL)
├── ui.c / ui.h             # UI generata da SquareLine Studio
├── ui_Screen1.*            # Schermata 1 (con arc di test)
├── ui_Screen2.*            # Schermata 2
├── ui_helpers.*, ui_comp_hook.c, ui_events.h, ui_img_boot_logo_png.c
├── build.sh                # Script compilazione
├── upload.sh               # Script upload
├── setup.sh                # Script setup librerie
└── README.md               # Questo file
```

## Funzionalità

Il progetto implementa:

1. **Inizializzazione display GC9A01** tramite LovyanGFX (classe `LGFX` condivisa) come backend di flush LVGL
2. **Inizializzazione LVGL 8.3.3** con driver display personalizzato
3. **Buffer doppio** per rendering fluido (240x10 pixel per buffer)
4. **Touch capacitivo CST816S** via I2C, con debug seriale dettagliato (stato I2C, coordinate raw)
5. **UI a due schermate** generata con SquareLine Studio (`ui_init()`), con un widget arc di test per verificare gli eventi touch

## Configurazione LVGL

Il progetto configura LVGL con:

- **Risoluzione**: 240x240 pixel
- **Colori**: RGB565 (16 bit)
- **Buffer**: Doppio buffer circolare (circa 11KB per buffer)
- **Driver**: Personalizzato per GC9A01 tramite LovyanGFX

## Troubleshooting

### Display non si accende

- Verifica che il pin backlight (3) sia configurato correttamente
- Controlla le connessioni SPI
- Verifica che i pin siano corretti in `lvgl_prototype.ino`

### Errore compilazione LVGL

- Verifica che LVGL 8.3.3 sia installato correttamente
- Controlla che tutte le librerie siano installate: `arduino-cli lib list`
- Se necessario, installa manualmente LVGL (vedi sezione sopra)

### Upload fallisce

- Verifica che la porta seriale sia corretta
- Assicurati che nessun altro programma stia usando la porta (Serial Monitor, ecc.)
- Prova a premere il pulsante BOOT durante l'upload
- Verifica che l'ESP32-C3 sia collegato correttamente

### Serial Monitor non mostra output

- Verifica che `CDCOnBoot=cdc` sia impostato nel FQBN
- Controlla il baudrate (115200)
- Assicurati che Serial.begin(115200) sia chiamato nello sketch

## Note Tecniche

- LVGL 8.3.3 richiede configurazione manuale del driver display (non c'è driver nativo GC9A01 in questa versione)
- Il driver personalizzato usa `LovyanGFX` (classe `LGFX` in `../src/lgfx_panel_config.h`) per comunicare con il display
- Il buffer LVGL è configurato per 240x10 pixel (circa 11KB per buffer)
- Il loop principale chiama `lv_timer_handler()` ogni 5ms per aggiornare il display

## Riferimenti

- [LVGL Documentation](https://docs.lvgl.io/8.3/)
- [LovyanGFX](https://github.com/lovyan03/LovyanGFX)
- [ESP32-C3 Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/)

