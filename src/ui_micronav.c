#include "ui_micronav.h"
LV_IMG_DECLARE(ui_img_boot_logo_png);
#include <stdio.h>

static lv_obj_t *scr_idle = NULL;
static lv_obj_t *scr_alert = NULL;
static lv_obj_t *scr_settings = NULL;
static bool settings_active = false;

static lv_obj_t *scr_settings_radius = NULL;
static lv_obj_t *settings_arc = NULL;
static lv_obj_t *settings_value_label = NULL;
static lv_obj_t *radius_arc = NULL;
static lv_obj_t *radius_value_label = NULL;
static micronav_brightness_cb_t brightness_callback = NULL;
static micronav_radius_cb_t radius_callback = NULL;

#define RADIUS_STEP_M 50

// true se durante il tocco corrente l'utente ha spostato un arco: in quel caso
// il gesto non deve cambiare pagina (la parte bassa dell'anello, sul vuoto
// dell'arco, e' comunque 'arco' per il hit test di LVGL)
static bool arc_value_moved = false;

// Schermata idle
static lv_obj_t *gps_indicator_circle = NULL;
static lv_obj_t *gps_status_label = NULL;
static lv_obj_t *gps_sat_label = NULL;

// Indicatore GPS + info satelliti, presenti anche nella schermata alert
// (persistenti su entrambe le schermate, a differenza del firmware
// attuale che li nasconde durante l'alert - scelta di layout da
// confermare prima di riportarla nell'implementazione reale)
static lv_obj_t *gps_indicator_circle_alert = NULL;
static lv_obj_t *gps_status_label_alert = NULL;
static lv_obj_t *gps_sat_label_alert = NULL;

// Schermata alert
static lv_obj_t *alert_type_label = NULL;
static lv_obj_t *alert_status_label = NULL;
static lv_obj_t *alert_distance_label = NULL;
static lv_obj_t *alert_circle = NULL;
static lv_obj_t *alert_circle_text = NULL;

LV_FONT_DECLARE(geist_black_26);
LV_FONT_DECLARE(geist_black_30);

// Colori ripresi da src/config.h (conversione da RGB565 a 24 bit)
#define COLOR_MICRONAV_RED  lv_color_make(197, 12, 58)
#define COLOR_LIGHT_GRAY    lv_color_make(198, 198, 198)

static lv_obj_t *create_black_screen(void) {
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(scr, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(scr, 0, LV_PART_MAIN);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    return scr;
}

static lv_obj_t *create_label(lv_obj_t *parent, const char *text, lv_color_t color) {
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, LV_PART_MAIN);
    return label;
}

// Naviga verso una schermata con scorrimento verticale (le schermate sono
// "impilate": swipe verso l'alto avanza, verso il basso torna indietro)
static void go_to_screen(lv_obj_t *target, lv_scr_load_anim_t anim) {
    settings_active = (target == scr_settings || target == scr_settings_radius);
    lv_scr_load_anim(target, anim, 180, 0, false);
}

// Anello verticale idle -> luminosita' -> raggio -> idle: swipe verso l'alto
// avanza, verso il basso va nella direzione opposta (idle -> raggio -> luminosita' -> idle)
static void arc_released_cb(lv_event_t *e) {
    (void)e;
    arc_value_moved = false;
}

static void screen_gesture_cb(lv_event_t *e) {
    if (arc_value_moved) return;
    lv_obj_t *cur = lv_event_get_current_target(e);
    if (cur != lv_scr_act()) return;
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());

    if (dir == LV_DIR_TOP) {
        if (cur == scr_idle || cur == scr_alert) go_to_screen(scr_settings, LV_SCR_LOAD_ANIM_MOVE_TOP);
        else if (cur == scr_settings) go_to_screen(scr_settings_radius, LV_SCR_LOAD_ANIM_MOVE_TOP);
        else if (cur == scr_settings_radius) go_to_screen(scr_idle, LV_SCR_LOAD_ANIM_MOVE_TOP);
    } else if (dir == LV_DIR_BOTTOM) {
        if (cur == scr_idle || cur == scr_alert) go_to_screen(scr_settings_radius, LV_SCR_LOAD_ANIM_MOVE_BOTTOM);
        else if (cur == scr_settings_radius) go_to_screen(scr_settings, LV_SCR_LOAD_ANIM_MOVE_BOTTOM);
        else if (cur == scr_settings) go_to_screen(scr_idle, LV_SCR_LOAD_ANIM_MOVE_BOTTOM);
    }
}

