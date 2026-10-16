#ifndef SETTINGS_STORE_H
#define SETTINGS_STORE_H

#include <Arduino.h>

// Impostazioni utente persistenti (NVS via Preferences)
struct Settings {
    int brightness_percent;  // 10-100
    int radius_m;            // SETTINGS_RADIUS_MIN_M - SETTINGS_RADIUS_MAX_M
};

// Carica le impostazioni salvate; per i valori mai salvati usa i default
Settings settingsLoad();

// Salva le impostazioni
void settingsSave(const Settings& settings);

#endif // SETTINGS_STORE_H
