#include "gps_controller.h"

GPSController::GPSController() : 
    gps_serial(nullptr),
    status(GPS_DISCONNECTED),
    position_update_callback(nullptr),
    fake_mode(false),
    fake_route(nullptr),
    fake_route_count(0),
    fake_route_index(0),
    fake_route_loop(false),
    fake_last_update(0) {
    stats.sentences_received = 0;
    stats.valid_sentences = 0;
    stats.fix_attempts = 0;
    stats.last_fix_time = 0;
}

GPSController::~GPSController() {
    if (gps_serial) {
        gps_serial->end();
        delete gps_serial;
    }
    if (fake_route) {
        delete[] fake_route;
    }
}

bool GPSController::begin(int rx_pin, int tx_pin) {
    // Crea seriale hardware per GPS
    // ESP32-C3 ha più UART, usiamo UART1 per GPS
    gps_serial = new HardwareSerial(1);
    
    if (!gps_serial) {
        status = GPS_ERROR;
        return false;
    }
    
    // Inizializza seriale
    gps_serial->begin(GPS_SERIAL_BAUD, SERIAL_8N1, rx_pin, tx_pin);
    
    status = GPS_CONNECTING;
    
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.println("[GPS] Controller inizializzato");
        Serial.print("[GPS] RX Pin: ");
        Serial.println(rx_pin);
        if (tx_pin >= 0) {
            Serial.print("[GPS] TX Pin: ");
            Serial.println(tx_pin);
        }
    }
    #endif
    
    return true;
}

void GPSController::update() {
    // Modalità fake: aggiorna posizione da percorso fake
    if (fake_mode) {
        updateFakePosition();
        return;
    }
    
    if (!gps_serial || status == GPS_DISCONNECTED || status == GPS_ERROR) {
        return;
    }
    
    // Leggi dati dalla seriale GPS
    while (gps_serial->available() > 0) {
        char c = gps_serial->read();
        stats.sentences_received++;
        
        // Passa carattere al parser NMEA
        if (gps_parser.encode(c)) {
            stats.valid_sentences++;
            updatePosition();
            updateStatus();
        }
    }
    
    // Verifica timeout connessione
    if (status == GPS_CONNECTING) {
        if (millis() > 5000) {  // Dopo 5 secondi considera connesso
            status = GPS_CONNECTED;
            #ifdef DEBUG_ENABLED
            if (DEBUG_ENABLED) {
                Serial.println("[GPS] Connesso");
            }
            #endif
        }
    }
}

void GPSController::updatePosition() {
    // Aggiorna posizione dai dati TinyGPS++
    if (gps_parser.location.isValid()) {
        current_position.latitude = gps_parser.location.lat();
        current_position.longitude = gps_parser.location.lng();
        current_position.is_valid = true;
    } else {
        current_position.is_valid = false;
    }
    
    if (gps_parser.altitude.isValid()) {
        current_position.altitude = gps_parser.altitude.meters();
    }
    
    if (gps_parser.speed.isValid()) {
        current_position.speed = gps_parser.speed.kmph();
    }
    
    if (gps_parser.course.isValid()) {
        current_position.course = gps_parser.course.deg();
    }
    
    current_position.satellites = gps_parser.satellites.value();
    current_position.hdop = gps_parser.hdop.hdop();
    current_position.last_update = millis();
    
    // Chiama callback se impostata
    if (position_update_callback && current_position.is_valid) {
        position_update_callback(current_position);
    }
}

void GPSController::updateStatus() {
    if (!gps_parser.location.isValid()) {
        if (status == GPS_CONNECTED || status == GPS_FIXING) {
            status = GPS_FIXING;
            stats.fix_attempts++;
        }
    } else {
        // Verifica qualità fix
        if (current_position.satellites >= GPS_MIN_SATELLITES && 
            current_position.hdop <= GPS_MAX_HDOP) {
            if (status != GPS_FIXED) {
                status = GPS_FIXED;
                stats.last_fix_time = millis();
                #ifdef DEBUG_ENABLED
                if (DEBUG_ENABLED) {
                    Serial.println("[GPS] Fix ottenuto!");
                    Serial.print("[GPS] Satelliti: ");
                    Serial.println(current_position.satellites);
                    Serial.print("[GPS] HDOP: ");
                    Serial.println(current_position.hdop);
                }
                #endif
            }
        } else {
            status = GPS_FIXING;
        }
    }
}

GPSPosition GPSController::getPosition() const {
    return current_position;
}

bool GPSController::hasFix() const {
    return status == GPS_FIXED && current_position.is_valid;
}

GPSStatus GPSController::getStatus() const {
    return status;
}

bool GPSController::waitForFix(unsigned long timeout_ms) {
    unsigned long start_time = millis();
    
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.print("[GPS] Attesa fix (timeout: ");
        Serial.print(timeout_ms);
        Serial.println("ms)...");
    }
    #endif
    
    while (millis() - start_time < timeout_ms) {
        update();
        
        if (hasFix()) {
            #ifdef DEBUG_ENABLED
            if (DEBUG_ENABLED) {
                Serial.println("[GPS] Fix ottenuto!");
            }
            #endif
            return true;
        }
        
        delay(100);
    }
    
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.println("[GPS] Timeout attesa fix");
    }
    #endif
    
    return false;
}

void GPSController::setPositionUpdateCallback(void (*callback)(const GPSPosition&)) {
    position_update_callback = callback;
}

GPSController::Stats GPSController::getStats() const {
    return stats;
}

