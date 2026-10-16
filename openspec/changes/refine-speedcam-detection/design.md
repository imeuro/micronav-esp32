## Stato attuale (verificato sul codice)

Flusso: `loop()` -> `gps_controller->update()` -> ogni 5 s `SpeedcamController::checkSpeedcams()` -> `detectSpeedcam()` scorre tutto l'array e, con Haversine, sceglie il POI **piu' vicino entro `detection_radius`** (cerchio). `DisplayController::showSpeedcamAlert()` mostra l'alert, che si nasconde dopo 5 s (`alert_display_time`).

Conferma dell'ipotesi: **la direzione non e' usata**. `GPSPosition.course` viene letto (`gps_controller.cpp:114`) ma nessuno lo consuma. Il filtro e' solo circolare.

Il "tracking allontanamento" esiste (`previous_detected_distance`, soglia 10 m) ma e' inaffidabile:

1. **`previous_detected_distance` non si aggiorna in avvicinamento** (solo nei rami "nuovo" e "<10 m"). Resta fissato alla distanza della prima rilevazione: dopo il passaggio, `distance_change = d - d_prima` e' negativo finche' non si torna oltre la distanza iniziale, cioe' praticamente al bordo del raggio. L'allontanamento di fatto non scatta mai; l'alert sparisce solo per timeout.
2. **Mascheramento**: viene considerato solo il POI piu' vicino. Se ce n'e' uno dietro (in allontanamento, restituisce `nullptr`) un secondo POI davanti, piu' lontano, non viene mai valutato.
3. **Nessun filtro direzionale**: un POI appena superato o su strada parallela/opposta e' "nuovo" alla prima rilevazione e genera alert.
4. **Doppio throttle da 5 s** (`loop()` e `checkSpeedcams()`): a 130 km/h sono ~180 m tra due check; con raggio 200 m si rischia di non valutare mai il POI. La soglia dei 10 m e' coerente solo a velocita' molto basse.
5. **`course` stale**: se il GPS non ha course valido, `current_position.course` conserva il valore precedente (e in fermo l'RMC da' course rumoroso/0).
6. **Caricamento DB**: `loadFromFile(..., max_count=500)` tronca *prima* di `filterByBoundingBox`. `speedcams.json` (~11k record) e' ordinato per latitudine crescente: in memoria finiscono i primi 500, cioe' solo la fascia piu' a sud, indipendentemente dal bbox.
7. Il DB ripulito non conteneva la direzione del POI, ma l'originale SCDB ha `richtung` (gradi, passi di 5, presente in tutti i record): era stato scartato da `clean_nodes.py`. Ora e' reintrodotto (vedi "Direzione del POI"). Campi del DB ripulito: id, ort, strasse, vmax, art, status, lat, lng, type. `art`: G velocita', A (Ampel) semaforo/T-Red, SC* tutor, BK/EK altri.

## Decisioni

**Heading**
- `course` GPS e' valido solo se `speed >= SPEEDCAM_MIN_SPEED_KMH` (default 10) e il campo e' valido nell'RMC. `GPSPosition` espone `course_valid`.
- Sotto soglia si mantiene l'ultimo heading valido per `HEADING_HOLD_MS` (es. 10 s: semaforo, rallentamento); oltre, heading ignoto.
- Fallback opzionale: bearing tra gli ultimi due fix distanti >= 10 m.
- Heading ignoto -> comportamento di sicurezza: si usa solo il trend di distanza (stato "passato" sotto), mai un alert nuovo per un POI che si sta gia' allontanando.

**Cono di marcia**
- `bearing_to_poi` = bearing iniziale da posizione a POI; `delta = |angdiff(course, bearing)|`.
- Candidato se `delta <= CONE_HALF_ANGLE` (default 60 deg, calcolato in `config.h`). Entro `NEAR_FIELD_M` (default 40 m) il cono non si applica, perche' il rumore GPS rende il bearing inutile.
- Cono piu' stretto a distanza (es. 45 deg oltre 500 m) e' un'estensione valutabile dopo i test; v1 usa un angolo fisso.

**Macchina a stati per POI** (solo per il POI correntemente tracciato, niente array di stati)
- `CANDIDATE`: dentro raggio e dentro cono -> alert, si traccia `min_distance`.
- `PASSED`: `delta > 90 deg` oppure `distance > min_distance + PASSED_HYSTERESIS_M` (default 25 m). Alert nascosto subito, id messo in `passed_id`.
- `passed_id` si libera solo quando il POI esce da `radius * 1.2` (isteresi) -> niente ri-alert sullo stesso POI.
- Sostituisce `previous_detected_distance`/`last_detected_distance`.

**Selezione**: tra tutti i POI dentro raggio, filtrati cono, escluso `passed_id`, si sceglie il piu' vicino. Pre-reject con differenza di lat/lng prima di Haversine (costo CPU, array fino a 500).

**Cadenza**: un solo throttle, in `checkSpeedcams()`, `CHECK_INTERVAL` = 1000 ms (GPS aggiorna a 1 Hz comunque). Rimosso quello in `loop()`.

**Direzione del POI**: `Speedcam.dir` ("richtung", `SPEEDCAM_DIR_NONE` se assente). Un POI con direzione e' candidato solo se `|angdiff(course, dir + SPEEDCAM_DIR_OFFSET_DEG)| <= SPEEDCAM_DIR_TOLERANCE_DEG` (60 deg), anche entro near-field (la carreggiata opposta e' vicina). Con heading ignoto un POI con direzione non genera nuovi alert. **Da verificare su strada**: se `richtung` indichi il traffico controllato o il lato verso cui punta il dispositivo; nel secondo caso `SPEEDCAM_DIR_OFFSET_DEG` va messo a 180. I dati non permettono di stabilirlo. `data/speedcams.json` e' rigenerato dall'originale con `richtung` in piu' (2.1 MB, la partizione LittleFS ne ha 2.4); `clean_nodes.py` in micronav-pi va aggiornato con lo stesso campo.

**Testabilita'**: `geo_utils.h` (bearing, angdiff, cono) e la state machine in classe senza `Arduino.h`, compilabili su host. Scenari su `fake_gps.json`: avvicinamento frontale, passaggio e allontanamento, direzione opposta, strada parallela, POI dietro + POI davanti.

## Rischi

- Course rumoroso a bassa velocita' -> soglia + hold.
- Curve: il POI e' in curva ma fuori cono -> cono largo 60 deg e near-field; valutare in prova su strada.
- Memoria: nessun array aggiuntivo.

## Area del database (aggiunta)

Il bbox statico e' sostituito da un box di ±0.5° centrato sulla posizione reale. Senza fix all'avvio si usa Milano (45.46, 9.19) e al primo fix valido il database viene ricaricato. Poi si ricarica ogni 30 minuti (`SPEEDCAM_DB_REFRESH_MS`), ma non mentre un POI e' tracciato: il parse incrementale blocca il loop per qualche secondo. Il tracker lavora per id, quindi sopravvive alla ricarica. Limite noto: spostandosi oltre ~50 km in meno di 30 minuti il box e' temporaneamente disallineato.
