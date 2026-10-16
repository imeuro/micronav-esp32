#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>
#include "config.h"

/**
 * Struttura dati speedcam
 * Ottimizzata per memoria limitata ESP32
 */
struct Speedcam {
    uint32_t id;
    float lat;
    float lng;
    char type[4];        // "G50", "A", "BK", ecc.
    char vmax[4];        // "50", "/", ecc.
    char status;         // 'A' (attivo) o 'L' (inattivo)
    char art;            // Tipo: 'G', 'A', 'BK', ecc.
    uint16_t dir;        // Direzione controllata in gradi ("richtung"), SPEEDCAM_DIR_NONE se assente
    
    Speedcam() : 
        id(0), 
        lat(0.0), 
        lng(0.0), 
        status(' '), 
        art(' '),
        dir(SPEEDCAM_DIR_NONE) {
        type[0] = '\0';
        vmax[0] = '\0';
    }
};

/**
 * Parser JSON per database speedcam
 * Supporta caricamento da LittleFS o array statico
 */
class JSONParser {
public:
    /**
     * Callback chiamata periodicamente (~30 ms) durante il parsing di file grandi,
     * per mantenere reattivi display e touch mentre il database si carica
     */
    static void setProgressCallback(void (*cb)());

    JSONParser();
    ~JSONParser();
    
    /**
     * Carica database speedcam da file JSON su LittleFS
     * @param filename Nome file JSON (es. "/speedcams.json")
     * @param speedcams Array dove salvare le speedcam
     * @param max_count Dimensione massima array
     * @return Numero di speedcam caricate, -1 se errore
     */
    int loadFromFile(const char* filename, Speedcam* speedcams, int max_count);
    
    /**
     * Carica database speedcam da stringa JSON
     * @param json_string Stringa JSON
     * @param speedcams Array dove salvare le speedcam
     * @param max_count Dimensione massima array
     * @return Numero di speedcam caricate, -1 se errore
     */
    int loadFromString(const char* json_string, Speedcam* speedcams, int max_count);
    
    /**
     * Pre-filtra speedcam per bounding box geografico
     * Utile per ridurre il numero di speedcam da processare
     * @param speedcams Array speedcam
     * @param count Numero speedcam nell'array
     * @param min_lat Latitudine minima
     * @param max_lat Latitudine massima
     * @param min_lng Longitudine minima
     * @param max_lng Longitudine massima
     * @return Numero di speedcam dopo filtro
     */
    int filterByBoundingBox(Speedcam* speedcams, int count, 
                           float min_lat, float max_lat, 
                           float min_lng, float max_lng);
    
    /**
     * Imposta un bounding box applicato durante il caricamento, prima del limite
     * max_count (i POI fuori area non consumano posti nell'array)
     */
    void setBoundingBox(float min_lat, float max_lat, float min_lng, float max_lng);

    /** POI scartati nell'ultimo caricamento perche' fuori dal bounding box */
    int getDiscardedByBBox() const { return discarded_bbox; }

    /** POI validi (nell'area) scartati nell'ultimo caricamento per il limite max_count */
    int getDiscardedByCap() const { return discarded_cap; }

    /**
     * Verifica se LittleFS è inizializzato
     */
    static bool isLittleFSMounted();

private:
    bool bbox_enabled;
    float bbox_min_lat, bbox_max_lat, bbox_min_lng, bbox_max_lng;
    int discarded_bbox;
    int discarded_cap;

    /**
     * Applica bounding box e limite max_count; salva in speedcams[count++] se accettato
     */
    void acceptSpeedcam(const Speedcam& sc, Speedcam* speedcams, int& count, int max_count);

    /**
     * Parsa un oggetto speedcam dal JSON
     */
    bool parseSpeedcam(JsonObject obj, Speedcam& speedcam);
    
    /**
     * Copia stringa limitata (per evitare overflow)
     */
    void safeStringCopy(char* dest, const char* src, size_t max_len);
    
    /**
     * Carica speedcam da file usando parser incrementale (per file molto grandi)
     * Processa il JSON carattere per carattere estraendo solo i dati necessari
     */
    int loadFromFileIncremental(File& file, Speedcam* speedcams, int max_count);
};

#endif // JSON_PARSER_H