bool GPSController::beginFake(const char* json_path) {
    fake_mode = true;
    status = GPS_CONNECTING;
    
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.println("[GPS] Modalità FAKE abilitata");
        Serial.print("[GPS] Caricamento percorso da: ");
        Serial.println(json_path);
    }
    #endif
    
    if (!loadFakeRoute(json_path)) {
        status = GPS_ERROR;
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            Serial.println("[GPS] ERRORE: Impossibile caricare percorso fake");
        }
        #endif
        return false;
    }
    
    // Imposta posizione iniziale
    fake_route_index = 0;
    fake_last_update = millis();
    updateFakePosition();
    
    status = GPS_FIXED;
    
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.print("[GPS] Percorso fake caricato: ");
        Serial.print(fake_route_count);
        Serial.println(" punti");
        Serial.print("[GPS] Loop: ");
        Serial.println(fake_route_loop ? "abilitato" : "disabilitato");
    }
    #endif
    
    return true;
}

bool GPSController::loadFakeRoute(const char* json_path) {
    // Monta LittleFS con gli stessi parametri usati in json_parser
    // Specifica esplicitamente il nome della partizione "littlefs"
    if (!LittleFS.begin(false, "/littlefs", 5, "littlefs")) {
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            Serial.println("[GPS] LittleFS non montato, tentativo di formattazione...");
        }
        #endif
        
        // Se il mount fallisce, formatta e rimontare (true = formatOnFail)
        if (!LittleFS.begin(true, "/littlefs", 5, "littlefs")) {
            #ifdef DEBUG_ENABLED
            if (DEBUG_ENABLED) {
                Serial.println("[GPS] ERRORE: Impossibile montare LittleFS anche dopo formattazione");
            }
            #endif
            return false;
        }
        
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            Serial.println("[GPS] LittleFS formattato e montato con successo");
        }
        #endif
    }
    
    File file = LittleFS.open(json_path, "r");
    if (!file) {
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            Serial.print("[GPS] ERRORE: File non trovato: ");
            Serial.println(json_path);
        }
        #endif
        return false;
    }
    
    // Leggi file JSON
    size_t file_size = file.size();
    char* json_buffer = new char[file_size + 1];
    if (!json_buffer) {
        file.close();
        return false;
    }
    
    file.readBytes(json_buffer, file_size);
    json_buffer[file_size] = '\0';
    file.close();
    
    // Parse JSON
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, json_buffer);
    delete[] json_buffer;
    
    if (error) {
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            Serial.print("[GPS] ERRORE: Parsing JSON fallito: ");
            Serial.println(error.c_str());
        }
        #endif
        return false;
    }
    
    // Leggi array route
    JsonArray route = doc["route"];
    if (!route) {
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            Serial.println("[GPS] ERRORE: Campo 'route' non trovato");
        }
        #endif
        return false;
    }
    
    fake_route_count = route.size();
    if (fake_route_count == 0) {
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            Serial.println("[GPS] ERRORE: Percorso vuoto");
        }
        #endif
        return false;
    }
    
    // Alloca array punti
    fake_route = new FakeRoutePoint[fake_route_count];
    if (!fake_route) {
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            Serial.println("[GPS] ERRORE: Memoria insufficiente");
        }
        #endif
        return false;
    }
    
    // Carica punti
    int i = 0;
    for (JsonObject point : route) {
        fake_route[i].lat = point["lat"].as<double>();
        fake_route[i].lng = point["lng"].as<double>();
        fake_route[i].altitude = point.containsKey("altitude") ? point["altitude"].as<double>() : 0.0;
        fake_route[i].speed = point.containsKey("speed") ? point["speed"].as<float>() : 0.0;
        fake_route[i].course = point.containsKey("course") ? point["course"].as<float>() : 0.0;
        fake_route[i].satellites = point.containsKey("satellites") ? point["satellites"].as<uint8_t>() : 8;
        fake_route[i].hdop = point.containsKey("hdop") ? point["hdop"].as<float>() : 1.5;
        i++;
    }
    
    // Leggi flag loop
    fake_route_loop = doc.containsKey("loop") ? doc["loop"].as<bool>() : false;
    
    return true;
}

void GPSController::updateFakePosition() {
    if (!fake_route || fake_route_count == 0) {
        return;
    }
    
    // Verifica se è il momento di aggiornare
    unsigned long current_time = millis();
    if (current_time - fake_last_update < GPS_FAKE_UPDATE_INTERVAL) {
        return;
    }
    
    fake_last_update = current_time;
    
    // Aggiorna posizione corrente
    FakeRoutePoint& point = fake_route[fake_route_index];
    
    current_position.latitude = point.lat;
    current_position.longitude = point.lng;
    current_position.altitude = point.altitude;
    current_position.speed = point.speed;
    current_position.course = point.course;
    current_position.satellites = point.satellites;
    current_position.hdop = point.hdop;
    current_position.is_valid = true;
    current_position.last_update = current_time;
    
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.print("[GPS] Posizione fake aggiornata: ");
        Serial.print(point.lat, 6);
        Serial.print(", ");
        Serial.print(point.lng, 6);
        Serial.print(" (punto ");
        Serial.print(fake_route_index + 1);
        Serial.print("/");
        Serial.print(fake_route_count);
        Serial.println(")");
    }
    #endif
    
    // Passa al punto successivo
    fake_route_index++;
    
    // Gestisci loop o fine percorso
    if (fake_route_index >= fake_route_count) {
        if (fake_route_loop) {
            fake_route_index = 0;  // Ricomincia dall'inizio
        } else {
            fake_route_index = fake_route_count - 1;  // Rimani sull'ultimo punto
        }
    }
    
    // Chiama callback se impostata
    if (position_update_callback) {
        position_update_callback(current_position);
    }
}
