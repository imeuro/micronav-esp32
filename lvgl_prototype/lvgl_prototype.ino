/*
 * LVGL 8.3.3 Hello World per ESP32-C3
 * Display GC9A01 240x240
 * 
 * Questo esempio mostra come inizializzare LVGL 8.3.3
 * e visualizzare una label "Hello World" sul display GC9A01
 */

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

// Includi configurazione LVGL personalizzata
// Il file lv_conf.h nella stessa cartella verrà usato automaticamente da LVGL
#include <lvgl.h>
#include "../src/lgfx_panel_config.h"  // Classe LGFX condivisa col firmware principale
#include "ui.h" // SquareLine Studio

// Backlight (non fa parte della config pannello LGFX, gestito a parte)
#define DISPLAY_BL_PIN 3

// Configurazione pin touchscreen (I2C)
// NOTA: Verifica i pin corretti per il tuo hardware
// Pin già usati: 2=DC, 3=BL, 6=SCK(SPI), 7=MOSI(SPI), 10=CS
// Possibili pin I2C: 4,5,8,9,11,12,13,14,15,16,17,18,19,20,21
// Prova diverse combinazioni se necessario
#define TOUCH_SDA_PIN 4   // Prova anche: 8, 9, 11, 12
#define TOUCH_SCL_PIN 5   // Prova anche: 8, 9, 11, 12
#define TOUCH_RST_PIN -1  // Se non usato
#define TOUCH_INT_PIN -1  // Se non usato

// Indirizzo I2C touchscreen CST816S
#define CST816S_ADDR 0x15

// Dimensioni display
#define DISPLAY_WIDTH 240
#define DISPLAY_HEIGHT 240

// Buffer per LVGL (circa 11KB per 240x10 pixel)
#define LVGL_BUFFER_SIZE (DISPLAY_WIDTH * 10)

// Oggetto display: classe LGFX condivisa (config pannello verificata su
// hardware reale, inclusa cfg.invert = true - vedi src/lgfx_panel_config.h
// e openspec/specs/display-rendering/spec.md)
LGFX tft;

// Buffer LVGL
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[LVGL_BUFFER_SIZE];
static lv_color_t buf2[LVGL_BUFFER_SIZE];  // Doppio buffer per rendering fluido

// Display driver LVGL
static lv_disp_drv_t disp_drv;

// Touch input driver LVGL
static lv_indev_drv_t indev_drv;
static lv_indev_t *touch_indev = NULL;  // Riferimento all'input device registrato

/**
 * Event callback di debug per l'arc
 */
static void arc_debug_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);

    if (code == LV_EVENT_PRESSED) {
        Serial.println("[Arc Event] PREMUTO");
    } else if (code == LV_EVENT_PRESSING) {
        static unsigned long last_pressing = 0;
        if (millis() - last_pressing > 300) {
            Serial.println("[Arc Event] PREMENDO...");
            last_pressing = millis();
        }
    } else if (code == LV_EVENT_CLICKED) {
        Serial.println("[Arc Event] CLICK");
    } else if (code == LV_EVENT_VALUE_CHANGED) {
        Serial.print("[Arc Event] VALORE: ");
        Serial.println(lv_arc_get_value(target));
    }
}

/**
 * Funzione di flush per LVGL
 * Viene chiamata quando LVGL ha finito di disegnare un'area
 */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    uint32_t size = w * h;

    // Inizia scrittura SPI
    tft.startWrite();
    
    // Imposta finestra di scrittura
    tft.setAddrWindow(area->x1, area->y1, w, h);
    
    // Invia pixel riga per riga (più sicuro per buffer grandi)
    // color_p è un array di lv_color_t (RGB565)
    for (uint32_t y = 0; y < h; y++) {
        // Converti lv_color_t* a uint16_t* per questa riga
        uint16_t *row = (uint16_t *)&color_p[y * w];
        // Scrivi la riga (swap=true: i buffer LVGL usano lo stesso ordine
        // di byte colore di Adafruit GFX, verificato nel firmware principale)
        tft.writePixels(row, w, true);
    }
    
    // Termina scrittura SPI
    tft.endWrite();

    // Notifica LVGL che il flush è completato
    lv_disp_flush_ready(disp);
}

/**
 * Funzione di lettura touch per LVGL
 * Supporta CST816S touchscreen (I2C)
 * 
 * Se hai un touchscreen diverso, modifica questa funzione
 */
