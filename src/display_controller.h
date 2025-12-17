#ifndef DISPLAY_CONTROLLER_H
#define DISPLAY_CONTROLLER_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include <SPI.h>
#include <LittleFS.h>
#include "config.h"

// Forward declaration
struct Speedcam;

/**
 * Controller display GC9A01 240x240
 * Gestisce visualizzazione boot logo, schermata idle e alert speedcam
 */
class DisplayController {
public:
    DisplayController();
    ~DisplayController();
    
    /**
     * Inizializza il display
     * @return true se inizializzazione riuscita
     */
    bool begin();
    
    /**
     * Mostra boot logo
     * @param display_time_ms Tempo visualizzazione in millisecondi
     */
    void showBootLogo(unsigned long display_time_ms = BOOT_LOGO_DISPLAY_TIME);
    
    /**
     * Mostra schermata idle (GPS status, ecc.)
     */
    void showIdleScreen();
    
    /**
     * Mostra alert speedcam
     * @param speedcam Dati speedcam rilevata
     * @param distance Distanza dalla speedcam in metri
     */
    void showSpeedcamAlert(const struct Speedcam& speedcam, float distance);
    
    /**
     * Nasconde alert speedcam e torna a schermata idle
     */
    void hideSpeedcamAlert();
    
    /**
     * Aggiorna display (da chiamare periodicamente)
     */
    void update();
    
    /**
     * Aggiorna indicatore GPS
     */
    void updateGPSIndicator(bool has_fix, uint8_t satellites);

private:
    Adafruit_GC9A01A* display;
    bool is_initialized;
    
    // Stato corrente
    bool showing_alert;
    unsigned long alert_start_time;
    unsigned long alert_display_time;
    
    // GPS status
    bool gps_has_fix;
    uint8_t gps_satellites;
    
    /**
     * Disegna contenuto alert speedcam
     */
    void drawSpeedcamAlertContent(const Speedcam& speedcam, float distance);
    
    /**
     * Disegna informazioni GPS (testo, senza re-render completo)
     */
    void drawGPSInfo();
    
    /**
     * Disegna indicatore GPS
     */
    void drawGPSIndicator(bool has_fix, uint8_t satellites);
    
    /**
     * Disegna rounded rectangle (simulazione, Adafruit GFX non ha rounded rect nativo)
     */
    void drawRoundedRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t radius, uint16_t color);
    
    /**
     * Disegna rounded rectangle filled
     */
    void drawRoundedRectFilled(int16_t x, int16_t y, int16_t w, int16_t h, int16_t radius, uint16_t color);
    
    /**
     * Disegna cerchio con bordo
     */
    void drawCircleWithBorder(int16_t x, int16_t y, int16_t radius, uint16_t fill_color, uint16_t border_color, int16_t border_width);
    
    /**
     * Ottiene colore da RGB
     */
    uint16_t color565(uint8_t r, uint8_t g, uint8_t b);
    
    /**
     * Mescola colore RGB565 con nero per effetto fade
     * @param color Colore originale (RGB565)
     * @param fade_factor Fattore fade (0.0 = nero, 1.0 = colore pieno)
     * @return Colore attenuato
     */
    uint16_t fadeColor565(uint16_t color, float fade_factor);
};

#endif // DISPLAY_CONTROLLER_H
