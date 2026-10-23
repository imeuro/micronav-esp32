# Proposal

## Why

Durante la guida lo schermino e' fuori dal campo visivo: un alert autovelox mostrato solo come testo su sfondo nero puo' passare inosservato. Serve un segnale visivo a tutto schermo, percepibile con la visione periferica, che dica anche se si sta rispettando il limite.

## What Changes

- All'inizio di un alert lo sfondo della schermata alert lampeggia in rosso (`COLOR_MICRONAV_RED`): 3 lampeggi da circa 150 ms.
- Dopo il flash lo sfondo dipende dalla velocita' GPS rispetto al limite del POI: nero se entro il limite, "arancio" se oltre di al massimo 10 km/h, rosso oltre. Senza limite confrontabile (semaforo, `vmax` assente o "/") o senza velocita' valida: nero.
- Durante l'alert la velocita' istantanea GPS (km/h) e' mostrata in alto, dove oggi c'e' il pallino GPS.
- Il pallino GPS passa sempre in basso, a sinistra del testo "GPS: ...", sia nella schermata alert sia in quella idle.
- Nuovo `COLOR_MICRONAV_ORANGE` (RGB 189,81,0 = 0xBA80), scelto per mantenere leggibile il testo bianco (contrasto circa 4,8:1 su sfondo arancio).
- Il bordo del cerchio del limite diventa scuro quando lo sfondo e' colorato, per restare visibile.

## Capabilities

### New Capabilities
- `speedcam-alert-feedback`: feedback visivo dell'alert (flash iniziale, colore di sfondo in base a velocita'/limite, velocita' istantanea mostrata).

### Modified Capabilities
- `display-rendering`: l'alert non e' piu' sempre su sfondo nero; il pallino GPS sta sempre in basso accanto al testo di stato (idle e alert).

## Impact

- `src/ui_micronav.c/.h`: sfondo alert, timer del flash, label velocita', posizione pallino GPS (idle e alert).
- `src/display_controller.cpp/.h`: `showSpeedcamAlert` riceve la velocita' GPS; rilevamento della transizione idle->alert per avviare il flash.
- `src/speedcam_controller.cpp`: passa `gps_position.speed` alla notifica.
- `src/config.h`: `COLOR_MICRONAV_ORANGE`, costanti soglia arancio, numero e durata dei lampeggi.
- Prestazioni: ridisegno a schermo pieno per circa 1 s a ogni nuovo alert; poi un cambio colore al secondo al massimo. Nessun impatto sul loop GPS.
