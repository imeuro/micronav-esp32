# Tasks

## 1. Setup libreria e classe pannello

- [x] 1.1 Aggiungere LovyanGFX (>=1.2.33) a `setup_arduino_cli.sh` — verify: `arduino-cli lib list` mostra LovyanGFX >=1.2.33 installata
- [x] 1.2 Creare la classe `LGFX` in `display_controller.cpp` (o header dedicato), con la configurazione pannello esatta verificata in `lovyangfx_boot_test/` (pin, `cfg.invert = true`, `spi_3wire = true`) — verify: il progetto compila con `./build.sh`

## 2. Sostituzione del driver in DisplayController

- [x] 2.1 Sostituire il membro `Adafruit_GC9A01A* display` con un'istanza `LGFX`, aggiornare `DisplayController::begin()` di conseguenza — verify: `./build.sh` compila, log seriale mostra "Display inizializzato con successo"
- [ ] 2.2 Verificare su hardware reale l'inizializzazione dopo un ciclo di alimentazione completo (scollega/ricollega USB, non solo reset) — verify: il boot logo compare dopo lo spegnimento/riaccensione completo, non solo dopo un soft reset

## 3. Boot logo

- [ ] 3.1 Portare `showBootLogo()` (incluso il fade-in) alle API LovyanGFX, con `setSwapBytes(true)` per la lettura di `src/boot_logo.h` — verify: colori del logo corretti su hardware (confronto visivo con l'immagine sorgente)
- [ ] 3.2 Verificare che la tempistica del fade-in resti conforme a `BOOT_LOGO_FADE_STEPS`/`BOOT_LOGO_FADE_DURATION` — verify: tempo di rendering loggato comparabile al comportamento precedente (~650-700ms osservati in precedenza)

## 4. Font custom Geist

- [ ] 4.1 Verificare se i font esistenti (`geist_font_big.h`, `geist_font_small.h`, `geist_mono_font.h`, tipo Adafruit `GFXfont`) funzionano direttamente con `lgfx::GFXfont` (vedi design.md, Decisione 4) — verify: testo leggibile e correttamente posizionato su hardware usando `setFont()` coi font esistenti
- [ ] 4.2 (Condizionale, solo se 4.1 fallisce) Riconvertire i tre font nel formato nativo LovyanGFX — verify: stesso criterio del task 4.1, con i font rigenerati

## 5. Schermata idle (stato GPS)

- [ ] 5.1 Portare `drawGPSIndicator()` e `drawGPSInfo()` alle API LovyanGFX — verify: su hardware, con GPS in modalità fake, l'indicatore e il testo di stato GPS sono visibili e si aggiornano (corrisponde allo scenario "Indicatore di stato GPS visibile" in specs/display-rendering/spec.md)

## 6. Alert autovelox

- [ ] 6.1 Portare `drawSpeedcamAlertContent()`, sostituendo gli stub `drawRoundedRect`/`drawRoundedRectFilled` con le rounded-rect native di LovyanGFX (`drawRoundRect`/`fillRoundRect`) — verify: su hardware, con GPS fake che passa vicino a uno speedcam del database di test, l'alert appare con forma arrotondata e colore rosso corretto (corrisponde agli scenari "Alert autovelox visibile" e "Colore dell'alert autovelox corretto")
- [ ] 6.2 Portare `drawCircleWithBorder()` (indicatore circolare nell'alert) — verify: il cerchio con bordo è visibile correttamente nell'alert su hardware

## 7. Pulizia dipendenze e documentazione

- [x] 7.1 Rimuovere `Adafruit GC9A01A` e `Adafruit GFX Library` da `setup_arduino_cli.sh` e `ARDUINO_CLI_SETUP.md` — verify: documentazione e script coerenti, nessun riferimento residuo alle librerie Adafruit per il display
- [ ] 7.2 Aggiornare `NOTE_IMPLEMENTAZIONE.md` per riflettere la migrazione completata (rimuovere lo stato "bug noto", descrivere lo stato attuale basato su LovyanGFX) — verify: documento aggiornato, nessun riferimento residuo a "Adafruit_GC9A01A in uso nel firmware principale"

## 8. Integration Verification

- [ ] 8.1 Su hardware reale: ciclo completo spegnimento/riaccensione → boot logo → schermata idle con GPS fake attivo → almeno un ciclo di rilevazione/alert autovelox visibile, confrontando il comportamento osservato con tutti gli scenari di `specs/display-rendering/spec.md` — verify: ogni scenario dello spec delta è stato osservato e confermato manualmente su hardware
