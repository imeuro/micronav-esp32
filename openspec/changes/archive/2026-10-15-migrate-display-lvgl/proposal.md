## Why

Il rendering diretto con LovyanGFX in `display_controller` e' lento sul C3 e non supporta il touch. Il prototipo `lvgl_prototype` (layout `ui_micronav`, font Geist Black nativi, schermata luminosita') e' stato validato su hardware: va portato nel firmware reale.

## What Changes

- `DisplayController` viene riscritto su LVGL 8.3 (flush via LovyanGFX `LGFX`), mantenendo la stessa interfaccia pubblica verso `micronav_esp32.ino` e `speedcam_controller`.
- Schermate idle, alert autovelox/semaforo e impostazioni provengono da `ui_micronav` (spostato in `src/`), con font Geist Black LVGL.
- Indicatore e testo GPS restano visibili anche durante l'alert (decisione confermata).
- Touch CST816S: toccando il pallino GPS si apre la schermata luminosita' (arco 10-100%, backlight PWM).
- **BREAKING**: rimossi il rendering Adafruit-style (font `geist_font_*.h`, `geist_mono_font.h`, `boot_logo.h`, `CUSTOM_FONT_*` in `config.h`).
- Loop principale piu' rapido (`MAIN_LOOP_DELAY`) per tenere reattivo `lv_timer_handler()`.

## Capabilities

### New Capabilities
- `display-settings`: schermata impostazioni raggiungibile da touch, regolazione luminosita'.

### Modified Capabilities
- `display-rendering`: l'indicatore GPS resta visibile durante l'alert; il rendering passa a LVGL.

## Impact

`src/display_controller.*`, nuovi `src/ui_micronav.*`, `src/geist_black_*.c`, `src/ui_img_boot_logo_png.c`, `micronav_esp32.ino`, `build.sh` (flag LVGL), `config.h`. Dipendenza nuova: LVGL 8.3.3 (gia' installata).
