## 1. Preparazione

- [x] 1.1 Copiare UI/font/logo LVGL del prototipo in `src/`
- [x] 1.2 Aggiornare `build.sh` con i flag LVGL (stessi del prototipo)

## 2. Riscrittura DisplayController

- [x] 2.1 Header: sostituire metodi di disegno privati con stato LVGL (buffer, driver, indev)
- [x] 2.2 `begin()`: LGFX init, backlight PWM, lv_init, driver display, touch CST816S
- [x] 2.3 Boot logo con fade, schermata idle, alert, GPS persistente
- [x] 2.4 Impostazioni luminosita' + sospensione timeout alert
- [x] 2.5 `update()` guida `lv_timer_handler`, `MAIN_LOOP_DELAY` ridotto, `Serial.setTxTimeoutMs(0)`

## 3. Pulizia

- [x] 3.1 Rimuovere `geist_font_*.h`, `geist_mono_font.h`, `boot_logo.h`, `CUSTOM_FONT_*`, `convert_font.py`
- [x] 3.2 Aggiornare README / NOTE_IMPLEMENTAZIONE

## 4. Verifica su hardware

- [x] 4.1 Compilazione e upload del firmware principale
- [x] 4.2 Avvio rapido, boot logo, idle con GPS fake
- [x] 4.3 Alert VELOX e T RED con GPS visibile
- [x] 4.4 Tocco pallino -> luminosita' -> indietro, anche durante alert
- [x] 4.5 Power-cycle completo
