#include "speedcam_controller.h"
#include "display_controller.h"

SpeedcamController::SpeedcamController() :
    gps_controller(nullptr),
    display_controller(nullptr),
    speedcams(nullptr),
    speedcam_count(0),
    max_speedcam_count(MAX_SPEEDCAM_COUNT),
    enabled(SPEEDCAM_ENABLED),
    detection_radius(SPEEDCAM_DETECTION_RADIUS),
    check_interval(SPEEDCAM_CHECK_INTERVAL),
    last_detected_speedcam_id(0),
    last_detected_distance(0.0),
    previous_detected_distance(0.0),
    last_check_time(0) {
    
    stats.detections_count = 0;
    stats.last_detection_time = 0;
    stats.checks_count = 0;
    
    // Alloca array speedcam
    // Ogni Speedcam è ~24 bytes, quindi 2000 speedcam = ~48KB
    // NOTA: Non usare Serial qui - viene chiamato prima che Serial.begin() sia eseguito
    speedcams = new Speedcam[max_speedcam_count];
    if (!speedcams) {
        // Allocazione fallita - imposta max a 0
        max_speedcam_count = 0;
    }
}

SpeedcamController::~SpeedcamController() {
    if (speedcams) {
        delete[] speedcams;
    }
}

bool SpeedcamController::begin(GPSController* gps_controller, DisplayController* display_controller) {
    this->gps_controller = gps_controller;
    this->display_controller = display_controller;
    
    // Messaggio di debug dopo che Serial è inizializzato
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.print("[Speedcam] Allocazione memoria: ");
        Serial.print(max_speedcam_count);
        Serial.print(" speedcam (~");
        Serial.print((max_speedcam_count * sizeof(Speedcam)) / 1024);
        Serial.println(" KB)");
        if (!speedcams) {
            Serial.println("[Speedcam] ERRORE: Memoria non allocata!");
        } else {
            Serial.println("[Speedcam] Memoria allocata con successo");
        }
    }
    #endif
    
    if (!gps_controller) {
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            Serial.println("[Speedcam] GPS controller non valido!");
        }
        #endif
        return false;
    }
    
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.println("[Speedcam] Controller inizializzato");
        Serial.print("[Speedcam] Memoria allocata per ");
        Serial.print(max_speedcam_count);
        Serial.println(" speedcam");
    }
    #endif
    
    return true;
}

bool SpeedcamController::loadDatabase(const char* filename, bool pre_filter,
                                     float min_lat, float max_lat,
                                     float min_lng, float max_lng) {
    if (!speedcams) {
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            Serial.println("[Speedcam] Array speedcam non allocato!");
        }
        #endif
        return false;
    }
    
    JSONParser parser;
    int loaded = parser.loadFromFile(filename, speedcams, max_speedcam_count);
    
    if (loaded < 0) {
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            Serial.println("[Speedcam] Errore caricamento database");
        }
        #endif
        return false;
    }
    
    speedcam_count = loaded;
    
    // Pre-filtraggio geografico se abilitato
    if (pre_filter && SPEEDCAM_PRE_FILTER_ENABLED && 
        min_lat != 0 && max_lat != 0 && min_lng != 0 && max_lng != 0) {
        int filtered = parser.filterByBoundingBox(speedcams, speedcam_count,
                                                  min_lat, max_lat, min_lng, max_lng);
        
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            Serial.print("[Speedcam] Pre-filtraggio geografico: ");
            Serial.print(speedcam_count);
            Serial.print(" -> ");
            Serial.print(filtered);
            Serial.println(" speedcam");
        }
        #endif
        
        speedcam_count = filtered;
    }
    
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.print("[Speedcam] Database caricato: ");
        Serial.print(speedcam_count);
        Serial.println(" speedcam");
        
        // Mostra prime 5 speedcam per debug
        if (speedcam_count > 0) {
            Serial.println("[Speedcam] Prime speedcam caricate:");
            int show_count = min(5, speedcam_count);
            for (int i = 0; i < show_count; i++) {
                Serial.print("  [");
                Serial.print(i);
                Serial.print("] ID: ");
                Serial.print(speedcams[i].id);
                Serial.print(", Lat: ");
                Serial.print(speedcams[i].lat, 6);
                Serial.print(", Lng: ");
                Serial.print(speedcams[i].lng, 6);
                Serial.print(", Tipo: ");
                Serial.println(speedcams[i].type);
            }
        }
    }
    #endif
    
    return true;
}

