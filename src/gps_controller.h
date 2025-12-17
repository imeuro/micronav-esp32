#ifndef GPS_CONTROLLER_H
#define GPS_CONTROLLER_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include <TinyGPSPlus.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "config.h"

/**
 * Struttura dati posizione GPS
 */
struct GPSPosition {
    double latitude;
    double longitude;
    double altitude;
    float speed;          // Velocità in km/h
    float course;         // Direzione in gradi
    uint8_t satellites;
    float hdop;           // Horizontal Dilution of Precision
    bool is_valid;
    unsigned long last_update;
    
    GPSPosition() : 
        latitude(0.0), 
        longitude(0.0), 
        altitude(0.0),
        speed(0.0),
        course(0.0),
        satellites(0),
        hdop(0.0),
        is_valid(false),
        last_update(0) {}
};

/**
 * Enum stati GPS
 */
enum GPSStatus {
    GPS_DISCONNECTED,
    GPS_CONNECTING,
    GPS_CONNECTED,
    GPS_FIXING,
    GPS_FIXED,
    GPS_ERROR
};

/**
 * Controller GPS per modulo ATGM336H
 * Compatibile con protocollo NMEA (stesso formato L76K)
 */
class GPSController {
public:
    GPSController();
    ~GPSController();
    
    /**
     * Inizializza il controller GPS
     * @param rx_pin Pin RX per seriale GPS
     * @param tx_pin Pin TX per seriale GPS (opzionale, -1 se non usato)
     * @return true se inizializzazione riuscita
     */
    bool begin(int rx_pin = GPS_SERIAL_RX_PIN, int tx_pin = GPS_SERIAL_TX_PIN);
    
    /**
     * Inizializza il controller GPS in modalità fake (per test)
     * Carica coordinate da file JSON su LittleFS
     * @param json_path Path del file JSON con percorso fake
     * @return true se inizializzazione riuscita
     */
    bool beginFake(const char* json_path = GPS_FAKE_JSON_PATH);
    
    /**
     * Aggiorna il controller (da chiamare nel loop)
     * Legge dati dalla seriale e aggiorna posizione
     */
    void update();
    
    /**
     * Ottiene la posizione GPS corrente
     * @return Struttura GPSPosition con dati aggiornati
     */
    GPSPosition getPosition() const;
    
    /**
     * Verifica se il GPS ha un fix valido
     * @return true se fix valido
     */
    bool hasFix() const;
    
    /**
     * Ottiene lo stato corrente del GPS
     * @return GPSStatus
     */
    GPSStatus getStatus() const;
    
    /**
     * Attende fino a ottenere un fix GPS
     * @param timeout_ms Timeout in millisecondi
     * @return true se fix ottenuto entro timeout
     */
    bool waitForFix(unsigned long timeout_ms = GPS_FIX_TIMEOUT);
    
    /**
     * Imposta callback per aggiornamento posizione
     * La callback viene chiamata quando la posizione viene aggiornata
     */
    void setPositionUpdateCallback(void (*callback)(const GPSPosition&));
    
    /**
     * Ottiene statistiche GPS
     */
    struct Stats {
        unsigned long sentences_received;
        unsigned long valid_sentences;
        unsigned long fix_attempts;
        unsigned long last_fix_time;
    };
    Stats getStats() const;

private:
    HardwareSerial* gps_serial;
    TinyGPSPlus gps_parser;
    GPSPosition current_position;
    GPSStatus status;
    
    // Modalità fake
    bool fake_mode;
    struct FakeRoutePoint {
        double lat;
        double lng;
        double altitude;
        float speed;
        float course;
        uint8_t satellites;
        float hdop;
    };
    FakeRoutePoint* fake_route;
    int fake_route_count;
    int fake_route_index;
    bool fake_route_loop;
    unsigned long fake_last_update;
    
    // Callback
    void (*position_update_callback)(const GPSPosition&);
    
    // Statistiche
    Stats stats;
    
    /**
     * Aggiorna la posizione corrente dai dati TinyGPS++
     */
    void updatePosition();
    
    /**
     * Aggiorna lo stato del GPS
     */
    void updateStatus();
    
    /**
     * Carica percorso fake da file JSON
     */
    bool loadFakeRoute(const char* json_path);
    
    /**
     * Aggiorna posizione in modalità fake
     */
    void updateFakePosition();
};

#endif // GPS_CONTROLLER_H
