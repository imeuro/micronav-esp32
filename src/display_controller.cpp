#include "display_controller.h"
#include "speedcam_controller.h"
#include "ui_micronav.h"
#include <Wire.h>

// LVGL usa callback C senza contesto: serve un puntatore all'istanza unica
static DisplayController* s_instance = nullptr;

DisplayController::DisplayController() :
    display(nullptr),
    is_initialized(false),
    touch_present(false),
    showing_alert(false),
    alert_start_time(0),
    alert_display_time(5000),  // 5 secondi default
    gps_has_fix(false),
    gps_satellites(0),
    gps_dirty(true),
    brightness_percent(BACKLIGHT_DEFAULT_PERCENT),
    radius_m(SPEEDCAM_DETECTION_RADIUS),
    settings_dirty(false),
    settings_were_active(false),
    radius_changed_cb(nullptr),
    last_tick_ms(0),
    buf1(nullptr),
    buf2(nullptr) {
}

DisplayController::~DisplayController() {
    delete display;
    delete[] buf1;
    delete[] buf2;
}

bool DisplayController::begin() {
    #ifdef DISPLAY_ENABLED
    if (!DISPLAY_ENABLED) {
        return false;
    }
    #endif

    s_instance = this;

    Settings saved = settingsLoad();
    brightness_percent = saved.brightness_percent;
    radius_m = saved.radius_m;

    // Backlight via PWM, spento finche' la prima schermata non e' disegnata
    ledcAttach(DISPLAY_BL_PIN, BACKLIGHT_PWM_FREQ, BACKLIGHT_PWM_RESOLUTION);
    ledcWrite(DISPLAY_BL_PIN, 0);

    // Pannello via LovyanGFX (config verificata su hardware: src/lgfx_panel_config.h)
    display = new LGFX();
    if (!display->init()) {
        Serial.println("[Display] ERRORE: init() LovyanGFX fallita");
        return false;
    }
    display->fillScreen(0x0000);

    // LVGL
    lv_init();
    const size_t buf_pixels = DISPLAY_WIDTH * 10;
    buf1 = new lv_color_t[buf_pixels];
    buf2 = new lv_color_t[buf_pixels];
    if (!buf1 || !buf2) {
        Serial.println("[Display] ERRORE: memoria insufficiente per i buffer LVGL");
        return false;
    }
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, buf_pixels);
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = DISPLAY_WIDTH;
    disp_drv.ver_res = DISPLAY_HEIGHT;
    disp_drv.flush_cb = flushCallback;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // Touch CST816S (indirizzo fisso, nessuna scansione del bus)
    Wire.begin(TOUCH_SDA_PIN, TOUCH_SCL_PIN);
    Wire.beginTransmission(TOUCH_I2C_ADDR);
    touch_present = (Wire.endTransmission() == 0);
    if (touch_present) {
        lv_indev_drv_init(&indev_drv);
        indev_drv.type = LV_INDEV_TYPE_POINTER;
        indev_drv.read_cb = touchCallback;
        indev_drv.gesture_limit = 40;  // swipe tra schermate (ui_micronav.c)
        lv_indev_drv_register(&indev_drv);
    } else {
        Serial.println("[Display] Touch CST816S non rilevato (UI senza touch)");
    }

    micronav_ui_init();
    micronav_set_brightness_callback(brightnessCallback);
    micronav_set_radius_callback(radiusCallback);
    micronav_set_settings_values(brightness_percent, radius_m);

    last_tick_ms = millis();
    is_initialized = true;
    Serial.println("[Display] LVGL inizializzato");
    return true;
}

void DisplayController::flushCallback(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_p) {
    LGFX* tft = s_instance->display;
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;

    tft->startWrite();
    tft->setAddrWindow(area->x1, area->y1, w, h);
    for (uint32_t y = 0; y < h; y++) {
        tft->writePixels((uint16_t*)&color_p[y * w], w, true);
    }
    tft->endWrite();

    lv_disp_flush_ready(drv);
}

void DisplayController::touchCallback(lv_indev_drv_t* drv, lv_indev_data_t* data) {
    static int16_t last_x = 0;
    static int16_t last_y = 0;
    data->state = LV_INDEV_STATE_RELEASED;
    data->point.x = last_x;
    data->point.y = last_y;

    // Registri CST816S da 0x02: finger_num, XH, XL, YH, YL
    Wire.beginTransmission(TOUCH_I2C_ADDR);
    Wire.write(0x02);
    if (Wire.endTransmission() != 0) return;
    if (Wire.requestFrom((uint8_t)TOUCH_I2C_ADDR, (uint8_t)5) < 5) return;

    uint8_t finger_num = Wire.read();
    uint8_t x_h = Wire.read();
    uint8_t x_l = Wire.read();
    uint8_t y_h = Wire.read();
    uint8_t y_l = Wire.read();
    if (finger_num == 0 || finger_num >= 5) return;

    int16_t x = ((x_h & 0x0F) << 8) | x_l;
    int16_t y = ((y_h & 0x0F) << 8) | y_l;
    if (x >= DISPLAY_WIDTH) x = DISPLAY_WIDTH - 1;
    if (y >= DISPLAY_HEIGHT) y = DISPLAY_HEIGHT - 1;

    last_x = x;
    last_y = y;
    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = x;
    data->point.y = y;
}