const Speedcam* SpeedcamController::checkSpeedcams(const GPSPosition* position) {
    // Throttling: esegui check solo ogni X millisecondi
    unsigned long current_time = millis();
    if (last_check_time > 0 && (current_time - last_check_time) < check_interval) {
        return nullptr;  // Troppo presto, salta il check
    }
    
    if (!enabled) {
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            static unsigned long last_warn = 0;
            if (millis() - last_warn > 10000) {
                Serial.println("[Speedcam] Controller disabilitato!");
                last_warn = millis();
            }
        }
        #endif
        return nullptr;
    }
    
    if (!gps_controller) {
        return nullptr;
    }
    
    // Ottieni posizione GPS
    GPSPosition gps_position;
    if (position) {
        gps_position = *position;
    } else {
        gps_position = gps_controller->getPosition();
    }
    
    // Verifica validità posizione
    if (!gps_position.is_valid) {
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            static unsigned long last_warn = 0;
            if (millis() - last_warn > 5000) {
                Serial.println("[Speedcam] Posizione GPS non valida!");
                last_warn = millis();
            }
        }
        #endif
        return nullptr;
    }
    
    // Aggiorna tempo ultimo check
    last_check_time = current_time;
    stats.checks_count++;
    
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        static unsigned long last_check_debug = 0;
        if (millis() - last_check_debug > 5000) {
            Serial.print("[Speedcam] Check eseguito - Posizione valida, Database: ");
            Serial.print(speedcam_count);
            Serial.println(" speedcam");
            last_check_debug = millis();
        }
    }
    #endif
    
    // Rileva speedcam vicine
    const Speedcam* detected = detectSpeedcam(gps_position, detection_radius);
    
    if (detected) {
        notifySpeedcamDetected(*detected, last_detected_distance);
    } else {
        // Nessuna speedcam rilevata: se c'era un alert attivo, nascondilo
        // Solo se la speedcam precedente non è più nel raggio
        if (last_detected_speedcam_id != 0 && display_controller) {
            // Verifica se la speedcam precedente è ancora nel raggio
            bool still_in_range = false;
            for (int i = 0; i < speedcam_count; i++) {
                if (speedcams[i].id == last_detected_speedcam_id) {
                    float distance = calculate_distance(
                        gps_position.latitude,
                        gps_position.longitude,
                        speedcams[i].lat,
                        speedcams[i].lng
                    );
                    if (distance <= detection_radius) {
                        still_in_range = true;
                    }
                    break;
                }
            }
            
            // Se non è più nel raggio, nascondi alert e resetta tracking
            if (!still_in_range) {
                #ifdef DEBUG_ENABLED
                if (DEBUG_ENABLED) {
                    Serial.println("[Speedcam] Speedcam uscita dal raggio, nascondo alert");
                }
                #endif
                display_controller->hideSpeedcamAlert();
                last_detected_speedcam_id = 0;
                last_detected_distance = 0.0;
                previous_detected_distance = 0.0;
            }
        }
    }
    
    return detected;
}

