#include "speedcam_controller.h"
#include "display_controller.h"
#include "road_log.h"

SpeedcamController::SpeedcamController() :
    gps_controller(nullptr),
    display_controller(nullptr),
    speedcams(nullptr),
    speedcam_count(0),
    max_speedcam_count(MAX_SPEEDCAM_COUNT),
    enabled(SPEEDCAM_ENABLED),
    detection_radius(SPEEDCAM_DETECTION_RADIUS),
    check_interval(SPEEDCAM_CHECK_INTERVAL),
    last_detected_distance(0.0),
    log_alert_id(0),
    log_last_track_ms(0),
    db_from_fix(false),
    last_db_load_ms(0),
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

    // Il bounding box e' applicato durante il parse, prima del limite max_speedcam_count:
    // il database non e' ordinato geograficamente e il cap tagliava la fascia sbagliata
    if (pre_filter && SPEEDCAM_PRE_FILTER_ENABLED &&
        min_lat != 0 && max_lat != 0 && min_lng != 0 && max_lng != 0) {
        parser.setBoundingBox(min_lat, max_lat, min_lng, max_lng);
    }

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
    // Il tracker lavora per id: sopravvive alla ricarica se il POI e' ancora nel box

    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.print("[Speedcam] Scartate per bounding box: ");
        Serial.print(parser.getDiscardedByBBox());
        Serial.print(", scartate per limite ");
        Serial.print(max_speedcam_count);
        Serial.print(": ");
        Serial.println(parser.getDiscardedByCap());
    }
    #endif

    if (parser.getDiscardedByCap() > 0) {
        Serial.print("[Speedcam] ATTENZIONE: ");
        Serial.print(parser.getDiscardedByCap());
        Serial.println(" POI nell'area scartati per MAX_SPEEDCAM_COUNT");
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

bool SpeedcamController::loadDatabaseAround(double lat, double lng, bool from_fix) {
    const float span = PRE_FILTER_HALF_SPAN_DEG;
    bool ok = loadDatabase(SPEEDCAM_JSON_PATH, true,
                           (float)lat - span, (float)lat + span,
                           (float)lng - span, (float)lng + span);
    db_from_fix = from_fix;
    last_db_load_ms = millis();
    RoadLog::event("D t=%lu centro=%.5f,%.5f fix=%d poi=%d ok=%d", millis(), lat, lng, from_fix ? 1 : 0, speedcam_count, ok ? 1 : 0);  // anche in caso di errore: niente retry a raffica
    return ok;
}

void SpeedcamController::refreshDatabaseIfDue(const GPSPosition& position) {
    if (tracker.isTracking()) return;
    if (db_from_fix && (millis() - last_db_load_ms) < SPEEDCAM_DB_REFRESH_MS) return;

    Serial.println("[Speedcam] Ricarico il database attorno alla posizione corrente");
    loadDatabaseAround(position.latitude, position.longitude, true);
}

#if SPEEDCAM_ROAD_TEST_LOG
// Log per la prova su strada: per i POI piu' vicini nel raggio mostra distanza,
// bearing, angolo rispetto alla marcia e scarto di direzione (con offset 0 e
// 180), cosi' la taratura di cono/soglie e di SPEEDCAM_DIR_OFFSET_DEG si fa
// leggendo il log invece che a occhio.
static void logRoadTest(const Speedcam* cams, int count, const GPSPosition& pos,
                        float radius, const TrackerResult& result) {
    static unsigned long last_heartbeat = 0;
    const int MAX_LOGGED = 3;
    int idx[MAX_LOGGED];
    float dist[MAX_LOGGED];
    int n = 0;

    for (int i = 0; i < count; i++) {
        float d = (float)geo_distance_m(pos.latitude, pos.longitude, cams[i].lat, cams[i].lng);
        if (d > radius) continue;
        int slot = n < MAX_LOGGED ? n : MAX_LOGGED - 1;
        if (n >= MAX_LOGGED && d >= dist[slot]) continue;
        while (slot > 0 && dist[slot - 1] > d) {
            idx[slot] = idx[slot - 1];
            dist[slot] = dist[slot - 1];
            slot--;
        }
        idx[slot] = i;
        dist[slot] = d;
        if (n < MAX_LOGGED) n++;
    }

    if (n == 0 && millis() - last_heartbeat < 10000) return;
    last_heartbeat = millis();

    Serial.printf("[RoadTest] t=%lu pos=%.6f,%.6f v=%.0fkm/h hdg=", millis(), pos.latitude, pos.longitude, pos.speed);
    if (pos.course_valid) Serial.printf("%d", (int)pos.course); else Serial.print("n/d");
    Serial.printf(" sat=%d r=%dm poi_nel_raggio=%d\n", pos.satellites, (int)radius, n);

    for (int k = 0; k < n; k++) {
        const Speedcam& sc = cams[idx[k]];
        double bearing = geo_bearing_deg(pos.latitude, pos.longitude, sc.lat, sc.lng);
        Serial.printf("[RoadTest]   id=%lu tipo=%s vmax=%s d=%dm brg=%d", (unsigned long)sc.id, sc.type, sc.vmax, (int)dist[k], (int)bearing);

        const char* verdict = "ok-non-scelto";
        if (pos.course_valid) {
            double cone = geo_angle_diff_deg(pos.course, bearing);
            Serial.printf(" cono=%d", (int)cone);
            if (sc.dir != SPEEDCAM_DIR_NONE) {
                double d0 = geo_angle_diff_deg(pos.course, (double)sc.dir);
                double d180 = geo_angle_diff_deg(pos.course, fmod((double)sc.dir + 180.0, 360.0));
                Serial.printf(" dir=%d dir_off0=%d dir_off180=%d", (int)sc.dir, (int)d0, (int)d180);
                double used = geo_angle_diff_deg(pos.course, fmod((double)sc.dir + SPEEDCAM_DIR_OFFSET_DEG, 360.0));
                if (used > SPEEDCAM_DIR_TOLERANCE_DEG) verdict = "scartato-direzione";
            } else {
                Serial.print(" dir=n/d");
            }
            if (verdict[0] == 'o' && dist[k] > SPEEDCAM_NEAR_FIELD_M && cone > SPEEDCAM_CONE_HALF_ANGLE_DEG) {
                verdict = "scartato-cono";
            }
        } else {
            verdict = "scartato-heading-ignoto";
        }
        if (result.index >= 0 && cams[result.index].id == sc.id) verdict = "ALERT";
        Serial.printf(" -> %s\n", verdict);
    }
}
#endif

// Eventi significativi su LittleFS: inizio alert (con geometria), avanzamento, fine alert
void SpeedcamController::logRoadEvents(const GPSPosition& pos, double eval_lat, double eval_lng,
                                       const TrackerResult& result) {
    if (result.hide_reason != HIDE_NONE) {
        static const char* reasons[] = {"", "dietro", "allontanamento", "poi-sparito", "fuori-cono", "sostituito"};
        RoadLog::event("H t=%lu id=%lu motivo=%s pos=%.6f,%.6f v=%.0f",
                       millis(), (unsigned long)result.hide_id, reasons[result.hide_reason],
                       pos.latitude, pos.longitude, pos.speed);
        RoadLog::flush();
    }

    if (result.index < 0) {
        log_alert_id = 0;
        return;
    }

    const Speedcam& sc = speedcams[result.index];
    double bearing = geo_bearing_deg(eval_lat, eval_lng, sc.lat, sc.lng);

    if (sc.id != log_alert_id) {
        log_alert_id = sc.id;
        log_last_track_ms = millis();
        RoadLog::event("A t=%lu id=%lu tipo=%s vmax=%s d=%d brg=%d pos=%.6f,%.6f v=%.0f hdg=%d sat=%d poi=%.6f,%.6f",
                       millis(), (unsigned long)sc.id, sc.type, sc.vmax, (int)result.distance, (int)bearing,
                       pos.latitude, pos.longitude, pos.speed, pos.course_valid ? (int)pos.course : -1,
                       pos.satellites, sc.lat, sc.lng);
        if (pos.course_valid) {
            RoadLog::event("A+ id=%lu cono=%d dir=%d dir_off0=%d dir_off180=%d eta_fix=%lu",
                           (unsigned long)sc.id, (int)geo_angle_diff_deg(pos.course, bearing),
                           sc.dir == SPEEDCAM_DIR_NONE ? -1 : (int)sc.dir,
                           sc.dir == SPEEDCAM_DIR_NONE ? -1 : (int)geo_angle_diff_deg(pos.course, sc.dir),
                           sc.dir == SPEEDCAM_DIR_NONE ? -1 : (int)geo_angle_diff_deg(pos.course, fmod(sc.dir + 180.0, 360.0)),
                           millis() - pos.last_update);
        }
    } else if (millis() - log_last_track_ms >= ROAD_LOG_TRACK_INTERVAL_MS) {
        log_last_track_ms = millis();
        RoadLog::event("T t=%lu id=%lu d=%d brg=%d v=%.0f hdg=%d", millis(), (unsigned long)sc.id,
                       (int)result.distance, (int)bearing, pos.speed, pos.course_valid ? (int)pos.course : -1);
    }
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
    static bool was_valid = true;
    if (was_valid != gps_position.is_valid) {
        RoadLog::event("G t=%lu fix=%d sat=%d", millis(), gps_position.is_valid ? 1 : 0, gps_position.satellites);
        was_valid = gps_position.is_valid;
    }
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
    
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        static unsigned long last_pos_debug = 0;
        if (millis() - last_pos_debug > 5000) {
            Serial.print("[Speedcam] Posizione: ");
            Serial.print(gps_position.latitude, 6);
            Serial.print(", ");
            Serial.print(gps_position.longitude, 6);
            Serial.print(" | heading: ");
            if (gps_position.course_valid) {
                Serial.print((int)gps_position.course);
            } else {
                Serial.print("n/d");
            }
            Serial.print(" | raggio: ");
            Serial.print((int)detection_radius);
            Serial.println("m");
            last_pos_debug = millis();
        }
    }
    #endif

    refreshDatabaseIfDue(gps_position);

    // Compensa il ritardo del fix: la posizione GPS e' vecchia (eta' + latenza del modulo),
    // l'auto e' gia' piu' avanti lungo l'heading
    double eval_lat = gps_position.latitude;
    double eval_lng = gps_position.longitude;
    if (gps_position.course_valid && gps_position.speed >= SPEEDCAM_MIN_SPEED_KMH) {
        unsigned long age_ms = current_time - gps_position.last_update;
        if (age_ms > 3000) age_ms = 3000;
        double lookahead_s = (age_ms + SPEEDCAM_GPS_LATENCY_MS) / 1000.0;
        geo_project(gps_position.latitude, gps_position.longitude, gps_position.course,
                    (gps_position.speed / 3.6) * lookahead_s, eval_lat, eval_lng);
    }

    TrackerResult result = tracker.update(speedcams, speedcam_count,
                                          eval_lat, eval_lng,
                                          gps_position.course_valid, gps_position.course,
                                          detection_radius,
                                          gps_position.speed_valid && gps_position.speed >= SPEEDCAM_MIN_SPEED_KMH);

    logRoadEvents(gps_position, eval_lat, eval_lng, result);

    #if SPEEDCAM_ROAD_TEST_LOG
    logRoadTest(speedcams, speedcam_count, gps_position, detection_radius, result);
    #endif

    if (result.index >= 0) {
        last_detected_distance = result.distance;
        notifySpeedcamDetected(speedcams[result.index], result.distance, gps_position.speed, gps_position.speed_valid);
        return &speedcams[result.index];
    }

    if (result.hide) {
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            Serial.println("[Speedcam] POI superato o fuori cono, nascondo alert");
        }
        #endif
        last_detected_distance = 0.0;
        if (display_controller) {
            display_controller->hideSpeedcamAlert();
        }
    }

    return nullptr;
}

void SpeedcamController::notifySpeedcamDetected(const Speedcam& speedcam, float distance, float speed_kmh, bool speed_valid) {
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
        // Distanza a gradini (100/50/10 m): il metro esatto e' rumore, e cambia di continuo
        display_controller->showSpeedcamAlert(speedcam, (float)geo_display_distance_m(distance), speed_kmh, speed_valid);
    }
}

void SpeedcamController::setEnabled(bool enabled) {
    this->enabled = enabled;
    if (!enabled) {
        tracker.reset();
    }
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
