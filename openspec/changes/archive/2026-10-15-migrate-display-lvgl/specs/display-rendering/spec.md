## ADDED Requirements

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
