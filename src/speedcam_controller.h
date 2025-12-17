#ifndef SPEEDCAM_CONTROLLER_H
#define SPEEDCAM_CONTROLLER_H

#include <Arduino.h>
#include "gps_controller.h"
#include "json_parser.h"
#include "utils.h"
#include "config.h"

// Forward declaration
class DisplayController;

/**
 * Controller per rilevazione speedcam
 * Gestisce il database speedcam e la detection basata su posizione GPS
 */
class SpeedcamController {
public:
    SpeedcamController();
    ~SpeedcamController();
    
    /**
     * Inizializza il controller
     * @param gps_controller Puntatore a GPS controller
     * @param display_controller Puntatore a display controller (opzionale)
     * @return true se inizializzazione riuscita
     */
    bool begin(GPSController* gps_controller, DisplayController* display_controller = nullptr);
    
    /**
     * Carica database speedcam da file JSON
     * @param filename Nome file JSON su LittleFS
     * @param pre_filter Abilita pre-filtraggio geografico (bounding box)
     * @param min_lat Latitudine minima per pre-filtro (opzionale)
     * @param max_lat Latitudine massima per pre-filtro (opzionale)
     * @param min_lng Longitudine minima per pre-filtro (opzionale)
     * @param max_lng Longitudine massima per pre-filtro (opzionale)
     * @return true se caricamento riuscito
     */
    bool loadDatabase(const char* filename, bool pre_filter = false,
                     float min_lat = 0, float max_lat = 0,
                     float min_lng = 0, float max_lng = 0);
    
    /**
     * Verifica speedcam vicine basandosi sulla posizione GPS
     * @param position Posizione GPS (opzionale, se nullptr usa GPS controller)
     * @return Puntatore a speedcam rilevata, nullptr se nessuna trovata
     */
    const Speedcam* checkSpeedcams(const GPSPosition* position = nullptr);
    
    /**
     * Abilita/disabilita rilevazione speedcam
     */
    void setEnabled(bool enabled);
    bool isEnabled() const;
    
    /**
     * Imposta raggio di rilevazione
     * @param radius Raggio in metri
     */
    void setDetectionRadius(float radius);
    float getDetectionRadius() const;
    
    /**
     * Imposta intervallo tra check
     * @param interval Intervallo in millisecondi
     */
    void setCheckInterval(unsigned long interval);
    
    /**
     * Ottiene statistiche
     */
    struct Stats {
        unsigned long detections_count;
        unsigned long last_detection_time;
        unsigned long checks_count;
    };
    Stats getStats() const;
    
    /**
     * Reset statistiche
     */
    void resetStats();

private:
    GPSController* gps_controller;
    DisplayController* display_controller;
    
    // Database speedcam
    Speedcam* speedcams;
    int speedcam_count;
    int max_speedcam_count;
    
    // Configurazione
    bool enabled;
    float detection_radius;
    unsigned long check_interval;
    
    // Stato rilevazione (anti-duplicati)
    uint32_t last_detected_speedcam_id;
    float last_detected_distance;
    float previous_detected_distance;  // Distanza precedente per rilevare allontanamento
    unsigned long last_check_time;
    
    // Statistiche
    Stats stats;
    
    /**
     * Rileva speedcam entro raggio dalla posizione GPS
     * @param position Posizione GPS
     * @param radius Raggio in metri
     * @return Puntatore a speedcam più vicina, nullptr se nessuna trovata
     */
    const Speedcam* detectSpeedcam(const GPSPosition& position, float radius);
    
    /**
     * Notifica rilevazione speedcam
     */
    void notifySpeedcamDetected(const Speedcam& speedcam, float distance);
};

#endif // SPEEDCAM_CONTROLLER_H
