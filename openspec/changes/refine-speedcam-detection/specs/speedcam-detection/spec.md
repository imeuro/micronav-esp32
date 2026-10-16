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
