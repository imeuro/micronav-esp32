/*
 * MicroNav ESP32-C3 Speedcam Detection
 * Versione compatibile Arduino IDE
 */

#include "src/config.h"
#include "src/gps_controller.h"
#include "src/speedcam_controller.h"
#include "src/display_controller.h"
#include "src/road_log.h"
#include "src/json_parser.h"

// Puntatori globali - verranno inizializzati in setup()
// Questo evita che la costruzione degli oggetti blocchi prima che Serial sia inizializzato
GPSController* gps_controller = nullptr;
SpeedcamController* speedcam_controller = nullptr;
DisplayController* display_controller = nullptr;

// NOTA IMPORTANTE: non registriamo una callback GPSController::setPositionUpdateCallback
// che tocchi il display. Verificato empiricamente su hardware reale (ESP32-2424S012):
// qualsiasi chiamata a display_controller da dentro quella callback (invocata da
// GPSController::update() -> updateFakePosition()/updatePosition()) non produce
// alcun disegno visibile sul pannello fisico, pur senza errori, crash o reset, e
// con stack abbondante (>24KB liberi, misurato con uxTaskGetStackHighWaterMark).
// La causa esatta nello stack SPI/ESP32 Arduino core non è stata identificata, ma
// spostando lo stesso lavoro nel loop() principale (contesto "superficiale",
// non annidato dentro la callback) il rendering funziona regolarmente. Per questo
// l'aggiornamento dell'indicatore GPS e il check speedcam vengono fatti entrambi
// qui in loop(), leggendo la posizione con gps_controller->getPosition() dopo
// gps_controller->update().

static void onRadiusChanged(float radius_m) {
    if (speedcam_controller) speedcam_controller->setDetectionRadius(radius_m);
}

static void onDatabaseLoadProgress() {
    if (display_controller) display_controller->update();
}

void setup() {
    // Inizializza Serial per debug PRIMA di tutto
    // IMPORTANTE: USB CDC On Boot deve essere "Enabled" nelle impostazioni della board
    Serial.setTxBufferSize(4096);  // buffer ampio: con timeout 0 i print oltre il buffer vengono scartati
    Serial.begin(115200);
    Serial.setTxTimeoutMs(0);  // senza monitor la CDC USB bloccherebbe ogni print (avvio lento)
    
    // Attendi che Serial sia pronto (importante per ESP32-C3)
    delay(2000);
    
    // Messaggio di avvio
    Serial.println("\n\n\n");
    Serial.println("========================================");
    Serial.println("MicroNav ESP32-C3 Speedcam Detection");
    Serial.print("Build: ");
    Serial.print(__DATE__);
    Serial.print(" ");
    Serial.println(__TIME__);
    Serial.println("========================================");
    Serial.print("Baudrate: ");
    Serial.println(115200);
    #ifdef ESP32
    Serial.print("Free heap: ");
    Serial.println(ESP.getFreeHeap());
    #endif
    Serial.println("========================================\n");
    Serial.flush();
    delay(100);
    
    Serial.println("Inizio setup completo...");
    Serial.flush();
    delay(100);
    
    // Crea istanze degli oggetti DOPO che Serial è inizializzato
    Serial.println("[Setup] Creazione oggetti controller...");
    Serial.flush();
    delay(100);
    
    display_controller = new DisplayController();
    gps_controller = new GPSController();
    speedcam_controller = new SpeedcamController();
    
    Serial.println("[Setup] Oggetti creati");
    Serial.flush();
    delay(100);
    
    // 1. Inizializza display
    Serial.println("[Setup] Inizializzazione display...");
    Serial.flush();
    delay(100);
    
    if (!display_controller->begin()) {
        Serial.println("[Setup] ERRORE: Display non inizializzato!");
        Serial.println("[Setup] Continuo comunque senza display...");
        Serial.flush();
        delay(100);
    } else {
        Serial.println("[Setup] Display inizializzato con successo!");
        Serial.flush();
        delay(100);
        
        // 2. Mostra boot logo
        Serial.println("[Setup] Mostra boot logo...");
        Serial.flush();
        display_controller->showBootLogo(BOOT_LOGO_DISPLAY_TIME);
        Serial.println("[Setup] Boot logo completato");
        Serial.flush();
    }
    
    // 3. Inizializza GPS controller
    Serial.println("[Setup] Inizializzazione GPS...");
    Serial.flush();
    delay(100);
    
    #ifdef GPS_FAKE_MODE
    if (GPS_FAKE_MODE) {
        Serial.println("[Setup] Modalità GPS FAKE abilitata");
        Serial.flush();
        if (!gps_controller->beginFake(GPS_FAKE_JSON_PATH)) {
            Serial.println("[Setup] ERRORE: GPS fake non inizializzato!");
            Serial.flush();
        } else {
            Serial.println("[Setup] GPS fake inizializzato");
            Serial.flush();
        }
    } else {
        if (!gps_controller->begin(GPS_SERIAL_RX_PIN, GPS_SERIAL_TX_PIN)) {
            Serial.println("[Setup] ERRORE: GPS non inizializzato!");
            Serial.flush();
        } else {
            Serial.println("[Setup] GPS inizializzato");
            Serial.flush();
        }
    }
    #else
    if (!gps_controller->begin(GPS_SERIAL_RX_PIN, GPS_SERIAL_TX_PIN)) {
        Serial.println("[Setup] ERRORE: GPS non inizializzato!");
        Serial.flush();
    } else {
        Serial.println("[Setup] GPS inizializzato");
        Serial.flush();
    }
    #endif
    delay(100);
    
    // Raggio di detection: valore salvato (letto da display_controller->begin()) e
    // aggiornamenti da touch
    speedcam_controller->setDetectionRadius((float)display_controller->getDetectionRadius());
    display_controller->setRadiusChangedCallback(onRadiusChanged);
    JSONParser::setProgressCallback(onDatabaseLoadProgress);  // display/touch vivi durante il caricamento DB

    // 4. Inizializza Speedcam controller
    Serial.println("[Setup] Inizializzazione Speedcam controller...");
    Serial.flush();
    delay(100);
    
    if (!speedcam_controller->begin(gps_controller, display_controller)) {
        Serial.println("[Setup] ERRORE: Speedcam controller non inizializzato!");
        Serial.flush();
    } else {
        Serial.println("[Setup] Speedcam controller inizializzato");
        Serial.flush();
    }
    delay(100);
    
    // 5. Carica database speedcam
    Serial.println("[Setup] Caricamento database speedcam...");
    Serial.flush();
    delay(100);
    
    // Box geografico centrato sulla posizione reale (es. GPS fake, o fix gia' disponibile);
    // senza fix si parte da Milano e si ricarica al primo fix (vedi SpeedcamController)
    GPSPosition start_position = gps_controller->getPosition();
    bool has_start_fix = start_position.is_valid;
    double db_lat = has_start_fix ? start_position.latitude : PRE_FILTER_FALLBACK_LAT;
    double db_lng = has_start_fix ? start_position.longitude : PRE_FILTER_FALLBACK_LNG;
    Serial.print("[Setup] Box database centrato su ");
    Serial.println(has_start_fix ? "posizione GPS" : "ripiego (Milano)");
    Serial.flush();

    if (!speedcam_controller->loadDatabaseAround(db_lat, db_lng, has_start_fix)) {
        Serial.println("[Setup] ERRORE: Database speedcam non caricato!");
        Serial.println("[Setup] Assicurati che il file sia presente su LittleFS");
        Serial.flush();
    } else {
        Serial.println("[Setup] Database speedcam caricato con successo!");
        Serial.flush();
    }
    delay(100);
    
    // Log eventi su LittleFS (dopo il mount fatto dal caricamento database)
    RoadLog::begin();

    // 6. Mostra schermata idle
    Serial.println("[Setup] Mostra schermata idle...");
    Serial.flush();
    if (display_controller) {
        display_controller->showIdleScreen();
    }
    delay(100);
    
    Serial.println("\n[Setup] ========================================");
    Serial.println("[Setup] Setup completato!");
    Serial.println("[Setup] In attesa di fix GPS...");
    Serial.println("[Setup] ========================================\n");
    Serial.flush();
}