void DisplayController::brightnessCallback(int percent) {
    if (s_instance) s_instance->setBrightness(percent);
}

void DisplayController::radiusCallback(int radius_m) {
    if (!s_instance) return;
    s_instance->radius_m = radius_m;
    s_instance->settings_dirty = true;
    if (s_instance->radius_changed_cb) s_instance->radius_changed_cb((float)radius_m);
}

void DisplayController::setRadiusChangedCallback(void (*cb)(float radius_m)) {
    radius_changed_cb = cb;
}

void DisplayController::setBrightness(int percent) {
    if (percent < 10) percent = 10;
    if (percent > 100) percent = 100;
    if (percent != brightness_percent) settings_dirty = true;
    brightness_percent = percent;
    ledcWrite(DISPLAY_BL_PIN, (uint32_t)((percent * 255L) / 100));
}

void DisplayController::serviceLvgl() {
    unsigned long now = millis();
    lv_tick_inc(now - last_tick_ms);
    last_tick_ms = now;
    lv_timer_handler();
}

void DisplayController::showBootLogo(unsigned long display_time_ms) {
    if (!is_initialized) return;

    // Disegna con backlight spento (niente "neve"), poi fade-in del backlight
    micronav_show_idle_screen();
    if (gps_dirty) {
        micronav_update_gps_status(gps_has_fix, gps_satellites);
        gps_dirty = false;
    }
    lv_refr_now(NULL);

    #if BOOT_LOGO_FADE_ENABLED
    const int steps = max(1, BOOT_LOGO_FADE_STEPS);
    for (int i = 1; i <= steps; i++) {
        ledcWrite(DISPLAY_BL_PIN, (uint32_t)((brightness_percent * 255L / 100) * i / steps));
        delay(BOOT_LOGO_FADE_DURATION / steps);
    }
    #endif
    setBrightness(brightness_percent);

    unsigned long start = millis();
    while (millis() - start < display_time_ms) {
        serviceLvgl();
        delay(5);
    }
}

void DisplayController::showIdleScreen() {
    if (!is_initialized) return;
    showing_alert = false;
    if (!micronav_is_settings_active()) {
        micronav_show_idle_screen();
    }
}

void DisplayController::showSpeedcamAlert(const struct Speedcam& speedcam, float distance) {
    if (!is_initialized) return;

    showing_alert = true;
    alert_start_time = millis();

    // Con le impostazioni aperte l'alert non le interrompe
    if (micronav_is_settings_active()) return;

    const bool is_semaforo = (speedcam.type[0] == 'A');
    const char* type_text = is_semaforo ? "T RED" : "VELOX";
    const char* status_text = (speedcam.status == 'A') ? "attivo" : "inattivo";
    const char* vmax = (speedcam.vmax[0] != '\0' && speedcam.vmax[0] != '/') ? speedcam.vmax : "";
    micronav_show_speedcam_alert(type_text, status_text, (int)distance, is_semaforo, vmax);
}

void DisplayController::hideSpeedcamAlert() {
    if (!is_initialized) return;

    showing_alert = false;
    if (!micronav_is_settings_active()) {
        micronav_show_idle_screen();
    }
}

void DisplayController::update() {
    if (!is_initialized) return;

    if (showing_alert && (millis() - alert_start_time) > alert_display_time) {
        hideSpeedcamAlert();
    }
    serviceLvgl();

    // Impostazioni aperte ma nessun tocco da un po': torna alla schermata iniziale
    if (micronav_is_settings_active() &&
        lv_disp_get_inactive_time(NULL) > SETTINGS_IDLE_TIMEOUT_MS) {
        micronav_close_settings();
    }

    // Salva in NVS solo alla chiusura delle impostazioni (non a ogni evento slider)
    bool settings_active = micronav_is_settings_active();
    if (settings_were_active && !settings_active && settings_dirty) {
        settingsSave({brightness_percent, radius_m});
        settings_dirty = false;
    }
    settings_were_active = settings_active;
}

void DisplayController::updateGPSIndicator(bool has_fix, uint8_t satellites) {
    if (!gps_dirty && has_fix == gps_has_fix && satellites == gps_satellites) return;

    gps_has_fix = has_fix;
    gps_satellites = satellites;
    gps_dirty = false;
    if (is_initialized) {
        micronav_update_gps_status(has_fix, satellites);
    }
}
