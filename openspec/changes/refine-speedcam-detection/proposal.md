## Why

La detection autovelox/speedcam/T-Red e' puramente geometrica: ogni POI entro un cerchio di raggio R (200-2000 m) dalla posizione e' candidato, senza alcun uso della direzione di marcia. Risultato: alert per POI dietro di noi, su strade parallele o in direzione opposta, e alert che non scompaiono dopo il passaggio. L'analisi del codice (vedi `design.md`, sezione "Stato attuale") ha inoltre trovato bug concreti nel tracking di avvicinamento/allontanamento e nel caricamento del database.

## What Changes

- Uso del `course` GPS (con fallback sul bearing calcolato tra gli ultimi fix) per scartare i POI fuori dal cono di marcia.
- Sostituzione della logica ad-hoc `previous_detected_distance` con una macchina a stati per POI (candidato -> in avvicinamento -> superato), cosi' un POI superato non genera piu' alert finche' non esce dall'area.
- La selezione non e' piu' "il piu' vicino in assoluto" ma "il piu' vicino tra quelli davanti a noi".
- Il `course` GPS diventa affidabile: validita' esplicita e soglia di velocita' minima.
- Fix caricamento DB: il limite `MAX_SPEEDCAM_COUNT` e' applicato prima del filtro geografico.
- Logica di geometria isolata in un modulo senza dipendenze Arduino, testabile su host.

## Capabilities

### New Capabilities
- `speedcam-detection`: regole di rilevazione e dismissione degli alert in base a posizione, direzione e stato di passaggio.

### Modified Capabilities
- `display-settings`: nessuna modifica di requisiti; il raggio resta quello di `settings-detection-radius`.

## Impact

`src/speedcam_controller.*`, `src/gps_controller.*` (course valido), `src/utils.*` (bearing, differenza angolare), `src/json_parser.*` (filtro in fase di parse), `src/config.h` (parametri cono/isteresi), `data/fake_gps.json` (scenari di test).

## Out of scope

Direzione di validita' propria del POI (il DB non la contiene, vedi design), map matching, section control tra due punti (Tutor).
