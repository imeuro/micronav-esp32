# Spec Delta

## MODIFIED Requirements

### Requirement: Il contenuto disegnato sul display è sempre visibile
Il sistema SHALL rendere visibile sul pannello fisico ogni elemento grafico disegnato dal firmware (forme, testo, immagini), senza eccezioni legate al tipo di primitiva di disegno usata.

#### Scenario: Indicatore di stato GPS visibile
- **WHEN** il GPS controller aggiorna lo stato della posizione (fix valido o in attesa)
- **THEN** l'indicatore grafico e il testo di stato GPS sono visibili sul pannello e riflettono lo stato corrente

#### Scenario: Alert autovelox visibile
- **WHEN** viene rilevato un autovelox entro il raggio di detection configurato
- **THEN** tipo, distanza, stato, velocita' GPS e l'indicatore circolare (con il limite di velocita' o l'icona semaforo) sono visibili sul pannello entro un ciclo di aggiornamento del display; lo sfondo e' nero senza riquadro colorato, salvo flash e colore di velocita' (vedi `speedcam-alert-feedback`)

### Requirement: Lo stato GPS resta visibile durante l'alert
Il sistema SHALL mostrare indicatore e testo di stato GPS (fix, satelliti) sia nella schermata idle sia nella schermata di alert autovelox. In entrambe le schermate l'indicatore SHALL stare in basso, a sinistra del testo di stato.

#### Scenario: Idle con GPS visibile
- **WHEN** e' mostrata la schermata idle
- **THEN** pallino GPS a sinistra del testo di stato e numero di satelliti sono visibili e riflettono lo stato corrente

#### Scenario: Alert con GPS visibile
- **WHEN** viene mostrato un alert autovelox o semaforo
- **THEN** pallino GPS (verde/rosso) a sinistra del testo di stato, testo di stato e numero di satelliti sono visibili e si aggiornano come nella schermata idle