void my_touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
    static int16_t last_x = 0;
    static int16_t last_y = 0;
    static unsigned long last_debug = 0;
    static uint32_t call_count = 0;
    static bool first_call = true;
    
    call_count++;
    
    // Debug alla prima chiamata
    if (first_call) {
        Serial.println("[Touch] ✓✓✓ Funzione my_touchpad_read chiamata per la prima volta! ✓✓✓");
        first_call = false;
    }
    
    // Debug ogni 1000 chiamate (circa ogni 5 secondi se chiamato ogni 5ms)
    if (call_count % 1000 == 0) {
        Serial.print("[Touch] Funzione chiamata - count: ");
        Serial.println(call_count);
    }
    
    // Prova a leggere il registro di stato (0x02) direttamente
    Wire.beginTransmission(CST816S_ADDR);
    Wire.write(0x02); // Registro finger_num
    uint8_t error = Wire.endTransmission();
    
    if (error != 0) {
        // Errore comunicazione I2C
        if (millis() - last_debug > 3000) {
            Serial.print("[Touch] ERRORE I2C (codice: ");
            Serial.print(error);
            Serial.println(") - verifica collegamenti");
            last_debug = millis();
        }
        data->state = LV_INDEV_STATE_RELEASED;
        data->point.x = last_x;
        data->point.y = last_y;
        return;
    }
    
    // Leggi 1 byte (finger_num)
    Wire.requestFrom(CST816S_ADDR, 1);
    
    if (Wire.available() >= 1) {
        uint8_t finger_num = Wire.read();
        
        // Debug ogni 3 secondi
        if (millis() - last_debug > 3000) {
            Serial.print("[Touch] Finger_num: ");
            Serial.print(finger_num);
            Serial.print(" (0x");
            Serial.print(finger_num, HEX);
            Serial.println(")");
            last_debug = millis();
        }
        
        if (finger_num > 0 && finger_num < 5) {
            // Touch rilevato, leggi coordinate
            Wire.beginTransmission(CST816S_ADDR);
            Wire.write(0x03); // Inizia da XPOS_H
            Wire.endTransmission();
            Wire.requestFrom(CST816S_ADDR, 4);
            
            if (Wire.available() >= 4) {
                uint8_t x_h = Wire.read();
                uint8_t x_l = Wire.read();
                uint8_t y_h = Wire.read();
                uint8_t y_l = Wire.read();
                
                // Calcola coordinate
                int16_t x = ((x_h & 0x0F) << 8) | x_l;
                int16_t y = ((y_h & 0x0F) << 8) | y_l;

                // Calibrazione minima: inverti X rispetto alla larghezza
                // x = DISPLAY_WIDTH - x;

                // Debug coordinate quando toccato
                static unsigned long last_coord_debug = 0;
                if (millis() - last_coord_debug > 200) {
                    Serial.print("[Touch] ✓ TOUCH! X: ");
                    Serial.print(x);
                    Serial.print(", Y: ");
                    Serial.print(y);
                    Serial.print(", Finger: ");
                    Serial.print(finger_num);
                    Serial.print(" (raw: x_h=0x");
                    Serial.print(x_h, HEX);
                    Serial.print(", x_l=0x");
                    Serial.print(x_l, HEX);
                    Serial.print(", y_h=0x");
                    Serial.print(y_h, HEX);
                    Serial.print(", y_l=0x");
                    Serial.print(y_l, HEX);
                    Serial.println(")");
                    last_coord_debug = millis();
                }
                
                // Limita coordinate al display
                if (x < 0) x = 0;
                if (x >= DISPLAY_WIDTH) x = DISPLAY_WIDTH - 1;
                if (y < 0) y = 0;
                if (y >= DISPLAY_HEIGHT) y = DISPLAY_HEIGHT - 1;
                
                last_x = x;
                last_y = y;
                
                data->state = LV_INDEV_STATE_PRESSED;
                data->point.x = x;
                data->point.y = y;
                
                return;
            } else {
                if (millis() - last_debug > 3000) {
                    Serial.print("[Touch] Dati coordinate insufficienti: ");
                    Serial.println(Wire.available());
                    last_debug = millis();
                }
            }
        }
    } else {
        if (millis() - last_debug > 3000) {
            Serial.println("[Touch] Nessun dato disponibile da I2C");
            last_debug = millis();
        }
    }
    
    // Nessun touch rilevato
    data->state = LV_INDEV_STATE_RELEASED;
    data->point.x = last_x;
    data->point.y = last_y;
}

/**
 * Setup iniziale
 */
