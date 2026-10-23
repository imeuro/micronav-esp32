# Tasks

## 1. Logica di colore (testabile su host)

- [x] 1.1 Aggiungere a `config.h` `COLOR_MICRONAV_ORANGE` (0xBA80), `SPEEDCAM_ALERT_ORANGE_MARGIN_KMH` (10), `SPEEDCAM_ALERT_FLASH_COUNT` (3), `SPEEDCAM_ALERT_FLASH_MS` (150). Verifica: `./build.sh` compila.
- [x] 1.2 Creare header puro `alert_level.h` con `alertLimitKmh(const char* vmax)` e `alertBackgroundLevel(speed, limit)` (NONE/ORANGE/RED). Verifica: test host in `tests/host` coprono gli scenari dello spec (48/58/61 con limite 50, semaforo, "/" , vuoto, velocita' non valida, bordo esatto 50 e 60) e passano.

## 2. UI alert

- [ ] 2.1 In `ui_micronav.c` aggiungere label velocita' in alto sulla schermata alert ("--" se non valida). Verifica: su hardware compare il valore e si aggiorna a 1 Hz.
- [ ] 2.2 Spostare il pallino GPS (idle e alert, `build_gps_widgets`) a sinistra del testo di stato, riallineandolo a ogni cambio testo; bordo bianco sottile. Verifica: su hardware con "In attesa..." e "Fix OK" il pallino resta adiacente al testo su entrambe le schermate e in alto non c'e' piu' nulla sull'idle.
- [ ] 2.3 Implementare il colore di sfondo alert da livello (nero/`COLOR_MICRONAV_ORANGE`/rosso) e bordo del cerchio limite scuro su sfondo colorato. Verifica: su hardware i tre stati sono distinguibili e cerchio e testi leggibili.
- [ ] 2.4 Implementare il flash con `lv_timer` (3x rosso/nero, 150 ms) con colore finale applicato a fine flash; aggiornamenti durante il flash aggiornano solo il colore finale. Verifica: su hardware 3 lampeggi visibili, nessun riavvio sugli aggiornamenti.

## 3. Integrazione

- [ ] 3.1 Passare `gps_position.speed` da `SpeedcamController::notifySpeedcamDetected` a `DisplayController::showSpeedcamAlert` e a `micronav_show_speedcam_alert`; far partire il flash solo sulla transizione a nuovo alert e mai con impostazioni aperte. Verifica: con GPS fake (`GPS_FAKE_MODE`) un percorso a velocita' crescente mostra nero->arancio->rosso e il flash parte una sola volta per alert.
- [x] 3.2 Aggiornare `README.md`/note se descrivono l'alert. Verifica: nessun riferimento a "sfondo sempre nero" resta nei documenti.
- [ ] 3.3 Prova su strada: controllare nel log che il loop non perda fix GPS durante il flash e che arancio/rosso siano leggibili alla luce del giorno. Verifica: log di strada senza perdite fix durante i flash; esito annotato qui prima di archiviare.
