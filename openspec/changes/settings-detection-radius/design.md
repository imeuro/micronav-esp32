## Decisions

- **Salvataggio alla chiusura**, non a ogni evento dello slider (evita usura flash): `DisplayController::update()` rileva la transizione impostazioni aperte -> chiuse e scrive solo se i valori sono cambiati.
- **`settings_store`**: `Settings {brightness, radius_m}` con `load()`/`save()` su `Preferences` namespace "micronav".
- **Raggio applicato via callback**: `DisplayController::setRadiusChangedCallback(void(*)(float))`, registrata in `setup()` verso `SpeedcamController::setDetectionRadius`. Valore iniziale letto dallo store in `setup()`.
- **UI**: `ui_micronav` espone `micronav_set_settings_values(brightness, radius)`, `micronav_set_radius_callback`. Due schermate impostazioni (luminosita', raggio) con indicatore di pagina a due puntini.
- **Navigazione a swipe, ad anello verticale** (decisa dopo prova su hardware: il pallino GPS cliccabile e il titolo cliccabile erano poco intuitivi): `screen_gesture_cb` su ogni schermata, animazione `MOVE_TOP`/`MOVE_BOTTOM` da 180 ms. `gesture_limit` a 40 px.
- **Gesti e arco**: per LVGL l'arco e' toccabile su tutto l'anello (anche nel vuoto in basso). I gesti passano dall'arco, ma vengono ignorati se durante il tocco il valore e' cambiato (`arc_value_moved`, azzerato al rilascio).
- **Touch durante il caricamento DB**: `JSONParser::setProgressCallback` (~30 ms) collegato a `DisplayController::update()` in `micronav_esp32.ino`; il caricamento da ~2 MB bloccava LVGL per qualche secondo dopo il boot.
- **Fuori scope**: altre impostazioni, reset ai default da UI.
