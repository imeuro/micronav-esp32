#ifndef CONFIG_H
#define CONFIG_H

// Compatibilità Arduino IDE
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#elif !defined(MICRONAV_HOST_TEST)
#include "WProgram.h"
#endif

// ============================================
// Configurazione Hardware
// ============================================

// GPS Configuration
#define GPS_SERIAL_BAUD 9600
// Connettore SH1.0-4P onboard (GND, 3.3V, IO20, IO21) = GPIO20 / GPIO21 dell'ESP32-C3;
// i pin 4/5 sono occupati dal touch CST816S. Verificato con test di scansione (NEO-M8):
// il TX del GPS e' collegato al pin GPIO21 del connettore, a 9600 baud.
#define GPS_SERIAL_RX_PIN 21   // RX dell'ESP32 (riceve dal TX del GPS)
#define GPS_SERIAL_TX_PIN -1   // TX dell'ESP32 non usato (come nello sketch di test che riceveva NMEA)
#define GPS_FIX_TIMEOUT 45000  // 45 secondi in millisecondi
#define GPS_MIN_SATELLITES 4
#define GPS_MAX_HDOP 5.0

// GPS Fake Mode (per test senza GPS hardware)
// Imposta a true per usare coordinate fake da file JSON invece del GPS reale
#define GPS_FAKE_MODE false
#define GPS_FAKE_JSON_PATH "/fake_gps.json"  // Path file JSON con coordinate fake
#define GPS_FAKE_UPDATE_INTERVAL 2000  // Intervallo aggiornamento posizione fake in millisecondi

// Speedcam Configuration
#define SPEEDCAM_DETECTION_RADIUS 1000  // Raggio di rilevazione in metri (default 1km)
#define SETTINGS_RADIUS_MIN_M 200       // Limiti regolabili da touch (devono coincidere con lo slider in ui_micronav.c)
#define SETTINGS_RADIUS_MAX_M 2000
#define SETTINGS_IDLE_TIMEOUT_MS 5000   // Inattivita' (nessun tocco) dopo cui le impostazioni si chiudono da sole
#define SPEEDCAM_CHECK_INTERVAL 1000    // Intervallo tra check in millisecondi (unico throttle, il GPS aggiorna a 1 Hz)
#define SPEEDCAM_JSON_PATH "/speedcams.json"
#define SPEEDCAM_ENABLED true

// Log dettagliato per la prova su strada (taratura cono, soglie e SPEEDCAM_DIR_OFFSET_DEG): mettere a 0 dopo
#define SPEEDCAM_ROAD_TEST_LOG 1

// Log degli eventi significativi su LittleFS (alert, passaggi, perdita GPS, ricarica DB).
// Si scarica con ./dump_roadlog.sh. Ruota a ROAD_LOG_MAX_BYTES (file corrente + .old).
#define ROAD_LOG_ENABLED 1
#define ROAD_LOG_PATH "/roadlog.txt"
#define ROAD_LOG_OLD_PATH "/roadlog.old.txt"
#define ROAD_LOG_MAX_BYTES 80000
#define ROAD_LOG_MIN_FREE_BYTES 24000      // Sotto questo spazio libero il log si ferma
#define ROAD_LOG_FLUSH_MS 15000            // Scrittura su flash al massimo ogni 15 s (e a ogni HIDE)
#define ROAD_LOG_TRACK_INTERVAL_MS 2000    // Riga di avanzamento durante un alert

// Detection direzionale (vedi openspec/changes/refine-speedcam-detection/design.md)
#define SPEEDCAM_MIN_SPEED_KMH 10          // Sotto questa velocita' il course GPS non e' affidabile
#define SPEEDCAM_HEADING_HOLD_MS 10000     // Quanto si mantiene l'ultimo heading valido sotto soglia
#define SPEEDCAM_HEADING_FALLBACK_MIN_M 10 // Distanza minima tra fix per calcolare il bearing di ripiego
#define SPEEDCAM_CONE_HALF_ANGLE_DEG 60    // Semiangolo massimo del cono di marcia (da vicino)
#define SPEEDCAM_CONE_LATERAL_M 150        // Scarto laterale ammesso: il cono si restringe con la distanza (atan(lat/d))
#define SPEEDCAM_CONE_HYSTERESIS_DEG 20    // Margine in piu' per il POI gia' tracciato: evita che l'alert lampeggi con heading instabile
#define SPEEDCAM_CONE_MIN_DEG 15           // Semiangolo minimo del cono, a grande distanza
#define SPEEDCAM_MAX_LATERAL_M 170         // Scarto laterale massimo (distanza x seno dell'angolo) per ammettere un POI nuovo: scarta le strade parallele lontane (il POI tracciato resta governato da cono + isteresi)
#define SPEEDCAM_SWITCH_MARGIN_M 30        // Un POI diverso sostituisce quello tracciato solo se piu' vicino di almeno cosi'
#define SPEEDCAM_GPS_LATENCY_MS 700        // Ritardo stimato tra posizione reale e fix GPS, oltre all'eta' del fix
#define SPEEDCAM_ALERT_ORANGE_MARGIN_KMH 10 // Sforamento massimo del limite per lo sfondo arancio (oltre: rosso)
#define SPEEDCAM_ALERT_FLASH_COUNT 3       // Lampeggi rossi all'inizio di un alert
#define SPEEDCAM_ALERT_FLASH_MS 150        // Durata di ogni fase (acceso/spento) del lampeggio
#define SPEEDCAM_NEAR_FIELD_M 40           // Entro questa distanza il cono non si applica
#define SPEEDCAM_PASSED_ANGLE_DEG 90       // Oltre questo angolo il POI e' alle spalle (superato)
#define SPEEDCAM_PASSED_HYSTERESIS_M 25    // Allontanamento oltre min_distance + isteresi = superato
#define SPEEDCAM_RELEASE_FACTOR 1.2f       // Un POI superato torna eleggibile oltre raggio * fattore
#define SPEEDCAM_DIR_NONE 0xFFFF           // POI senza direzione nel database
#define SPEEDCAM_DIR_TOLERANCE_DEG 75      // Scarto massimo tra heading e direzione del POI
#define SPEEDCAM_DIR_OFFSET_DEG 0          // Da verificare su strada: se "richtung" indica il lato verso cui punta
                                           // il dispositivo e non il traffico controllato, impostare 180

