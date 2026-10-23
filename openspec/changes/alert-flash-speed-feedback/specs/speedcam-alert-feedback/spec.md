# Spec Delta

## Purpose
Rendere un alert autovelox percepibile anche senza guardare lo schermo e indicare a colpo d'occhio se la velocita' e' entro il limite del POI.

## ADDED Requirements

### Requirement: Flash di sfondo all'inizio dell'alert
Il sistema SHALL, quando compare un nuovo alert (transizione da nessun alert ad alert), far lampeggiare lo sfondo della schermata alert nel rosso `COLOR_MICRONAV_RED` per 3 lampeggi da circa 150 ms, alternati al nero. Il flash NOT SHALL ripartire sugli aggiornamenti successivi dello stesso alert.

#### Scenario: Nuovo alert
- **WHEN** viene rilevato un autovelox e non c'era alert attivo
- **THEN** lo sfondo lampeggia rosso 3 volte, poi assume il colore determinato dalla velocita'

#### Scenario: Aggiornamento dello stesso alert
- **WHEN** l'alert e' gia' attivo e arriva un nuovo controllo (distanza aggiornata)
- **THEN** il flash non viene rieseguito

#### Scenario: Impostazioni aperte
- **WHEN** compare un alert con le impostazioni aperte
- **THEN** la schermata delle impostazioni non viene interrotta e nessun flash viene mostrato

### Requirement: Colore di sfondo secondo velocita' e limite
Il sistema SHALL, dopo il flash e per tutta la durata dell'alert, colorare lo sfondo in base alla velocita' GPS `v` e al limite `L` del POI: nero se `v <= L`; "arancio" se `L < v <= L + 10 km/h`; rosso (`COLOR_MICRONAV_RED`) se `v > L + 10 km/h`. Il colore SHALL aggiornarsi a ogni nuovo controllo senza ripetere il flash.

#### Scenario: Entro il limite
- **WHEN** il limite e' 50 e la velocita' GPS e' 48 km/h
- **THEN** lo sfondo e' nero

#### Scenario: Sforamento lieve
- **WHEN** il limite e' 50 e la velocita' GPS e' 58 km/h
- **THEN** lo sfondo e' arancio

#### Scenario: Sforamento oltre soglia
- **WHEN** il limite e' 50 e la velocita' GPS e' 61 km/h
- **THEN** lo sfondo e' rosso

#### Scenario: Limite non confrontabile
- **WHEN** il POI e' un semaforo, oppure `vmax` e' assente o "/"
- **THEN** dopo il flash lo sfondo e' nero

#### Scenario: Velocita' non valida
- **WHEN** il GPS non fornisce una velocita' valida
- **THEN** dopo il flash lo sfondo e' nero

#### Scenario: Flash in corso
- **WHEN** la velocita' cambia durante il flash
- **THEN** il flash non viene interrotto e il colore finale riflette la velocita' piu' recente

### Requirement: Velocita' istantanea durante l'alert
Il sistema SHALL mostrare la velocita' GPS istantanea in km/h, arrotondata all'intero, nella parte alta della schermata alert, aggiornandola a ogni controllo. La velocita' NOT SHALL comparire nella schermata idle.

#### Scenario: Velocita' visibile
- **WHEN** un alert e' attivo e la velocita' GPS e' 63.4 km/h
- **THEN** in alto compare "63"

#### Scenario: Velocita' assente
- **WHEN** un alert e' attivo senza velocita' valida
- **THEN** al posto del valore compare "--"

### Requirement: Leggibilita' degli elementi su sfondo colorato
Il sistema SHALL mantenere visibili cerchio del limite, testi e pallino GPS quando lo sfondo e' rosso o arancio.

#### Scenario: Sfondo rosso
- **WHEN** lo sfondo e' rosso
- **THEN** il cerchio del limite resta distinguibile dallo sfondo (bordo scuro) e i testi restano leggibili
