# display-settings Specification

## Purpose
TBD - created by archiving change migrate-display-lvgl. Update Purpose after archive.

## Requirements

### Requirement: Accesso alle impostazioni da touch
Il sistema SHALL aprire la schermata impostazioni quando l'utente tocca il pallino GPS, sia nella schermata idle sia in quella di alert.

#### Scenario: Tocco sul pallino in idle
- **WHEN** l'utente tocca il pallino GPS (anche appena fuori dal bordo visibile) nella schermata idle
- **THEN** viene mostrata la schermata impostazioni

#### Scenario: Tocco sul pallino durante un alert
- **WHEN** l'utente tocca il pallino GPS durante un alert
- **THEN** viene mostrata la schermata impostazioni e il timeout dell'alert non la chiude

### Requirement: Regolazione della luminosita'
Il sistema SHALL permettere di regolare la luminosita' del backlight tra 10% e 100% con uno slider ad arco, applicando il valore in tempo reale.

#### Scenario: Spostamento dello slider
- **WHEN** l'utente trascina l'arco
- **THEN** la luminosita' dello schermo cambia subito e la percentuale mostrata al centro si aggiorna

#### Scenario: Uscita dalle impostazioni
- **WHEN** l'utente tocca "< Indietro"
- **THEN** il display torna alla schermata idle mantenendo la luminosita' scelta
