/*
 * MicroNav ESP32-C3 Speedcam Detection
 * Versione compatibile Arduino IDE
 */

#include "src/config.h"
#include "src/gps_controller.h"
#include "src/speedcam_controller.h"
#include "src/display_controller.h"

// Puntatori globali - verranno inizializzati in setup()
// Questo evita che la costruzione degli oggetti blocchi prima che Serial sia inizializzato
GPSController* gps_controller = nullptr;
SpeedcamController* speedcam_controller = nullptr;
DisplayController* display_controller = nullptr;

// Callback per aggiornamento posizione GPS
void onGPSPositionUpdate(const GPSPosition& position) {
    // Aggiorna display con nuovo stato GPS
    if (display_controller) {
        display_controller->updateGPSIndicator(
            position.is_valid && position.satellites >= GPS_MIN_SATELLITES,
            position.satellites
        );
    }
    
    // Verifica speedcam (se posizione valida)
    if (position.is_valid && speedcam_controller) {
        speedcam_controller->checkSpeedcams(&position);
    }
}

void setup() {
    // Inizializza Serial per debug PRIMA di tutto
    // IMPORTANTE: USB CDC On Boot deve essere "Enabled" nelle impostazioni della board
    Serial.begin(115200);
    
    // Attendi che Serial sia pronto (importante per ESP32-C3)
    delay(2000);
    
    // Messaggio di avvio
    Serial.println("\n\n\n");
    Serial.println("========================================");
    Serial.println("MicroNav ESP32-C3 Speedcam Detection");
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
    
    // Imposta callback per aggiornamento posizione GPS
    Serial.println("[Setup] Impostazione callback GPS...");
    Serial.flush();
    gps_controller->setPositionUpdateCallback(onGPSPositionUpdate);
    delay(100);
    
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
    
    // Carica con pre-filtraggio geografico se abilitato
    bool pre_filter = SPEEDCAM_PRE_FILTER_ENABLED;
    Serial.print("[Setup] Pre-filtro geografico: ");
    Serial.println(pre_filter ? "abilitato" : "disabilitato");
    Serial.flush();
    delay(100);
    
    if (!speedcam_controller->loadDatabase(
            SPEEDCAM_JSON_PATH, 
            pre_filter,
            PRE_FILTER_MIN_LAT, PRE_FILTER_MAX_LAT,
            PRE_FILTER_MIN_LNG, PRE_FILTER_MAX_LNG)) {
        Serial.println("[Setup] ERRORE: Database speedcam non caricato!");
        Serial.println("[Setup] Assicurati che il file sia presente su LittleFS");
        Serial.flush();
    } else {
        Serial.println("[Setup] Database speedcam caricato con successo!");
        Serial.flush();
    }
    delay(100);
    
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
    
    // 2. Se GPS ha fix, verifica speedcam (callback viene chiamato automaticamente)
    // Ma facciamo anche un check manuale periodico
    static unsigned long last_speedcam_check = 0;
    if (millis() - last_speedcam_check > SPEEDCAM_CHECK_INTERVAL) {
        GPSPosition position = gps_controller->getPosition();
        // In modalità fake, la posizione è sempre valida anche se hasFix() potrebbe non essere true
        // Verifica direttamente is_valid invece di hasFix()
        if (position.is_valid) {
            speedcam_controller->checkSpeedcams(&position);
        } else {
            #ifdef DEBUG_ENABLED
            if (DEBUG_ENABLED) {
                static unsigned long last_warn = 0;
                if (millis() - last_warn > 5000) {
                    Serial.println("[Loop] Posizione GPS non valida, skip check speedcam");
                    last_warn = millis();
                }
            }
            #endif
        }
        last_speedcam_check = millis();
    }
    
    // 3. Aggiorna display (gestisce timeout alert, ecc.)
    display_controller->update();
    
    // 4. Piccolo delay per evitare loop troppo veloce
    delay(MAIN_LOOP_DELAY);
}
