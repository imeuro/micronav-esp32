## 1. Persistenza
- [x] 1.1 `src/settings_store.h/.cpp` (Preferences)
- [x] 1.2 Limiti raggio in `config.h`

## 2. UI
- [x] 2.1 Pagina raggio in `ui_micronav`
- [x] 2.2 API `micronav_set_settings_values` / `micronav_set_radius_callback`
- [x] 2.3 Navigazione a swipe ad anello verticale con indicatore di pagina (sostituisce tocco su pallino/titolo)
- [x] 2.4 Swipe compatibile con il trascinamento degli archi

## 3. Integrazione
- [x] 3.1 `DisplayController`: carica/salva impostazioni, callback raggio
- [x] 3.2 `micronav_esp32.ino`: applica raggio salvato e registra callback
- [x] 3.3 Callback di progresso nel parser JSON per tenere vivo il touch durante il caricamento DB

## 4. Verifica su hardware
- [x] 4.1 Modifica raggio e luminosita', riavvio, valori mantenuti
- [x] 4.2 Swipe in entrambe le direzioni, da idle e da alert, senza interferenze con gli slider
- [ ] 4.3 Raggio piccolo/grande cambia effettivamente quando scatta l'alert (non ancora verificato)
- [ ] 4.4 Touch reattivo durante il caricamento DB (non ancora verificato esplicitamente)
