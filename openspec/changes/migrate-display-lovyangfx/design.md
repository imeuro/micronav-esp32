# Design

## Context

`DisplayController` (`src/display_controller.h/.cpp`, ~700 righe) incapsula
tutto l'accesso al pannello dietro un'interfaccia pubblica stabile
(`begin()`, `showBootLogo()`, `showIdleScreen()`, `showSpeedcamAlert()`,
`hideSpeedcamAlert()`, `update()`, `updateGPSIndicator()`) usata da
`micronav_esp32.ino` e `speedcam_controller.cpp`. Internamente usa un
puntatore `Adafruit_GC9A01A* display` e le primitive di `Adafruit_GFX`
(`fillRect`, `fillCircle`, `drawCircle`, `print`, `setFont`, ecc.).

Il test `lovyangfx_boot_test/` ha verificato su hardware reale la
configurazione pannello corretta per questa board (vedi proposal.md).
Questa change porta quella configurazione dentro `DisplayController`,
sostituendo il driver sottostante senza cambiare l'interfaccia pubblica.

## Goals / Non-Goals

**Goals:**
- Sostituire `Adafruit_GC9A01A` con `LovyanGFX` in `DisplayController`,
  mantenendo invariata l'interfaccia pubblica verso il resto del firmware
- Riprodurre esattamente lo stesso output visivo attuale (layout, colori,
  font) con il nuovo driver
- Garantire che l'inizializzazione sopravviva a un ciclo di alimentazione
  completo (il bug che questa change risolve)

**Non-Goals:**
- Migrazione a LVGL (change separata successiva)
- Supporto touch (non usato da `DisplayController` oggi)
- Ridisegno del layout grafico (stesso identico aspetto, solo driver diverso)
- Aggiornamento di `lvgl_prototype/` (fuori scope, vedi proposal.md)

## Decisions

### Decisione 1: Classe pannello `LGFX` dedicata, copiata dal test verificato
Si introduce una classe `LGFX : public lgfx::LGFX_Device` (pattern
standard LovyanGFX) con la configurazione esatta già validata in
`lovyangfx_boot_test/lovyangfx_boot_test.ino` (pin, `cfg.invert = true`,
`spi_3wire = true`, frequenze SPI). Nessuna nuova sperimentazione sulla
configurazione pannello: si riusa quella provata.

**Alternative considerate**: scrivere una configurazione pannello da zero
per `display_controller.cpp` — scartata, perché rifare da capo la
sperimentazione già fatta (e già fallita una volta con Adafruit) non ha
senso quando esiste una configurazione verificata.

### Decisione 2: API Adafruit_GFX-compatibile di LovyanGFX dove possibile
LovyanGFX espone primitive con firma molto simile a `Adafruit_GFX`
(`fillRect`, `fillCircle`, `drawCircle`, `print`, `setCursor`,
`setTextColor`, `getTextBounds`/`textWidth`). La maggior parte delle
chiamate in `display_controller.cpp` dovrebbe poter essere portata con
modifiche minime di sintassi, non una riscrittura concettuale.

Le funzioni `drawRoundedRect`/`drawRoundedRectFilled`, oggi stub che
disegnano rettangoli squadrati (`// TODO` nel codice esistente), possono
finalmente usare i rounded-rect nativi di LovyanGFX (`drawRoundRect`/
`fillRoundRect`) — stesso bug di cui si era discusso in precedenza con
Adafruit_GFX (che li avrebbe comunque supportati nativamente), qui risolto
come effetto collaterale della migrazione.

### Decisione 3: Boot logo — lettura pixel con `setSwapBytes(true)`
Confermato nel test: i dati RGB565 in `src/boot_logo.h` (generati per
Adafruit_GFX) necessitano `tft.setSwapBytes(true)` prima di
`pushImage()`/equivalente per mostrare i colori corretti con LovyanGFX.
Il fade-in esistente (righe multiple con fattore di fade applicato
pixel-per-pixel) resta concettualmente identico, sostituendo le chiamate
`display->startWrite()/setAddrWindow()/writePixels()/endWrite()` con gli
equivalenti LovyanGFX (`pushImage`/`pushPixels` con byte swap attivo).

### Decisione 4: Font custom Geist — verifica compatibilità diretta prima di riconvertire
`src/geist_font_big.h`/`geist_font_small.h`/`geist_mono_font.h` definiscono
font in formato `GFXfont` (Adafruit). LovyanGFX definisce un proprio tipo
`lgfx::GFXfont` con struttura intenzionalmente compatibile con il formato
Adafruit GFX (stesso layout di campi). Percorso:
1. Tentare l'uso diretto dei font esistenti passandoli a `setFont()`
   tramite cast al tipo `lgfx::GFXfont` (compatibilità di layout)
2. Se il rendering risulta corrotto/glifi sbagliati, usare `convert_font.py`
   (o l'equivalente strumento di LovyanGFX) per rigenerare i tre font nel
   formato nativo `lgfx::GFXfont`

Questa verifica va fatta presto nell'implementazione (prima di integrare
tutto il resto), perché determina se serve un passaggio di riconversione
font aggiuntivo nel piano di task.

### Decisione 5: Libreria da rimuovere esplicitamente
Rimuovere `Adafruit GC9A01A` e, se non altrimenti necessaria,
`Adafruit GFX Library` da `setup_arduino_cli.sh`/`ARDUINO_CLI_SETUP.md`,
aggiungendo `LovyanGFX` (versione >= 1.2.33 — la 1.1.5 non compila con il
core ESP32 Arduino 3.3.12 attualmente in uso, verificato nel test).

## Risks / Trade-offs

- **[Rischio]** I font Geist potrebbero non essere compatibili 1:1 col
  tipo `lgfx::GFXfont` → **Mitigazione**: Decisione 4, verifica precoce
  con fallback a riconversione
- **[Rischio]** Differenze di clipping/bounds-checking tra le due librerie
  potrebbero cambiare leggermente il posizionamento di testo/forme →
  **Mitigazione**: confronto visivo diretto su hardware reale per ogni
  schermata (idle, alert, boot logo) prima di considerare la change completa
- **[Rischio]** `lvgl_prototype/` resta con lo stesso bug non risolto
  (fuori scope) → **Mitigazione**: documentato esplicitamente in
  `NOTE_IMPLEMENTAZIONE.md` come lavoro futuro noto, non dimenticato

## Migration Plan

1. Implementare la nuova classe `LGFX` e sostituire il tipo del membro
   `display` in `DisplayController`
2. Portare `begin()` e verificare l'init su hardware con power-cycle completo
3. Portare boot logo, idle screen, alert, nello stesso ordine logico
   esistente, verificando visivamente ogni schermata su hardware
4. Rimuovere le dipendenze Adafruit da script/doc di setup
5. Nessun rollback automatico previsto: se la migrazione introducesse
   regressioni visive non risolvibili rapidamente, si torna al commit
   precedente via git (nessuna migrazione dati, nessuno stato persistito
   da preservare)
