#include "settings_store.h"
#include "config.h"
#include <Preferences.h>

static const char* NAMESPACE = "micronav";

Settings settingsLoad() {
    Preferences prefs;
    Settings s;
    s.brightness_percent = BACKLIGHT_DEFAULT_PERCENT;
    s.radius_m = SPEEDCAM_DETECTION_RADIUS;
    if (prefs.begin(NAMESPACE, true)) {
        s.brightness_percent = prefs.getInt("brightness", s.brightness_percent);
        s.radius_m = prefs.getInt("radius", s.radius_m);
        prefs.end();
    }
    s.brightness_percent = constrain(s.brightness_percent, 10, 100);
    s.radius_m = constrain(s.radius_m, SETTINGS_RADIUS_MIN_M, SETTINGS_RADIUS_MAX_M);
    return s;
}

void settingsSave(const Settings& settings) {
    Preferences prefs;
    if (prefs.begin(NAMESPACE, false)) {
        prefs.putInt("brightness", settings.brightness_percent);
        prefs.putInt("radius", settings.radius_m);
        prefs.end();
    }
}
