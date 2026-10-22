# display-settings Specification

## Purpose
TBD - created by archiving change migrate-display-lvgl. Update Purpose after archive.

## Requirements

### Requirement: Regolazione della luminosita'
Il sistema SHALL permettere di regolare la luminosita' del backlight tra 10% e 100% con uno slider ad arco, applicando il valore in tempo reale.

#### Scenario: Spostamento dello slider
- **WHEN** l'utente trascina l'arco
- **THEN** la luminosita' dello schermo cambia subito e la percentuale mostrata al centro si aggiorna

#### Scenario: Uscita dalle impostazioni
- **WHEN** l'utente tocca "< Indietro"
- **THEN** il display torna alla schermata idle mantenendo la luminosita' scelta

### Requirement: Navigazione tra schermate a swipe
Il sistema SHALL organizzare le schermate (principale, luminosita', raggio di detection) in un anello verticale navigabile con swipe, sia dalla schermata idle sia da quella di alert: swipe verso l'alto avanza (idle -> luminosita' -> raggio -> idle), swipe verso il basso va nel verso opposto (idle -> raggio -> luminosita' -> idle).

#### Scenario: Swipe verso l'alto dalla schermata principale
- **WHEN** l'utente fa uno swipe verso l'alto dalla schermata idle o da un alert
- **THEN** viene mostrata la pagina luminosita', e ogni ulteriore swipe verso l'alto passa a raggio e poi alla schermata principale

#### Scenario: Swipe verso il basso dalla schermata principale
- **WHEN** l'utente fa uno swipe verso il basso dalla schermata idle o da un alert
- **THEN** viene mostrata la pagina raggio, poi luminosita', poi la schermata principale

#### Scenario: Alert durante le impostazioni
- **WHEN** scatta un alert mentre una pagina impostazioni e' aperta
- **THEN** la pagina impostazioni resta visibile e non viene interrotta dall'alert

#### Scenario: Trascinamento dello slider
- **WHEN** l'utente trascina l'arco di luminosita' o raggio
- **THEN** cambia solo il valore e la pagina resta la stessa

### Requirement: Regolazione del raggio di detection
Il sistema SHALL permettere di regolare da touch il raggio di rilevazione degli autovelox tra 200 m e 2000 m, a passi da 50 m, con effetto immediato sul controllo speedcam.

#### Scenario: Modifica del raggio
- **WHEN** l'utente trascina l'arco del raggio
- **THEN** il valore in metri mostrato al centro si aggiorna e le successive verifiche speedcam usano il nuovo raggio

### Requirement: Il touch resta reattivo durante il caricamento del database
Il sistema SHALL mantenere display e touch reattivi mentre il database speedcam viene caricato o ricaricato.

#### Scenario: Tocco subito dopo il boot
- **WHEN** l'utente esegue uno swipe mentre il database e' in caricamento
- **THEN** la navigazione tra le schermate risponde senza attendere la fine del caricamento

### Requirement: Chiusura automatica per inattivita'
Il sistema SHALL tornare alla schermata iniziale quando una pagina impostazioni resta aperta senza alcun tocco per 5 secondi (`SETTINGS_IDLE_TIMEOUT_MS`).

#### Scenario: Nessun tocco per 5 secondi
- **WHEN** una pagina impostazioni e' aperta e non avviene alcun tocco per 5 secondi
- **THEN** il display torna alla schermata idle e le impostazioni modificate vengono salvate

#### Scenario: Tocco prima della scadenza
- **WHEN** l'utente tocca lo schermo o trascina uno slider prima dei 5 secondi
- **THEN** il conto dell'inattivita' riparte da zero