const Speedcam* SpeedcamController::detectSpeedcam(const GPSPosition& position, float radius) {
    if (speedcam_count == 0) {
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            Serial.println("[Speedcam] detectSpeedcam: Nessuna speedcam nel database!");
        }
        #endif
        return nullptr;
    }
    
    const Speedcam* closest_speedcam = nullptr;
    float closest_distance = radius + 1.0;  // Inizia oltre il raggio
    
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        static unsigned long last_debug_time = 0;
        if (millis() - last_debug_time > 5000) {  // Debug ogni 5 secondi
            Serial.print("[Speedcam] Check posizione: ");
            Serial.print(position.latitude, 6);
            Serial.print(", ");
            Serial.print(position.longitude, 6);
            Serial.print(" | Database: ");
            Serial.print(speedcam_count);
            Serial.print(" speedcam | Raggio: ");
            Serial.print(radius);
            Serial.println("m");
            last_debug_time = millis();
        }
    }
    #endif
    
    // Calcola distanza da tutte le speedcam e trova la più vicina
    for (int i = 0; i < speedcam_count; i++) {
        const Speedcam& sc = speedcams[i];
        
        // Verifica che le coordinate siano valide
        if (!is_valid_float(sc.lat) || !is_valid_float(sc.lng)) {
            continue;
        }
        
        // Calcola distanza usando formula Haversine
        float distance = calculate_distance(
            position.latitude,
            position.longitude,
            sc.lat,
            sc.lng
        );
        
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            // Log solo le speedcam vicine (entro 2km per debug)
            if (distance < 2000.0) {
                static unsigned long last_near_debug = 0;
                if (millis() - last_near_debug > 2000) {  // Ogni 2 secondi
                    Serial.print("[Speedcam] Speedcam vicina - ID: ");
                    Serial.print(sc.id);
                    Serial.print(", Distanza: ");
                    Serial.print((int)distance);
                    Serial.print("m, Tipo: ");
                    Serial.println(sc.type);
                    last_near_debug = millis();
                }
            }
        }
        #endif
        
        // Verifica se è entro raggio e più vicina
        if (distance <= radius && distance < closest_distance) {
            closest_distance = distance;
            closest_speedcam = &sc;
        }
    }
    
    // Verifica se ci stiamo avvicinando o allontanando
    if (closest_speedcam) {
        // Se è la stessa speedcam, verifica se ci stiamo avvicinando o allontanando
        if (closest_speedcam->id == last_detected_speedcam_id && previous_detected_distance > 0.0) {
            // Calcola differenza distanza (positiva = allontanamento, negativa = avvicinamento)
            float distance_change = closest_distance - previous_detected_distance;
            
            // Se ci stiamo allontanando (distanza aumenta), nascondi alert e non notificare
            if (distance_change > 10.0) {  // Soglia 10m per evitare oscillazioni
                #ifdef DEBUG_ENABLED
                if (DEBUG_ENABLED) {
                    Serial.print("[Speedcam] Allontanamento rilevato - Distanza: ");
                    Serial.print((int)closest_distance);
                    Serial.print("m (precedente: ");
                    Serial.print((int)previous_detected_distance);
                    Serial.println("m), nascondo alert");
                }
                #endif
                
                // Nascondi alert se presente
                if (display_controller) {
                    display_controller->hideSpeedcamAlert();
                }
                
                // Se ci siamo allontanati oltre il raggio, resetta tracking
                if (closest_distance > radius) {
                    last_detected_speedcam_id = 0;
                    last_detected_distance = 0.0;
                    previous_detected_distance = 0.0;
                } else {
                    // Aggiorna distanza ma non notificare
                    previous_detected_distance = closest_distance;
                }
                
                return nullptr;
            }
            
            // Se distanza cambiata meno di 10m, ignora (evita notifiche continue)
            if (abs(distance_change) < 10.0) {
                // Aggiorna comunque la distanza per il prossimo check
                previous_detected_distance = closest_distance;
                return nullptr;
            }
        } else {
            // Nuova speedcam rilevata o prima rilevazione, imposta distanza precedente
            previous_detected_distance = closest_distance;
        }
        
        // Aggiorna tracking
        last_detected_speedcam_id = closest_speedcam->id;
        last_detected_distance = closest_distance;
    }
    
    return closest_speedcam;
}

void SpeedcamController::notifySpeedcamDetected(const Speedcam& speedcam, float distance) {
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.print("[Speedcam] 🚨 Speedcam rilevata - ID: ");
        Serial.print(speedcam.id);
        Serial.print(", Tipo: ");
        Serial.print(speedcam.type);
        Serial.print(", Limite: ");
        Serial.print(speedcam.vmax);
        Serial.print(" km/h, Distanza: ");
        Serial.print((int)distance);
        Serial.println("m");
    }
    #endif
    
    // Aggiorna statistiche
    stats.detections_count++;
    stats.last_detection_time = millis();
    
    // Visualizza alert su display
    if (display_controller) {
        display_controller->showSpeedcamAlert(speedcam, distance);
    }
}

void SpeedcamController::setEnabled(bool enabled) {
    this->enabled = enabled;
}

bool SpeedcamController::isEnabled() const {
    return enabled;
}

void SpeedcamController::setDetectionRadius(float radius) {
    this->detection_radius = radius;
}

float SpeedcamController::getDetectionRadius() const {
    return detection_radius;
}

void SpeedcamController::setCheckInterval(unsigned long interval) {
    this->check_interval = interval;
}

SpeedcamController::Stats SpeedcamController::getStats() const {
    return stats;
}

void SpeedcamController::resetStats() {
    stats.detections_count = 0;
    stats.last_detection_time = 0;
    stats.checks_count = 0;
}