// Arco luminosita': aggiorna l'etichetta percentuale e notifica il
// firmware (callback registrata con micronav_set_brightness_callback)
static void settings_arc_event_cb(lv_event_t *e) {
    arc_value_moved = true;
    lv_obj_t *arc = lv_event_get_target(e);
    int value = lv_arc_get_value(arc);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", value);
    lv_label_set_text(settings_value_label, buf);
    if (brightness_callback) brightness_callback(value);
}

// Arco raggio: arrotonda a passi di RADIUS_STEP_M, aggiorna etichetta e notifica
static void radius_arc_event_cb(lv_event_t *e) {
    arc_value_moved = true;
    lv_obj_t *arc = lv_event_get_target(e);
    int value = lv_arc_get_value(arc);
    int snapped = ((value + RADIUS_STEP_M / 2) / RADIUS_STEP_M) * RADIUS_STEP_M;
    if (snapped != value) lv_arc_set_value(arc, snapped);
    char buf[12];
    snprintf(buf, sizeof(buf), "%d m", snapped);
    lv_label_set_text(radius_value_label, buf);
    if (radius_callback) radius_callback(snapped);
}

// Crea indicatore GPS (pallino in alto) + testo stato/satelliti (in basso),
// identici su qualunque schermata li ospiti.
static void build_gps_widgets(lv_obj_t *parent, lv_obj_t **circle_out,
                               lv_obj_t **status_out, lv_obj_t **sat_out) {
    lv_obj_t *circle = lv_obj_create(parent);
    lv_obj_remove_style_all(circle);
    lv_obj_set_size(circle, 16, 16);
    lv_obj_set_pos(circle, 120 - 8, 15 - 8);
    lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(circle, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(circle, LV_OPA_COVER, LV_PART_MAIN);
    *circle_out = circle;

    lv_obj_t *status = create_label(parent, "GPS: In attesa...", lv_color_white());
    lv_obj_set_width(status, LV_SIZE_CONTENT);
    lv_obj_align(status, LV_ALIGN_TOP_MID, 0, 190);
    *status_out = status;

    lv_obj_t *sat = create_label(parent, "", lv_color_white());
    lv_obj_align(sat, LV_ALIGN_TOP_MID, 0, 205);
    *sat_out = sat;
}

// Crea la schermata idle: boot logo nella stessa posizione del firmware
// (offset 20,95 - 200x50, vedi src/boot_logo.h), indicatore GPS in alto al
// centro, testo stato GPS in basso.
static void build_idle_screen(void) {
    scr_idle = create_black_screen();

    lv_obj_t *logo = lv_img_create(scr_idle);
    lv_img_set_src(logo, &ui_img_boot_logo_png);
    lv_obj_set_pos(logo, 20, 95);

    build_gps_widgets(scr_idle, &gps_indicator_circle, &gps_status_label, &gps_sat_label);
}

// Crea una pagina impostazioni: arco 215px, valore al centro, titolo sotto il
// valore, indicatore di pagina (due puntini) in basso. La navigazione e' a
// swipe (screen_gesture_cb); se l'arco e' stato spostato durante il tocco
// il gesto viene ignorato (arc_value_moved).
static lv_obj_t *build_settings_page(const char *title_text, int page_index,
                                      int range_min, int range_max, int value,
                                      const char *value_text, lv_event_cb_t arc_cb,
                                      lv_obj_t **arc_out, lv_obj_t **value_label_out) {
    lv_obj_t *scr = create_black_screen();

    lv_obj_t *arc = lv_arc_create(scr);
    lv_obj_set_size(arc, 215, 215);
    lv_obj_center(arc);
    lv_arc_set_bg_angles(arc, 135, 45);  // arco a "misuratore", apertura in basso
    lv_arc_set_rotation(arc, 0);
    lv_arc_set_range(arc, range_min, range_max);
    lv_arc_set_value(arc, value);
    lv_obj_set_style_arc_color(arc, COLOR_MICRONAV_RED, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, 10, LV_PART_MAIN);
    lv_obj_add_event_cb(arc, arc_released_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(arc, arc_released_cb, LV_EVENT_PRESS_LOST, NULL);
    lv_obj_add_event_cb(arc, arc_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *value_label = create_label(scr, value_text, lv_color_white());
    lv_obj_set_style_text_font(value_label, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align_to(value_label, arc, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *title = create_label(scr, title_text, lv_color_white());
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 28);  // sotto il valore

    for (int i = 0; i < 2; i++) {
        lv_obj_t *dot = lv_obj_create(scr);
        lv_obj_remove_style_all(dot);
        lv_obj_set_size(dot, 8, 8);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_bg_color(dot, i == page_index ? lv_color_white() : lv_color_make(80, 80, 80), LV_PART_MAIN);
        lv_obj_align(dot, LV_ALIGN_BOTTOM_MID, i == 0 ? -8 : 8, -22);
    }

    *arc_out = arc;
    *value_label_out = value_label;
    return scr;
}

static void build_settings_screens(void) {
    scr_settings = build_settings_page("Luminosita'", 0,
                                        10, 100, 80, "80%", settings_arc_event_cb,
                                        &settings_arc, &settings_value_label);
    scr_settings_radius = build_settings_page("Raggio", 1,
                                               200, 2000, 1000, "1000 m", radius_arc_event_cb,
                                               &radius_arc, &radius_value_label);
}

// Crea la schermata alert: nessun riquadro colorato (sfondo nero), testo
// tipo/stato/distanza a sinistra, cerchio indicatore (bianco, bordo rosso)
// con numero limite o "TL" a destra. Layout/coordinate riprese da
// drawSpeedcamAlertContent() in src/display_controller.cpp.
static void build_alert_screen(void) {
    scr_alert = create_black_screen();

    alert_type_label = create_label(scr_alert, "VELOX", lv_color_white());
    lv_obj_align(alert_type_label, LV_ALIGN_TOP_LEFT, 20, 92);

    alert_status_label = create_label(scr_alert, "attivo", COLOR_LIGHT_GRAY);
    lv_obj_align(alert_status_label, LV_ALIGN_TOP_LEFT, 20, 108);

    alert_distance_label = create_label(scr_alert, "250m", lv_color_white());
    lv_obj_set_style_text_font(alert_distance_label, &geist_black_30, LV_PART_MAIN);
    lv_obj_align(alert_distance_label, LV_ALIGN_TOP_LEFT, 20, 126);

    // Cerchio indicatore: diametro 66 (radius 33, come indicator_size/2 nel
    // firmware), centrato a (187, 120) - stessa posizione di indicator_x/y
    alert_circle = lv_obj_create(scr_alert);
    lv_obj_remove_style_all(alert_circle);
    lv_obj_set_size(alert_circle, 66, 66);
    lv_obj_set_pos(alert_circle, 187 - 33, 120 - 33);
    lv_obj_set_style_radius(alert_circle, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(alert_circle, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(alert_circle, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(alert_circle, COLOR_MICRONAV_RED, LV_PART_MAIN);
    lv_obj_set_style_border_width(alert_circle, 8, LV_PART_MAIN);
    lv_obj_clear_flag(alert_circle, LV_OBJ_FLAG_SCROLLABLE);

    alert_circle_text = create_label(alert_circle, "50", lv_color_black());
    lv_obj_set_style_text_font(alert_circle_text, &geist_black_26, LV_PART_MAIN);
    lv_obj_set_style_text_letter_space(alert_circle_text, -1, LV_PART_MAIN);
    lv_obj_align(alert_circle_text, LV_ALIGN_CENTER, 0, 0);

    // Indicatore + info GPS, persistenti anche durante l'alert
    build_gps_widgets(scr_alert, &gps_indicator_circle_alert, &gps_status_label_alert, &gps_sat_label_alert);
}

void micronav_ui_init(void) {
    build_idle_screen();
    build_alert_screen();
    build_settings_screens();
    lv_obj_t *screens[] = { scr_idle, scr_alert, scr_settings, scr_settings_radius };
    for (int i = 0; i < 4; i++) {
        lv_obj_add_event_cb(screens[i], screen_gesture_cb, LV_EVENT_GESTURE, NULL);
    }
    lv_scr_load(scr_idle);
}

void micronav_show_idle_screen(void) {
    lv_scr_load(scr_idle);
}

static void update_one_gps_widget(lv_obj_t *circle, lv_obj_t *status, lv_obj_t *sat,
                                   bool has_fix, int satellites) {
    lv_obj_set_style_bg_color(circle,
                               has_fix ? lv_palette_main(LV_PALETTE_GREEN) : lv_palette_main(LV_PALETTE_RED),
                               LV_PART_MAIN);
    lv_label_set_text(status, has_fix ? "GPS: Fix OK" : "GPS: In attesa...");
    if (has_fix) {
        char buf[16];
        snprintf(buf, sizeof(buf), "Sat: %d", satellites);
        lv_label_set_text(sat, buf);
    } else {
        lv_label_set_text(sat, "");
    }
}

void micronav_update_gps_status(bool has_fix, int satellites) {
    update_one_gps_widget(gps_indicator_circle, gps_status_label, gps_sat_label, has_fix, satellites);
    update_one_gps_widget(gps_indicator_circle_alert, gps_status_label_alert, gps_sat_label_alert, has_fix, satellites);
}

void micronav_show_speedcam_alert(const char *type_text, const char *status_text,
                                   int distance_m, bool is_semaforo,
                                   const char *vmax_text) {
    lv_label_set_text(alert_type_label, type_text);
    lv_label_set_text(alert_status_label, status_text);

    char dist_buf[16];
    snprintf(dist_buf, sizeof(dist_buf), "%dm", distance_m);
    lv_label_set_text(alert_distance_label, dist_buf);

    lv_label_set_text(alert_circle_text, is_semaforo ? "TL" : vmax_text);

    lv_scr_load(scr_alert);
}

void micronav_hide_speedcam_alert(void) {
    lv_scr_load(scr_idle);
}

void micronav_set_brightness_callback(micronav_brightness_cb_t cb) {
    brightness_callback = cb;
}

void micronav_set_radius_callback(micronav_radius_cb_t cb) {
    radius_callback = cb;
}

void micronav_set_settings_values(int brightness_pct, int radius_m) {
    char buf[12];
    lv_arc_set_value(settings_arc, brightness_pct);
    snprintf(buf, sizeof(buf), "%d%%", brightness_pct);
    lv_label_set_text(settings_value_label, buf);
    lv_arc_set_value(radius_arc, radius_m);
    snprintf(buf, sizeof(buf), "%d m", radius_m);
    lv_label_set_text(radius_value_label, buf);
}

void micronav_show_settings_screen(int brightness_initial_pct) {
    if (settings_arc) {
        lv_arc_set_value(settings_arc, brightness_initial_pct);
        char buf[8];
        snprintf(buf, sizeof(buf), "%d%%", brightness_initial_pct);
        lv_label_set_text(settings_value_label, buf);
    }
    settings_active = true;
    lv_scr_load(scr_settings);
}

void micronav_close_settings(void) {
    settings_active = false;
    lv_scr_load_anim(scr_idle, LV_SCR_LOAD_ANIM_FADE_ON, 250, 0, false);
}

bool micronav_is_settings_active(void) {
    return settings_active;
}
