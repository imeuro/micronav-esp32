## 1. Dati GPS
- [x] 1.1 `GPSPosition.course_valid` + soglia velocita' minima; non conservare course stale
- [x] 1.2 Heading hold e fallback bearing tra fix

## 2. Geometria
- [x] 2.1 `utils`: bearing iniziale, differenza angolare, test cono (modulo senza Arduino)
- [x] 2.2 Parametri in `config.h` (cono, near-field, isteresi, hold, intervallo)

## 3. Detection
- [x] 3.1 State machine CANDIDATE/PASSED in `SpeedcamController`, rimozione `previous_detected_distance`
- [x] 3.2 Selezione "piu' vicino davanti a noi", esclusione `passed_id`
- [x] 3.3 Nascondere l'alert al passaggio (non solo timeout)
- [x] 3.4 Unico throttle a 1 s, rimosso quello in `loop()`
- [x] 3.5 Pre-reject lat/lng prima di Haversine

## 4. Database
- [x] 4.1 Applicare il bbox durante il parse, prima del limite `MAX_SPEEDCAM_COUNT`
- [x] 4.2 Log: quanti POI scartati per cap
- [x] 4.3 Box centrato sulla posizione reale (±0.5°), ricalcolato ogni 30 min; ripiego su Milano senza fix, ricarica al primo fix

- [x] 4.4 Campo `richtung` nel database (`Speedcam.dir`), filtro direzionale nel tracker, `SPEEDCAM_DIR_OFFSET_DEG`; test host e scenario `corsia_opposta`

## 5. Verifica
- [x] 5.1 Test host sulla geometria e sulla state machine
- [x] 5.2 Scenari `fake_gps.json`: frontale, passato, opposto, parallelo, dietro+davanti
- [ ] 5.3 Prova su strada e taratura cono / soglie, verifica convenzione `richtung` (offset 0 o 180)
