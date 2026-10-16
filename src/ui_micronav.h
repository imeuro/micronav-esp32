#ifndef UI_MICRONAV_H
#define UI_MICRONAV_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <stdbool.h>

// UI alternativa, disegnata sul layout reale dell'app (non SquareLine
// Studio): schermata idle (boot logo + stato GPS) e schermata alert
// autovelox, fedeli a src/display_controller.cpp del firmware principale.
//
// File separato apposta: la UI generata da SquareLine Studio (ui.c,
// ui_Screen1.c, ui_Screen2.c) resta intatta e selezionabile, nel caso si
// preferisca ripartire da quella.

// Crea entrambe le schermate e mostra la schermata idle
void micronav_ui_init(void);

// Schermata idle: aggiorna indicatore e testo stato GPS
void micronav_show_idle_screen(void);
void micronav_update_gps_status(bool has_fix, int satellites);

// Schermata alert: type_text ("VELOX"/"T RED"), status_text
// ("attivo"/"inattivo"), distance_m (metri), is_semaforo (true = mostra
// "TL" invece del numero), vmax_text (numero limite, ignorato se
// is_semaforo)
void micronav_show_speedcam_alert(const char *type_text, const char *status_text,
                                   int distance_m, bool is_semaforo,
                                   const char *vmax_text);
void micronav_hide_speedcam_alert(void);

// Schermata impostazioni: slider ad arco per la luminosità (10-100%).
// Raggiungibile toccando il pallino indicatore GPS nella schermata idle.
// brightness_initial_pct: valore iniziale dello slider (10-100).
// on_brightness_change: chiamata ogni volta che l'arco cambia valore,
// con la nuova percentuale di luminosità.
typedef void (*micronav_brightness_cb_t)(int percent);
void micronav_set_brightness_callback(micronav_brightness_cb_t cb);
void micronav_show_settings_screen(int brightness_initial_pct);

// Seconda pagina impostazioni: raggio di detection in metri (200-2000, passi
// da 50). La callback riceve il nuovo valore a ogni modifica.
typedef void (*micronav_radius_cb_t)(int radius_m);
void micronav_set_radius_callback(micronav_radius_cb_t cb);

// Imposta i valori iniziali degli slider (es. letti dalla memoria)
void micronav_set_settings_values(int brightness_pct, int radius_m);

// true se la schermata impostazioni e' quella attualmente mostrata (usato
// per sospendere il ciclo demo automatico mentre l'utente regola lo slider)
bool micronav_is_settings_active(void);

#ifdef __cplusplus
}
#endif

#endif // UI_MICRONAV_H
