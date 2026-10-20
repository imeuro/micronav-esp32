## ADDED Requirements

### Requirement: Filtro direzionale degli alert
Il sistema SHALL mostrare un alert solo per POI (autovelox, T-Red, speedcam) entro il raggio di detection e dentro il cono di marcia definito dall'heading, salvo entro la distanza near-field.

#### Scenario: POI davanti
- **WHEN** un POI e' entro il raggio e l'angolo tra heading e direzione verso il POI e' entro il semiangolo del cono
- **THEN** viene mostrato l'alert

#### Scenario: POI dietro o laterale
- **WHEN** un POI e' entro il raggio ma fuori dal cono di marcia
- **THEN** non viene mostrato alcun alert

#### Scenario: POI con direzione opposta alla marcia
- **WHEN** un POI ha una direzione nel database e l'angolo tra heading e direzione del POI supera la tolleranza
- **THEN** non viene mostrato alcun alert, anche entro la distanza near-field

#### Scenario: Heading non affidabile
- **WHEN** la velocita' e' sotto la soglia minima e l'heading mantenuto e' scaduto
- **THEN** non vengono generati nuovi alert, e i POI gia' in allontanamento restano esclusi

### Requirement: Cono di marcia dipendente dalla distanza
Il sistema SHALL restringere il cono di marcia con la distanza dal POI, in modo che i POI con grande scarto laterale rispetto alla direzione di marcia non generino alert da lontano.

#### Scenario: POI su strada laterale
- **WHEN** un POI e' nel raggio ma il suo scarto angolare supera il semiangolo ammesso per quella distanza
- **THEN** non viene mostrato alcun alert

### Requirement: Compensazione del ritardo GPS
Il sistema SHALL valutare la distanza dai POI sulla posizione proiettata lungo l'heading per l'eta' del fix piu' la latenza stimata del modulo GPS, quando heading e velocita' sono affidabili.

#### Scenario: Passaggio sotto il POI
- **WHEN** l'auto si avvicina a un POI a velocita' sostenuta
- **THEN** la distanza usata per alert e dismissione tiene conto del ritardo del fix, cosi' l'alert non segna ancora decine di metri quando si e' gia' al POI

### Requirement: Distanza mostrata a gradini
Il sistema SHALL mostrare la distanza dal POI a gradini di 100 m oltre i 500 m, di 50 m fino a 100 m e di 10 m sotto.

#### Scenario: Avvicinamento
- **WHEN** la distanza dal POI scende da 800 m a 30 m
- **THEN** il valore mostrato cambia a passi di 100 m, poi 50 m, poi 10 m, non al metro

### Requirement: Dismissione al passaggio
Il sistema SHALL nascondere l'alert di un POI non appena viene superato e SHALL NOT mostrarlo di nuovo finche' non e' uscito dall'area (raggio con isteresi).

#### Scenario: POI superato
- **WHEN** la distanza dal POI cresce oltre il minimo registrato piu' l'isteresi, oppure il POI esce dal semipiano di marcia
- **THEN** l'alert viene nascosto subito e il POI non genera altri alert

### Requirement: Selezione del POI rilevante
Il sistema SHALL scegliere il POI piu' vicino tra quelli dentro il cono, ignorando i POI alle spalle.

#### Scenario: POI dietro e POI davanti
- **WHEN** un POI e' alle spalle piu' vicino e un altro davanti piu' lontano, entrambi nel raggio
- **THEN** l'alert riguarda il POI davanti