void loop() {
    // Verifica che i controller siano inizializzati
    if (!gps_controller || !speedcam_controller || !display_controller) {
        delay(1000);
        return;
    }
    
    // 1. Aggiorna GPS (legge seriale e parse NMEA)
    gps_controller->update();

    // 2. Aggiorna indicatore GPS sul display (vedi nota sopra sul perché questo
    // non viene fatto da una callback di GPSController)
    GPSPosition current_position = gps_controller->getPosition();
    display_controller->updateGPSIndicator(
        current_position.is_valid && current_position.satellites >= GPS_MIN_SATELLITES,
        current_position.satellites
    );

    // 3. Verifica speedcam (il throttle a SPEEDCAM_CHECK_INTERVAL e' dentro checkSpeedcams)
    // In modalità fake, la posizione è sempre valida anche se hasFix() potrebbe non essere true
    // Verifica direttamente is_valid invece di hasFix()
    if (current_position.is_valid) {
        speedcam_controller->checkSpeedcams(&current_position);
    } else {
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            static unsigned long last_warn = 0;
            if (millis() - last_warn > 5000) {
                GPSController::Stats gps_stats = gps_controller->getStats();
                Serial.printf("[Loop] Posizione GPS non valida, skip check speedcam "
                              "(status=%d sat=%u hdop=%.1f char=%lu frasi_ok=%lu)\n",
                              (int)gps_controller->getStatus(), current_position.satellites,
                              current_position.hdop, gps_stats.sentences_received,
                              gps_stats.valid_sentences);
                last_warn = millis();
            }
        }
        #endif
    }
    
    // Comandi seriali: 'L' scarica il log eventi, 'C' lo cancella
    while (Serial.available() > 0) {
        char cmd = Serial.read();
        if (cmd == 'L') {
            // Con timeout 0 i byte oltre il buffer TX verrebbero scartati: il dump va fatto con attesa
            Serial.setTxTimeoutMs(500);
            RoadLog::dump(Serial);
            Serial.flush();
            Serial.setTxTimeoutMs(0);
        }
        else if (cmd == 'C') RoadLog::clear();
    }
    RoadLog::update();

    // 4. Aggiorna display (gestisce timeout alert, ecc.)
    display_controller->update();

    // 5. Piccolo delay per evitare loop troppo veloce
    delay(MAIN_LOOP_DELAY);
}
