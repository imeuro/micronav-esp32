#ifndef ROAD_LOG_H
#define ROAD_LOG_H

#include <Arduino.h>
#include "config.h"

/**
 * Log degli eventi significativi su LittleFS, per analizzare le prove su strada.
 * Scrive in un buffer RAM e lo svuota su flash ogni ROAD_LOG_FLUSH_MS (o su richiesta).
 * Ruota a ROAD_LOG_MAX_BYTES: file corrente + file .old. Si ferma sotto
 * ROAD_LOG_MIN_FREE_BYTES di spazio libero, per non toccare il database.
 */
namespace RoadLog {
    // Da chiamare dopo il mount di LittleFS
    void begin();

    // Aggiunge una riga (printf-like, senza newline finale)
    void event(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

    // Svuota il buffer su flash
    void flush();

    // Da chiamare nel loop: flush periodico
    void update();

    // Stampa i file di log su s tra marcatori ---ROADLOG-BEGIN--- / ---ROADLOG-END---
    void dump(Print& s);

    // Cancella i file di log
    void clear();
}

#endif // ROAD_LOG_H
