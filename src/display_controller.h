#ifndef DISPLAY_CONTROLLER_H
#define DISPLAY_CONTROLLER_H

#include <Arduino.h>
#include <lvgl.h>
#include "config.h"
#include "lgfx_panel_config.h"
#include "settings_store.h"

// Forward declaration
struct Speedcam;

/**
 * Controller display GC9A01 240x240 + touch CST816S, basato su LVGL.
 * Schermate (idle, alert, impostazioni) definite in ui_micronav.c;
 * LVGL fa il flush tramite LovyanGFX (classe LGFX condivisa).
 */
class DisplayController {
public:
    DisplayController();
    ~DisplayController();

    /** Inizializza display, LVGL, touch e UI. @return true se riuscita */
    bool begin();

    /** Mostra boot logo (schermata idle) con fade-in del backlight */
    void showBootLogo(unsigned long display_time_ms = BOOT_LOGO_DISPLAY_TIME);

    /** Mostra schermata idle */
    void showIdleScreen();

    /** Mostra alert speedcam (ignorato visivamente se le impostazioni sono aperte) */
    void showSpeedcamAlert(const struct Speedcam& speedcam, float distance);

    /** Nasconde alert e torna alla schermata idle */
    void hideSpeedcamAlert();

    /** Da chiamare a ogni loop(): tick LVGL, touch, timeout alert */
    void update();

    /** Aggiorna indicatore e testo GPS (su entrambe le schermate) */
    void updateGPSIndicator(bool has_fix, uint8_t satellites);

    /** Imposta luminosita' backlight (10-100%) */
    void setBrightness(int percent);

    /** Raggio di detection corrente (metri), da impostazioni salvate o touch */
    int getDetectionRadius() const { return radius_m; }

    /** Callback chiamata quando l'utente cambia il raggio da touch */
    void setRadiusChangedCallback(void (*cb)(float radius_m));

private:
    LGFX* display;
    bool is_initialized;
    bool touch_present;

    // Stato alert
    bool showing_alert;
    unsigned long alert_start_time;
    unsigned long alert_display_time;

    // GPS status
    bool gps_has_fix;
    uint8_t gps_satellites;
    bool gps_dirty;

    int brightness_percent;
    int radius_m;
    bool settings_dirty;
    bool settings_were_active;
    void (*radius_changed_cb)(float);
    unsigned long last_tick_ms;

    // LVGL
    lv_disp_draw_buf_t draw_buf;
    lv_color_t* buf1;
    lv_color_t* buf2;
    lv_disp_drv_t disp_drv;
    lv_indev_drv_t indev_drv;

    static void flushCallback(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_p);
    static void touchCallback(lv_indev_drv_t* drv, lv_indev_data_t* data);
    static void brightnessCallback(int percent);
    static void radiusCallback(int radius_m);

    void serviceLvgl();
};

#endif // DISPLAY_CONTROLLER_H
