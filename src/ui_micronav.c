#include "ui_micronav.h"
LV_IMG_DECLARE(ui_img_boot_logo_png);
#include <stdio.h>

static lv_obj_t *scr_idle = NULL;
static lv_obj_t *scr_alert = NULL;
static lv_obj_t *scr_settings = NULL;
static bool settings_active = false;

static lv_obj_t *settings_arc = NULL;
static lv_obj_t *settings_value_label = NULL;
static micronav_brightness_cb_t brightness_callback = NULL;

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

// Apre la schermata impostazioni (tap sul pallino GPS nella schermata idle)
static void gps_icon_click_cb(lv_event_t *e) {
    (void)e;
    settings_active = true;
    lv_scr_load(scr_settings);
}

// Arco luminosita': aggiorna l'etichetta percentuale e notifica il
// firmware (callback registrata con micronav_set_brightness_callback)
static void settings_arc_event_cb(lv_event_t *e) {
    lv_obj_t *arc = lv_event_get_target(e);
    int value = lv_arc_get_value(arc);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", value);
    lv_label_set_text(settings_value_label, buf);
    if (brightness_callback) brightness_callback(value);
}

// Torna alla schermata idle
static void settings_back_cb(lv_event_t *e) {
    (void)e;
    settings_active = false;
    lv_scr_load(scr_idle);
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

    // Tocca il pallino GPS per aprire le impostazioni (luminosità)
    lv_obj_add_flag(gps_indicator_circle, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_ext_click_area(gps_indicator_circle, 30);  // il pallino e' solo 16px: area di tocco allargata
    lv_obj_add_event_cb(gps_indicator_circle, gps_icon_click_cb, LV_EVENT_CLICKED, NULL);
}

// Crea la schermata impostazioni: arco per regolare la luminosità (10-100%),
// con etichetta percentuale al centro e link "Indietro" in basso.
static void build_settings_screen(void) {
    scr_settings = create_black_screen();

    lv_obj_t *title = create_label(scr_settings, "Luminosita'", lv_color_white());
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 28);  // sotto il valore percentuale

    settings_arc = lv_arc_create(scr_settings);
    lv_obj_set_size(settings_arc, 215, 215);
    lv_obj_center(settings_arc);
    lv_arc_set_bg_angles(settings_arc, 135, 45);  // arco a "misuratore", apertura in basso
    lv_arc_set_rotation(settings_arc, 0);
    lv_arc_set_range(settings_arc, 10, 100);
    lv_arc_set_value(settings_arc, 80);
    lv_obj_set_style_arc_color(settings_arc, COLOR_MICRONAV_RED, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(settings_arc, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(settings_arc, 10, LV_PART_MAIN);
    lv_obj_add_event_cb(settings_arc, settings_arc_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    settings_value_label = create_label(scr_settings, "80%", lv_color_white());
    lv_obj_set_style_text_font(settings_value_label, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align_to(settings_value_label, settings_arc, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *back_label = create_label(scr_settings, "< Indietro", lv_color_white());
    lv_obj_align(back_label, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_flag(back_label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(back_label, settings_back_cb, LV_EVENT_CLICKED, NULL);
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
    lv_obj_add_flag(gps_indicator_circle_alert, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_ext_click_area(gps_indicator_circle_alert, 30);
    lv_obj_add_event_cb(gps_indicator_circle_alert, gps_icon_click_cb, LV_EVENT_CLICKED, NULL);
}

void micronav_ui_init(void) {
    build_idle_screen();
    build_alert_screen();
    build_settings_screen();
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

bool micronav_is_settings_active(void) {
    return settings_active;
}