// Display Configuration (GC9A01 240x240 onboard)
#define DISPLAY_WIDTH 240
#define DISPLAY_HEIGHT 240
#define DISPLAY_DRIVER_GC9A01
#define BOOT_LOGO_DISPLAY_TIME 3000  // Tempo visualizzazione boot logo in millisecondi
#define BOOT_LOGO_FADE_ENABLED true  // Abilita fade-in del boot logo
#define BOOT_LOGO_FADE_DURATION 500  // Durata fade-in in millisecondi
#define BOOT_LOGO_FADE_STEPS 3      // Numero di step per fade (più step = più fluido, ma più lento)

// Display Pin Configuration
// Configurazione basata su Factory_samples.ino del produttore ESP32-2424S012
// File: 1.28inch_ESP32-2424S012/1-Demo/Demo_Arduino/Factory_samples.ino
// Questi sono i pin ufficiali del display GC9A01 su questa board!

// Per disabilitare temporaneamente il display (per debug), decommenta:
// #define DISPLAY_ENABLED 0

// Pin configurazione dal file Factory_samples.ino del produttore
#define DISPLAY_CS_PIN 10  // cfg.pin_cs = 10
#define DISPLAY_DC_PIN 2   // cfg.pin_dc = 2
#define DISPLAY_RST_PIN -1 // cfg.pin_rst = -1 (non usato, ma proviamo 8 se necessario)
#define DISPLAY_BL_PIN 3   // pinMode(3, OUTPUT); digitalWrite(3, HIGH);
// NOTA: MOSI/SCK non sono piu' passati al costruttore del display (che userebbe
// SPI software/bit-bang). Coincidono con i pin SPI hardware di default
// dell'ESP32-C3 e restano qui solo a scopo di documentazione.
// Touchscreen CST816S (I2C)
#define TOUCH_SDA_PIN 4
#define TOUCH_SCL_PIN 5
#define TOUCH_I2C_ADDR 0x15

// Backlight PWM
#define BACKLIGHT_PWM_FREQ 5000
#define BACKLIGHT_PWM_RESOLUTION 8
#define BACKLIGHT_DEFAULT_PERCENT 80

#define DISPLAY_MOSI_PIN 7 // cfg.pin_mosi = 7 (= SPI hardware default)
#define DISPLAY_SCK_PIN 6  // cfg.pin_sclk = 6 (= SPI hardware default)

// Colori Display (RGB565)
#define COLOR_BLACK 0x0000
#define COLOR_WHITE 0xFFFF
#define COLOR_RED 0xC0C7
#define COLOR_GREEN 0x2D28
#define COLOR_BLUE 0x03DF
#define COLOR_YELLOW 0xD500
#define COLOR_GRAY 0x8410
#define COLOR_DARK_GRAY 0x4208
#define COLOR_LIGHT_GRAY 0xC618

// Colori MicroNav (RGB565)
#define COLOR_MICRONAV_RED 0xC0C7      // Rosso principale
#define COLOR_MICRONAV_RED_20 0x31E7   // Rosso con trasparenza ~20% (approssimato)
#define COLOR_MICRONAV_ORANGE 0xBA80   // Arancio scuro (189,81,0): testo bianco leggibile (contrasto ~4,8:1)

// Serial Debug
#define SERIAL_DEBUG_BAUD 115200
#define DEBUG_ENABLED true

// Memory Configuration
// ESP32-C3 ha ~400KB RAM totale, ogni Speedcam è ~24 bytes
// 2000 speedcam = ~48KB (ragionevole per ESP32-C3)
#define MAX_SPEEDCAM_COUNT 2000  // Numero massimo speedcam in memoria (~48 KB)
#define SPEEDCAM_PRE_FILTER_ENABLED true  // Abilita pre-filtraggio geografico

// Pre-filtraggio geografico: box centrato sulla posizione reale, ricalcolato periodicamente.
// Senza fix GPS si usa il centro di ripiego (Milano) finche' non arriva la prima posizione.
#define PRE_FILTER_HALF_SPAN_DEG 0.5       // Semi-lato del box in gradi (~55 km in latitudine)
#define PRE_FILTER_FALLBACK_LAT 45.46      // Centro di ripiego senza fix (Milano)
#define PRE_FILTER_FALLBACK_LNG 9.19
#define SPEEDCAM_DB_REFRESH_MS 1800000UL   // Ricalcolo del box e ricarica del database (30 minuti)

// Timing
#define MAIN_LOOP_DELAY 5  // Delay loop principale in millisecondi (basso: serve a LVGL/touch)
#define GPS_UPDATE_INTERVAL 1000  // Intervallo aggiornamento GPS in millisecondi

#endif // CONFIG_H
