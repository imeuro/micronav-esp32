#ifndef CONFIG_H
#define CONFIG_H

// Compatibilità Arduino IDE
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

// ============================================
// Configurazione Hardware
// ============================================

// GPS Configuration
#define GPS_SERIAL_BAUD 9600
#define GPS_SERIAL_RX_PIN 4    // Da verificare pin UART disponibili ESP32-C3
#define GPS_SERIAL_TX_PIN 5    // Da verificare pin UART disponibili ESP32-C3
#define GPS_FIX_TIMEOUT 45000  // 45 secondi in millisecondi
#define GPS_MIN_SATELLITES 4
#define GPS_MAX_HDOP 5.0

// GPS Fake Mode (per test senza GPS hardware)
// Imposta a true per usare coordinate fake da file JSON invece del GPS reale
#define GPS_FAKE_MODE true
#define GPS_FAKE_JSON_PATH "/fake_gps.json"  // Path file JSON con coordinate fake
#define GPS_FAKE_UPDATE_INTERVAL 2000  // Intervallo aggiornamento posizione fake in millisecondi

// Speedcam Configuration
#define SPEEDCAM_DETECTION_RADIUS 1000  // Raggio di rilevazione in metri (default 1km)
#define SPEEDCAM_CHECK_INTERVAL 5000    // Intervallo tra check in millisecondi (default 5 secondi)
#define SPEEDCAM_JSON_PATH "/speedcams.json"
#define SPEEDCAM_ENABLED true

// Display Configuration (GC9A01 240x240 onboard)
#define DISPLAY_WIDTH 240
#define DISPLAY_HEIGHT 240
#define DISPLAY_DRIVER_GC9A01
#define BOOT_LOGO_DISPLAY_TIME 3000  // Tempo visualizzazione boot logo in millisecondi
#define BOOT_LOGO_FADE_ENABLED true  // Abilita fade-in del boot logo
#define BOOT_LOGO_FADE_DURATION 500  // Durata fade-in in millisecondi
#define BOOT_LOGO_FADE_STEPS 12      // Numero di step per fade (più step = più fluido, ma più lento)

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
#define DISPLAY_MOSI_PIN 7 // cfg.pin_mosi = 7
#define DISPLAY_SCK_PIN 6  // cfg.pin_sclk = 6

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

// Serial Debug
#define SERIAL_DEBUG_BAUD 115200
#define DEBUG_ENABLED true

// Memory Configuration
// ESP32-C3 ha ~400KB RAM totale, ogni Speedcam è ~24 bytes
// 2000 speedcam = ~48KB (ragionevole per ESP32-C3)
#define MAX_SPEEDCAM_COUNT 2000  // Numero massimo speedcam in memoria
#define SPEEDCAM_PRE_FILTER_ENABLED true  // Abilita pre-filtraggio geografico

// Pre-filtraggio geografico (bounding box Italia settentrionale)
// Utile per ridurre numero speedcam caricate in memoria
#define PRE_FILTER_MIN_LAT 43.0   // Latitudine minima
#define PRE_FILTER_MAX_LAT 47.0   // Latitudine massima
#define PRE_FILTER_MIN_LNG 6.0    // Longitudine minima
#define PRE_FILTER_MAX_LNG 14.0   // Longitudine massima

// Timing
#define MAIN_LOOP_DELAY 100  // Delay loop principale in millisecondi
#define GPS_UPDATE_INTERVAL 1000  // Intervallo aggiornamento GPS in millisecondi

#endif // CONFIG_H
