#ifndef SPEEDCAM_CONTROLLER_H
#define SPEEDCAM_CONTROLLER_H

#include <Arduino.h>
#include "gps_controller.h"
#include "json_parser.h"
#include "utils.h"
#include "config.h"
#include "speedcam_tracker.h"

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
     * Carica il database filtrando per un box di PRE_FILTER_HALF_SPAN_DEG attorno a (lat, lng).
     * @param from_fix true se il centro e' una posizione GPS reale; false se e' il ripiego,
     *                 nel qual caso al primo fix valido il database viene ricaricato
     */
    bool loadDatabaseAround(double lat, double lng, bool from_fix);

    /**
     * Verifica speedcam vicine basandosi su posizione e direzione di marcia.
     * Mostra l'alert per il POI piu' vicino davanti a noi, lo nasconde al passaggio.
     * @param position Posizione GPS (opzionale, se nullptr usa GPS controller)
     * @return Puntatore a speedcam da segnalare, nullptr se nessuna
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
    
    // Stato rilevazione: state machine CANDIDATE/PASSED del POI tracciato
    SpeedcamTracker tracker;
    float last_detected_distance;
    unsigned long last_check_time;

    // Area del database
    bool db_from_fix;               // il box attuale e' centrato su una posizione reale
    unsigned long last_db_load_ms;

    /**
     * Ricarica il database attorno alla posizione se serve (primo fix o scaduti i 30 minuti).
     * Salta finche' c'e' un POI tracciato, per non bloccare il display durante un alert.
     */
    void refreshDatabaseIfDue(const GPSPosition& position);
    
    // Statistiche
    Stats stats;
    
    /**
     * Notifica rilevazione speedcam
     */
    void notifySpeedcamDetected(const Speedcam& speedcam, float distance);
};

#endif // SPEEDCAM_CONTROLLER_H