void setup() {
    // Inizializza Serial per debug
    Serial.begin(115200);
    delay(2000);  // Attendi che Serial sia pronto
    
    Serial.println("\n\n========================================");
    Serial.println("LVGL 8.3.3 Hello World - ESP32-C3");
    Serial.println("========================================\n");

    // Inizializza backlight (ma NON accenderlo ancora)
    pinMode(DISPLAY_BL_PIN, OUTPUT);
    digitalWrite(DISPLAY_BL_PIN, LOW);  // Backlight spento durante inizializzazione
    
    Serial.println("[Setup] Inizializzazione display GC9A01...");

    // Inizializza display (init() restituisce bool)
    bool display_ok = tft.init();
    Serial.print("[Setup] tft.init() risultato: ");
    Serial.println(display_ok ? "OK" : "FALLITO");

    // Pulisci schermo
    tft.fillScreen(0x0000);  // Nero

    Serial.println("[Setup] Display inizializzato con successo!");
    
    // Imposta orientamento (0 = normale)
    tft.setRotation(0);

    // Accendi backlight DOPO che il display è inizializzato e pulito
    digitalWrite(DISPLAY_BL_PIN, HIGH);

    Serial.println("[Setup] Inizializzazione LVGL...");
    
    // Inizializza LVGL
    lv_init();
    
    // Inizializza buffer di disegno (doppio buffer per rendering fluido)
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, LVGL_BUFFER_SIZE);
    
    Serial.println("[Setup] Buffer LVGL inizializzati");
    
    // Inizializza display driver
    lv_disp_drv_init(&disp_drv);
    
    // Configura risoluzione
    disp_drv.hor_res = DISPLAY_WIDTH;
    disp_drv.ver_res = DISPLAY_HEIGHT;
    
    // Imposta callback di flush
    disp_drv.flush_cb = my_disp_flush;
    
    // Imposta buffer di disegno
    disp_drv.draw_buf = &draw_buf;
    
    // Registra display driver
    lv_disp_drv_register(&disp_drv);
    
    Serial.println("[Setup] Display driver LVGL registrato");
    
    // Inizializza I2C per touchscreen
    Serial.println("[Setup] Inizializzazione I2C per touchscreen...");
    Serial.print("[Setup] Pin I2C - SDA: ");
    Serial.print(TOUCH_SDA_PIN);
    Serial.print(", SCL: ");
    Serial.println(TOUCH_SCL_PIN);
    
    Wire.begin(TOUCH_SDA_PIN, TOUCH_SCL_PIN);
    delay(100);
    
    // Inizializza touchscreen CST816S se presente
    Wire.beginTransmission(CST816S_ADDR);
    if (Wire.endTransmission() == 0) {
        Serial.println("[Setup] Inizializzazione CST816S...");
        // Reset touchscreen (se pin RST disponibile)
        if (TOUCH_RST_PIN >= 0) {
            pinMode(TOUCH_RST_PIN, OUTPUT);
            digitalWrite(TOUCH_RST_PIN, LOW);
            delay(10);
            digitalWrite(TOUCH_RST_PIN, HIGH);
            delay(50);
        }
        // Abilita interrupt (se pin INT disponibile)
        if (TOUCH_INT_PIN >= 0) {
            pinMode(TOUCH_INT_PIN, INPUT_PULLUP);
        }
        Serial.println("[Setup] CST816S inizializzato");
    }
    
    // Scanner I2C: cerca dispositivi I2C collegati
    Serial.println("[Setup] Scansione bus I2C...");
    byte devices_found = 0;
    for (byte address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        byte error = Wire.endTransmission();
        
        if (error == 0) {
            Serial.print("[Setup] Dispositivo I2C trovato all'indirizzo 0x");
            if (address < 16) Serial.print("0");
            Serial.print(address, HEX);
            Serial.print(" (");
            Serial.print(address);
            Serial.println(")");
            devices_found++;
            
            // Indirizzi comuni touchscreen
            if (address == 0x15) {
                Serial.println("[Setup] ✓ Probabilmente CST816S touchscreen!");
            } else if (address == 0x38) {
                Serial.println("[Setup] ✓ Probabilmente FT5x06 touchscreen!");
            } else if (address == 0x5D || address == 0x5C) {
                Serial.println("[Setup] ✓ Potrebbe essere un touchscreen!");
            }
        }
    }
    
    if (devices_found == 0) {
        Serial.println("[Setup] ⚠️ Nessun dispositivo I2C trovato!");
        Serial.println("[Setup] Verifica:");
        Serial.println("[Setup]   1. Pin I2C corretti (SDA/SCL)");
        Serial.println("[Setup]   2. Collegamenti hardware");
        Serial.println("[Setup]   3. Alimentazione touchscreen");
        Serial.println("[Setup]   4. Prova altri pin (es. 8,9 o 11,12)");
    } else {
        Serial.print("[Setup] Totale dispositivi I2C trovati: ");
        Serial.println(devices_found);
    }
    
    // Test lettura touchscreen prima di registrare
    /*
    Serial.println("[Setup] Test lettura touchscreen...");
    Wire.beginTransmission(CST816S_ADDR);
    Wire.write(0x02); // Registro finger_num
    uint8_t test_error = Wire.endTransmission();
    if (test_error == 0) {
        Wire.requestFrom(CST816S_ADDR, 1);
        if (Wire.available()) {
            uint8_t test_finger = Wire.read();
            Serial.print("[Setup] Test lettura OK - finger_num: ");
            Serial.println(test_finger);
        } else {
            Serial.println("[Setup] ⚠️ Test lettura: nessun dato disponibile");
        }
    } else {
        Serial.print("[Setup] ⚠️ Test lettura fallito - errore I2C: ");
        Serial.println(test_error);
    }
    */
    
    // Inizializza input device (touch) driver
    Serial.println("[Setup] Inizializzazione touch driver...");
    Serial.println("[Setup] Verifica callback function...");
    Serial.print("[Setup] Indirizzo my_touchpad_read: 0x");
    Serial.println((uintptr_t)my_touchpad_read, HEX);
    
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    
    Serial.println("[Setup] Registrazione input device...");
    touch_indev = lv_indev_drv_register(&indev_drv);
    
    if (touch_indev != NULL) {
        Serial.println("[Setup] ✓ Touch driver registrato con successo");
        Serial.print("[Setup] Input device pointer: 0x");
        Serial.println((uintptr_t)touch_indev, HEX);
        Serial.println("[Setup] NOTA: La lettura touch verrà chiamata manualmente nel loop");
    } else {
        Serial.println("[Setup] ✗✗✗ ERRORE: Touch driver non registrato! ✗✗✗");
    }
    Serial.println("[Setup] NOTA: Se le coordinate touch sono invertite, modifica my_touchpad_read()");
    Serial.println("[Setup] La funzione my_touchpad_read dovrebbe essere chiamata periodicamente da LVGL");
    
    // Imposta colore di sfondo dello schermo (nero)
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), LV_PART_MAIN);
    
    // Crea label "Hello World"
    Serial.println("[Setup] Creazione label Hello World...");
    
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello.");
    
    // Allinea al centro dello schermo
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    
    // Imposta stile (testo bianco su sfondo nero)
    lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
    // Usa font di default (LVGL 8.3.3 ha font built-in)
    // Se disponibile, usa montserrat, altrimenti usa default
    #ifdef LV_FONT_MONTSERRAT_20
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, LV_PART_MAIN);
    #else
    // Usa font di default se montserrat non è disponibile
    lv_obj_set_style_text_font(label, LV_FONT_DEFAULT, LV_PART_MAIN);
    #endif
    
    // Forza refresh immediato
    lv_refr_now(NULL);
    
    Serial.println("[Setup] Label creata e configurata");
    Serial.println("[Setup] Refresh forzato");
    Serial.println("\n[Setup] Setup completato!");
    Serial.println("========================================\n");

    ui_init();

    Serial.println("[Setup] SquareLine Studio UI inizializzata");
    
    // Configura arc per essere interattivo
    // IMPORTANTE: L'arc deve essere configurato con LV_ARC_MODE_NORMAL per essere interattivo
    if (ui_Arc1 != NULL) {
        // Imposta modalità normale (interattiva)
        lv_arc_set_mode(ui_Arc1, LV_ARC_MODE_NORMAL);
        
        // Assicurati che l'arc possa ricevere input touch
        lv_obj_clear_flag(ui_Arc1, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(ui_Arc1, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(ui_Arc1, LV_OBJ_FLAG_ADV_HITTEST);
        
        // Porta l'arc in primo piano
        lv_obj_move_foreground(ui_Arc1);

        // Aggiungi event handler di debug per verificare gli eventi
        lv_obj_add_event_cb(ui_Arc1, arc_debug_event_cb, LV_EVENT_ALL, NULL);
        Serial.println("[Setup] Arc event handler registrato");

        Serial.println("[Setup] Arc configurato per interazione touch");
        Serial.print("[Setup] Arc mode: ");
        Serial.println(lv_arc_get_mode(ui_Arc1));
    } else {
        Serial.println("[Setup] ERRORE: ui_Arc1 non trovato!");
    }
    
    Serial.println("[Setup] Refresh forzato");
    Serial.println("\n[Setup] Setup completato!");
    Serial.println("========================================\n");
    Serial.println("[Setup] Tocca lo schermo per vedere le coordinate nel monitor seriale");

    lv_refr_now(NULL);
}

/**
 * Loop principale
 */
void loop() {
    // Chiama il timer handler di LVGL (necessario per aggiornare display)
    // Deve essere chiamato periodicamente, almeno ogni 5ms
    lv_timer_handler();

    // Aggiorna il tick di LVGL (usando il delay sottostante di 5ms)
    lv_tick_inc(5);
    
    // Debug ogni 5 secondi per verificare che il loop funzioni
    static unsigned long last_loop_debug = 0;
    if (millis() - last_loop_debug > 5000) {
        Serial.print("[Loop] Loop attivo - millis: ");
        Serial.println(millis());
        last_loop_debug = millis();
    }
    
    // Piccolo delay per evitare loop troppo veloce
    delay(5);
}

