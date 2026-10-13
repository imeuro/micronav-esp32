# Proposal

## Why

Il firmware usa `Adafruit_GC9A01A` per pilotare il display GC9A01 240x240
della board ESP32-2424S012, ma è stato verificato su hardware reale che
questa libreria **non inizializza correttamente il pannello**: dopo uno
spegnimento/riaccensione completo (non un semplice reset software), lo
schermo resta nero, e nessun disegno ad alto livello (testo, forme,
rettangoli — quindi la schermata GPS e gli alert autovelox) è mai
risultato visibile, in nessuna condizione. Solo la scrittura raw di
pixel del boot logo produceva qualcosa, probabilmente per coincidenza.

Un test mirato (`lovyangfx_boot_test/`) con **LovyanGFX**, usando la
stessa identica configurazione pannello del demo ufficiale del produttore
(inclusa `cfg.invert = true`, l'inversione colore che Adafruit_GC9A01A
non applica), ha confermato che il pannello si inizializza correttamente
e sopravvive a uno spegnimento completo.

Questo non è un problema di performance ma di **affidabilità funzionale
di base**: nella configurazione attuale, il dispositivo può restare con
lo schermo nero dopo un ciclo di alimentazione reale (es. in auto, dopo
che la batteria/USB si scollega), con la schermata GPS e gli alert
autovelox mai visibili.

## What Changes

- Sostituire `Adafruit_GFX` + `Adafruit_GC9A01A` con **LovyanGFX** come
  driver di basso livello per il pannello GC9A01 in `display_controller.cpp`
- Riconfigurare il pannello con gli stessi parametri verificati nel test
  (`cfg.invert = true`, offset, pin SPI hardware, nessun pin RST)
- Adattare la lettura del boot logo (`src/boot_logo.h`) al corretto ordine
  dei byte colore richiesto da LovyanGFX (`setSwapBytes`)
- Verificare la compatibilità dei font custom Geist (formato `GFXfont`
  Adafruit) con LovyanGFX, o prevedere una riconversione se necessario
- **BREAKING** (solo a livello di dipendenze di build): rimuove la
  dipendenza da `Adafruit_GC9A01A`/`Adafruit GFX Library` nel firmware
  principale, sostituita da `LovyanGFX`

Fuori scope per questa change:
- La migrazione a LVGL (touch, schermata di configurazione) — resta una
  change successiva, che partirà da questa base già corretta
- L'aggiornamento di `lvgl_prototype/` (che userebbe lo stesso driver
  Adafruit, probabilmente con lo stesso bug) — verrà affrontato quando si
  riprenderà la migrazione LVGL

## Capabilities

### New Capabilities
- `display-rendering`: comportamento del firmware nel disegnare contenuto
  sul pannello fisico GC9A01 (boot logo, schermata idle con stato GPS,
  alert autovelox), inclusa l'affidabilità dell'inizializzazione del
  pannello attraverso cicli di alimentazione completi

### Modified Capabilities

(nessuna — prima capability definita per questo progetto)

## Impact

- `src/display_controller.cpp`, `src/display_controller.h`: riscrittura
  del livello di accesso al display (costruzione, init, tutte le
  primitive di disegno)
- `src/config.h`: eventuali nuove costanti di configurazione pannello
  (es. inversione colore), pin invariati
- `micronav_esp32.ino`: nessuna modifica attesa all'interfaccia pubblica
  di `DisplayController` verso il resto del firmware
- Dipendenze libreria: rimuove `Adafruit GFX Library` + `Adafruit GC9A01A`,
  aggiunge `LovyanGFX` (già verificato: versione 1.2.33, la 1.1.5 non
  compila con il core ESP32 Arduino attuale)
- `ARDUINO_CLI_SETUP.md`, `setup_arduino_cli.sh`: aggiornare l'elenco
  librerie installate
