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
    
    Speedcam() : 
        id(0), 
        lat(0.0), 
        lng(0.0), 
        status(' '), 
        art(' ') {
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
     * Verifica se LittleFS è inizializzato
     */
    static bool isLittleFSMounted();

private:
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
