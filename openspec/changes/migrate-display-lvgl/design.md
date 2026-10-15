## Context

Prototipo validato: `lvgl_prototype/` (LVGL 8.3.3, flush via `LGFX`, touch CST816S su I2C SDA=4 SCL=5, indirizzo 0x15, backlight PWM su pin 3). `lv_conf.h` e' globale in `~/Documents/Arduino/libraries`.

## Decisions

- **Interfaccia pubblica invariata**: `begin/showBootLogo/showIdleScreen/showSpeedcamAlert/hideSpeedcamAlert/update/updateGPSIndicator`. `speedcam_controller` non cambia.
- **UI in `src/`** (Arduino compila `src/` ricorsivamente): `ui_micronav.c/h`, `geist_black_26.c`, `geist_black_30.c`, `ui_img_boot_logo_png.c`. Il prototipo continua a usare le proprie copie.
- **LVGL guidato da `update()`**: `lv_tick_inc` + `lv_timer_handler()` in `update()`, chiamato a ogni `loop()`; `MAIN_LOOP_DELAY` scende a 5 ms.
- **Touch dentro `DisplayController`** (`readTouch` come callback indev), senza log di debug per tocco.
- **Boot logo**: l'immagine LVGL con fade-in via opacita' dell'immagine; la schermata idle riusa lo stesso oggetto.
- **Aggiornamenti GPS solo su cambio**: `updateGPSIndicator` aggiorna LVGL solo se fix/satelliti cambiano.
- **Timeout alert**: sospeso mentre le impostazioni sono aperte.
- **Seriale non bloccante**: `Serial.setTxTimeoutMs(0)` dopo `Serial.begin`.
- **Fuori scope**: raggio di detection nelle impostazioni (solo luminosita'); persistenza luminosita' (NVS).

## Risks

- Heap: buffer LVGL doppi 2x4.8KB + UI; il C3 ha ~320KB, margine ampio.
- Flash: ~690KB su partizione factory da 1.5MB: ok.
- Regressione GPS/LittleFS: il database speedcam usa LittleFS, non toccato.
