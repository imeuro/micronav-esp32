## ADDED Requirements

### Requirement: Le impostazioni sopravvivono al riavvio
Il sistema SHALL salvare luminosita' e raggio di detection in memoria non volatile e ripristinarli all'avvio, usando i valori di default solo se nessun valore e' stato salvato.

#### Scenario: Riavvio dopo una modifica
- **WHEN** l'utente cambia luminosita' o raggio, esce dalle impostazioni e il dispositivo viene riavviato o spento
- **THEN** all'avvio successivo luminosita' e raggio hanno i valori scelti

#### Scenario: Primo avvio
- **WHEN** non esistono impostazioni salvate
- **THEN** si usano luminosita' 80% e il raggio di `config.h`
