# Design

## Context

La schermata alert (`scr_alert` in `ui_micronav.c`) e' ricaricata a ogni check (1 Hz) con `micronav_show_speedcam_alert`; `DisplayController::showing_alert` distingue alert nuovo da aggiornamento. `GPSPosition.speed` e' in km/h ed e' disponibile in `checkSpeedcams`. Il limite del POI e' la stringa `Speedcam.vmax`. Display 240x240 RGB565, LVGL con buffer parziali da 10 righe, refresh 30 ms.

## Goals / Non-Goals

**Goals:**
- Flash e colore di sfondo senza impatto sul loop GPS/NMEA.
- Logica di colore isolata e testabile su host (`tests/host`).

**Non-Goals:**
- Velocita' mostrata fuori dall'alert, suoni/vibrazione, arancio definitivo, soglie configurabili da UI.

## Decisions

- **Flash con `lv_timer` a passi** (150 ms, 6 passi rosso/nero, poi colore finale), non con `lv_anim` sul colore: transizioni nette on/off, nessun frame intermedio da interpolare, stato semplice. Alternativa: `lv_anim` con path lineare, scartata perche' l'interpolazione rosso-nero da' tonalita' scure poco visibili.
- **Trigger del flash in `DisplayController::showSpeedcamAlert`** sulla transizione `showing_alert` false->true; la UI espone `micronav_show_speedcam_alert(..., speed_kmh, limit_kmh)` e un flag/funzione di avvio flash. Se le impostazioni sono aperte il flash non parte (l'alert gia' non carica la schermata).
- **Funzione pura `alertBackgroundLevel(speed, limit)`** in un header senza dipendenze (come `heading_tracker.h`/`geo_utils.h`) che restituisce NONE/ORANGE/RED; la UI mappa il livello in colore. Limite: `atoi(vmax)` se `vmax` e' numerico > 0, altrimenti -1 (nessun confronto). Velocita' non valida: -1.
- **Colori**: rosso = `COLOR_MICRONAV_RED` (gia' replicato in `ui_micronav.c`); arancio = nuovo `COLOR_MICRONAV_ORANGE` 0xBA80 (RGB 189,81,0): arancio scuro scelto perche' il testo bianco ci resta leggibile (contrasto WCAG circa 4,8:1; il rosso del progetto sta a circa 6:1) pur restando distinguibile dal rosso per tonalita'. `COLOR_YELLOW` (oro) darebbe circa 2:1. Soglia `SPEEDCAM_ALERT_ORANGE_MARGIN_KMH` = 10, `SPEEDCAM_ALERT_FLASH_COUNT` = 3, `SPEEDCAM_ALERT_FLASH_MS` = 150 in `config.h`.
- **Velocita'**: label `geist_black_30` centrata in alto (y~22); `--` se non valida. Il pallino GPS (idle e alert, stessa funzione `build_gps_widgets`) e' allineato con `lv_obj_align_to(..., LV_ALIGN_OUT_LEFT_MID)` al testo di stato, riallineato dopo ogni cambio testo (la larghezza varia).
- **Leggibilita'**: bordo del cerchio limite scuro con sfondo colorato, rosso con sfondo nero; pallino GPS con bordo bianco sottile (sull'idle, nero, e' invisibile ma innocuo).

## Risks / Trade-offs

- Il testo di stato "attivo" (grigio chiaro) su arancio ha contrasto circa 2,8:1 → verificare su hardware; se serve, bianco durante lo sfondo colorato.
- Il pallino GPS rosso (senza fix) su sfondo rosso si perde → bordo bianco.
- Ridisegno a schermo pieno ogni 150 ms per ~1 s → blocca il loop per ~20-40 ms a frame; accettabile (buffer UART GPS ~1 B/ms), da confermare dal log di strada.
- Velocita' GPS a 1 Hz con latenza: il colore puo' ritardare di ~1 s sullo sforamento.
