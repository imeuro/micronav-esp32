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

## 5. Esito prova su strada (16/10) e interventi
- [x] 5.4 Log eventi su LittleFS (`road_log.*`, `./dump_roadlog.sh`): alert (con geometria), avanzamento, fine alert e motivo, perdita GPS, ricarica DB
- [x] 5.5 Cono che si restringe con la distanza (`SPEEDCAM_CONE_LATERAL_M`, `SPEEDCAM_CONE_MIN_DEG`) contro gli alert su strade laterali
- [x] 5.6 Compensazione del ritardo del fix (eta' del fix + `SPEEDCAM_GPS_LATENCY_MS`, proiezione lungo l'heading) contro il ritardo sul T-Red
- [x] 5.7 Distanza mostrata a gradini 100/50/10 m

- [x] 5.8 Isteresi sul cono per il POI tracciato (`SPEEDCAM_CONE_HYSTERESIS_DEG` 20): dal log del 22/10, 9 nascondimenti "fuori-cono" su 15 alert, quasi tutti flapping dello stesso POI
- [x] 5.9 `SPEEDCAM_DIR_TOLERANCE_DEG` 60 -> 75: POI 4431 passato con scarto di direzione 58-59 gradi; `richtung` con offset 0 confermato dal log (offset 180 sempre > 120 gradi)
- [x] 5.10 Dump seriale del log con timeout TX di attesa (con timeout 0 arrivavano solo ~4 KB)

- [x] 5.11 POI tracciato stabile: resta finche' valido salvo un altro piu' vicino di `SPEEDCAM_SWITCH_MARGIN_M` (30 m); la sostituzione e' loggata come `H motivo=sostituito`. Dal log del 22/10 sera: 8996 e 26772 (7 m di distanza) si scambiavano ~10 volte. Test host `test_tracked_poi_is_sticky`
- [x] 5.12 Niente "superato" (dietro/allontanamento) sotto `SPEEDCAM_MIN_SPEED_KMH`: la deriva GPS da fermi (POI 8989, v=2, 171->195 m) lo marcava passato e il passaggio vero restava senza alert. Test host `test_no_receding_when_slow`
- [x] 5.13 Tetto allo scarto laterale per ammettere un POI nuovo (`SPEEDCAM_MAX_LATERAL_M` 170, distanza x seno dell'angolo): il minimo del cono (15 gradi) ammetteva ~230 m laterali a 890 m (POI 4611 su strada parallela: 198-285 m, alert per 80 s). Alert reali del log: scarto <= 139 m. Il POI tracciato resta governato da cono + isteresi. Test host `test_lateral_cap`

## 5b. Verifica
- [x] 5.1 Test host sulla geometria e sulla state machine
- [x] 5.2 Scenari `fake_gps.json`: frontale, passato, opposto, parallelo, dietro+davanti
- [ ] 5.3 Prova su strada e taratura cono / soglie, verifica convenzione `richtung` (offset 0 o 180)
- [x] 5.4 Log `[RoadTest]` (`SPEEDCAM_ROAD_TEST_LOG`): per i 3 POI piu' vicini distanza, bearing, angolo cono, scarto direzione con offset 0 e 180, verdetto; database caricato su LittleFS con `richtung`
