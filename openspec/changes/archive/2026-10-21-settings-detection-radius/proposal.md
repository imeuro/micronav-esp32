## Why

La schermata impostazioni regola solo la luminosita', e ad ogni riavvio i valori tornano al default. Il raggio di detection autovelox (fisso a 1000 m in `config.h`) va reso configurabile da touch, e le impostazioni devono sopravvivere al riavvio.

## What Changes

- Seconda pagina impostazioni con slider ad arco per il raggio di detection (200-2000 m, passi da 50 m).
- Navigazione tra schermate a swipe verticale, ad anello (idle, luminosita', raggio); il pallino GPS non e' piu' cliccabile.
- Il touch resta reattivo durante il caricamento del database speedcam (callback di progresso nel parser JSON).
- Luminosita' e raggio salvati in NVS (`Preferences`) alla chiusura delle impostazioni e ripristinati all'avvio.
- Nuovo modulo `src/settings_store.*`; `SpeedcamController::setDetectionRadius()` (gia' esistente) viene collegato alla UI.
- Rimossi dalla pipeline asset i residui del vecchio boot logo/font (`convert_assets.py`, `requirements.txt`).

## Capabilities

### New Capabilities
- `settings-persistence`: salvataggio e ripristino delle impostazioni utente.

### Modified Capabilities
- `display-settings`: aggiunge la regolazione del raggio di detection.

## Impact

`src/ui_micronav.*`, `src/display_controller.*`, nuovo `src/settings_store.*`, `micronav_esp32.ino`, `src/config.h` (limiti raggio).
