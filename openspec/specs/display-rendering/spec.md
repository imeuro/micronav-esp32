# display-rendering Specification

## Purpose
Garantisce che il firmware disegni in modo affidabile e con colori corretti sul pannello fisico GC9A01 240x240, in ogni condizione di accensione.

## Requirements

### Requirement: Inizializzazione del pannello sopravvive a un ciclo di alimentazione completo
Il sistema SHALL inizializzare correttamente il pannello display dopo uno spegnimento e riaccensione completi (rimozione e ripristino dell'alimentazione), non solo dopo un reset software.

#### Scenario: Accensione dopo rimozione completa dell'alimentazione
- **WHEN** il dispositivo viene alimentato dopo essere stato completamente scollegato dall'alimentazione per almeno alcuni secondi
- **THEN** il pannello mostra il boot logo entro il tempo configurato (`BOOT_LOGO_DISPLAY_TIME`)

#### Scenario: Riavvio tramite reset software
- **WHEN** il dispositivo viene riavviato tramite reset software (upload firmware, pulsante reset) senza rimuovere l'alimentazione
- **THEN** il pannello mostra il boot logo correttamente, in modo equivalente a un'accensione da spento

### Requirement: Il contenuto disegnato sul display è sempre visibile
Il sistema SHALL rendere visibile sul pannello fisico ogni elemento grafico disegnato dal firmware (forme, testo, immagini), senza eccezioni legate al tipo di primitiva di disegno usata.

#### Scenario: Indicatore di stato GPS visibile
- **WHEN** il GPS controller aggiorna lo stato della posizione (fix valido o in attesa)
- **THEN** l'indicatore grafico e il testo di stato GPS sono visibili sul pannello e riflettono lo stato corrente

#### Scenario: Alert autovelox visibile
- **WHEN** viene rilevato un autovelox entro il raggio di detection configurato
- **THEN** tipo, distanza, stato e l'indicatore circolare (con il limite di velocità o l'icona semaforo) sono visibili sul pannello entro un ciclo di aggiornamento del display, su sfondo nero senza riquadro colorato

### Requirement: I colori renderizzati corrispondono ai colori richiesti dal firmware
Il sistema SHALL mostrare sul pannello colori che corrispondono a quelli specificati nel codice (RGB565), senza inversioni o scambi di canale introdotti dal livello di accesso al display.

#### Scenario: Colore del boot logo corretto
- **WHEN** il boot logo viene disegnato all'avvio
- **THEN** i colori mostrati corrispondono ai colori sorgente dell'immagine originale (es. gli elementi rossi del logo appaiono rossi, non verdi o di un altro colore)

#### Scenario: Colore dell'alert autovelox corretto
- **WHEN** un alert autovelox viene disegnato
- **THEN** il bordo del cerchio indicatore appare nel colore rosso configurato (`COLOR_MICRONAV_RED`), non in un colore alterato, senza artefatti (buchi/puntini) lungo il bordo

### Requirement: Lo stato GPS resta visibile durante l'alert
Il sistema SHALL mostrare indicatore e testo di stato GPS (fix, satelliti) sia nella schermata idle sia nella schermata di alert autovelox.

#### Scenario: Alert con GPS visibile
- **WHEN** viene mostrato un alert autovelox o semaforo
- **THEN** pallino GPS (verde/rosso), stato e numero di satelliti sono visibili e si aggiornano come nella schermata idle

### Requirement: Il rendering resta reattivo durante il normale funzionamento
Il sistema SHALL mantenere il display aggiornato e il touch reattivo mentre GPS e controllo speedcam sono attivi.

#### Scenario: Avvio senza monitor seriale
- **WHEN** il dispositivo si avvia senza monitor seriale collegato
- **THEN** la schermata idle appare entro pochi secondi dall'accensione, senza blocchi dovuti ai log
