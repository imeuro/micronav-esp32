#include "display_controller.h"
#include "speedcam_controller.h"

// Include boot logo array (se il file esiste, definisce BOOT_LOGO_DATA_AVAILABLE all'inizio)
// Se il file non esiste, la compilazione fallirà - genera con: python3 convert_assets.py
#include "boot_logo.h"

// Nota: Implementare con libreria display GC9A01 corretta
// Per ora, implementazione stub che deve essere completata con driver specifico
// Opzioni:
// 1. Usare libreria Adafruit_GC9A01 (se disponibile)
// 2. Implementare driver custom per GC9A01
// 3. Usare TFT_eSPI con supporto GC9A01

DisplayController::DisplayController() :
    display(nullptr),
    is_initialized(false),
    showing_alert(false),
    alert_start_time(0),
    alert_display_time(10000),  // 10 secondi default
    gps_has_fix(false),
    gps_satellites(0) {
}

DisplayController::~DisplayController() {
    if (display) {
        delete display;
    }
}

bool DisplayController::begin() {
    // Opzione per disabilitare display (per debug)
    #ifdef DISPLAY_ENABLED
    if (!DISPLAY_ENABLED) {
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            Serial.println("[Display] Display disabilitato in config.h");
        }
        #endif
        return false;
    }
    #endif
    
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.println("[Display] Inizio inizializzazione...");
        Serial.flush();
    }
    #endif
    
    // Inizializza pin di reset e backlight prima di SPI
    // Nota: RST può essere -1 se non usato (come nel Factory_samples.ino)
    if (DISPLAY_RST_PIN >= 0) {
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            Serial.print("[Display] Reset pin: ");
            Serial.println(DISPLAY_RST_PIN);
        }
        #endif
        pinMode(DISPLAY_RST_PIN, OUTPUT);
        digitalWrite(DISPLAY_RST_PIN, LOW);
        delay(10);
        digitalWrite(DISPLAY_RST_PIN, HIGH);
        delay(10);
    } else {
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            Serial.println("[Display] Reset pin: non usato (-1)");
        }
        #endif
    }
    
    // Inizializza backlight se presente (ma NON accenderlo ancora!)
    // Lo accenderemo solo DOPO che lo schermo è pulito per evitare "effetto neve"
    if (DISPLAY_BL_PIN >= 0) {
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            Serial.print("[Display] Backlight pin: ");
            Serial.println(DISPLAY_BL_PIN);
        }
        #endif
        pinMode(DISPLAY_BL_PIN, OUTPUT);
        digitalWrite(DISPLAY_BL_PIN, LOW);  // Backlight SPENTO durante inizializzazione
    }
    
    // Inizializza SPI per display
    // NOTA: Non chiamare SPI.begin() qui - la libreria Adafruit lo fa automaticamente
    // Chiamarlo qui può causare conflitti
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.println("[Display] SPI verrà inizializzato dalla libreria...");
        Serial.flush();
    }
    #endif
    delay(50);  // Delay per stabilizzazione prima di creare display
    
    // Crea oggetto display GC9A01
    // Nota: Per ESP32-C3, i pin SPI hardware di default sono:
    // - MOSI: GPIO7
    // - MISO: GPIO2
    // - SCK: GPIO6
    // Il costruttore con 3 parametri usa SPI hardware di default
    // Se i pin MOSI/SCK sono diversi, usare costruttore con 5 parametri
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.print("[Display] Creazione oggetto display (CS=");
        Serial.print(DISPLAY_CS_PIN);
        Serial.print(", DC=");
        Serial.print(DISPLAY_DC_PIN);
        Serial.print(", RST=");
        Serial.print(DISPLAY_RST_PIN);
        #if defined(DISPLAY_MOSI_PIN) && defined(DISPLAY_SCK_PIN)
        Serial.print(", MOSI=");
        Serial.print(DISPLAY_MOSI_PIN);
        Serial.print(", SCK=");
        Serial.print(DISPLAY_SCK_PIN);
        #else
        Serial.print(", SPI=hardware default (MOSI=7, SCK=6)");
        #endif
        Serial.println(")...");
    }
    #endif
    
    // Usa costruttore con pin SPI espliciti se definiti, altrimenti usa SPI hardware default
    #if defined(DISPLAY_MOSI_PIN) && defined(DISPLAY_SCK_PIN) && DISPLAY_MOSI_PIN >= 0 && DISPLAY_SCK_PIN >= 0
        // Costruttore con pin SPI espliciti
        display = new Adafruit_GC9A01A(
            DISPLAY_CS_PIN,
            DISPLAY_DC_PIN,
            DISPLAY_MOSI_PIN,
            DISPLAY_SCK_PIN,
            DISPLAY_RST_PIN
        );
    #else
        // Costruttore con SPI hardware di default (MOSI=7, SCK=6 su ESP32-C3)
        display = new Adafruit_GC9A01A(
            DISPLAY_CS_PIN,
            DISPLAY_DC_PIN,
            DISPLAY_RST_PIN
        );
    #endif
    
    if (!display) {
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            Serial.println("[Display] ERRORE: Errore creazione oggetto display");
        }
        #endif
        return false;
    }
    
    // Inizializza display
    // Nota: begin() restituisce void, non bool
    // IMPORTANTE: begin() può richiedere tempo e potrebbe causare reset watchdog
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.println("[Display] Chiamata display->begin()...");
        Serial.flush();
    }
    #endif
    
    // Feed watchdog prima di begin() per evitare reset
    #ifdef ESP32
    yield();  // Feed watchdog
    #endif
    
    // Prova inizializzazione con timeout implicito
    // Se begin() blocca, il watchdog resetterà il sistema
    display->begin();
    
    #ifdef ESP32
    yield();  // Feed watchdog dopo begin()
    #endif
    
    // Pulisci IMMEDIATAMENTE lo schermo per evitare puntini casuali all'avvio
    // Fallo prima di qualsiasi altra operazione e PRIMA di accendere il backlight
    display->fillScreen(COLOR_BLACK);
    delay(10);  // Delay per permettere l'aggiornamento
    
    delay(50);  // Delay per stabilizzazione hardware dopo pulizia
    
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.println("[Display] display->begin() completato");
        Serial.flush();
    }
    #endif
    
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.println("[Display] Configurazione display...");
    }
    #endif
    
    // Configura display
    display->setRotation(0);  // Orientamento normale
    delay(10);
    
    // Pulisci nuovamente lo schermo per sicurezza
    display->fillScreen(COLOR_BLACK);
    delay(10);
    
    // ORA accendi il backlight solo quando lo schermo è già nero e pulito
    // Questo elimina completamente l'effetto "neve" all'avvio
    if (DISPLAY_BL_PIN >= 0) {
        digitalWrite(DISPLAY_BL_PIN, HIGH);
        delay(10);  // Piccolo delay per stabilizzazione backlight
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            Serial.println("[Display] Backlight acceso (schermo già pulito)");
        }
        #endif
    }
    
    is_initialized = true;
    
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.println("[Display] ✅ Display inizializzato (240x240)");
    }
    #endif
    
    return true;
}

void DisplayController::showBootLogo(unsigned long display_time_ms) {
    if (!is_initialized || !display) {
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            Serial.println("[Display] showBootLogo: display non inizializzato!");
        }
        #endif
        return;
    }
    
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.println("[Display] showBootLogo: inizio rendering...");
    }
    #endif
    
    // Assicura che backlight sia acceso
    if (DISPLAY_BL_PIN >= 0) {
        digitalWrite(DISPLAY_BL_PIN, HIGH);
    }
    
    // Mostra boot logo da array C (veloce, compilato nel firmware)
    display->fillScreen(COLOR_BLACK);
    delay(10);
    
    #ifdef BOOT_LOGO_DATA_AVAILABLE
    // Mostra boot logo con fade-in opzionale
    unsigned long render_start = millis();
    
    const uint16_t width = boot_logo_data_width;
    const uint16_t height = boot_logo_data_height;
    
    #if BOOT_LOGO_FADE_ENABLED
        // Fade-in: renderizza il logo più volte con intensità crescente
        const uint16_t fade_steps = BOOT_LOGO_FADE_STEPS;
        const unsigned long fade_duration = BOOT_LOGO_FADE_DURATION;
        const unsigned long step_delay = fade_duration / fade_steps;
        
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            Serial.print("[Display] Boot logo fade-in: ");
            Serial.print(fade_steps);
            Serial.print(" step, ");
            Serial.print(fade_duration);
            Serial.println(" ms");
            Serial.flush();
        }
        #endif
        
        // Buffer per una riga (200 pixel = 400 bytes)
        static uint16_t row_buffer[200];  // Max width del logo
        
        for (uint16_t step = 0; step <= fade_steps; step++) {
            // Calcola fattore fade (0.0 = nero, 1.0 = colore pieno)
            float fade_factor = (float)step / (float)fade_steps;
            
            // Imposta area di disegno
            display->startWrite();
            display->setAddrWindow(boot_logo_data_offset_x, boot_logo_data_offset_y, 
                                  width, height);
            
            // Renderizza logo con fade applicato
            for (uint16_t y = 0; y < height; y++) {
                // Copia riga da PROGMEM a RAM e applica fade
                for (uint16_t x = 0; x < width; x++) {
                    uint32_t idx = y * width + x;
                    uint16_t original_color = pgm_read_word(&boot_logo_data[idx]);
                    row_buffer[x] = fadeColor565(original_color, fade_factor);
                }
                // Invia riga completa in batch
                display->writePixels(row_buffer, width, true, false);
            }
            
            display->endWrite();
            
            // Delay tra step (tranne l'ultimo)
            if (step < fade_steps) {
                delay(step_delay);
            }
        }
    #else
        // Rendering normale senza fade (velocissimo)
        display->startWrite();
        display->setAddrWindow(boot_logo_data_offset_x, boot_logo_data_offset_y, 
                              width, height);
        
        // Buffer per una riga (200 pixel = 400 bytes)
        static uint16_t row_buffer[200];  // Max width del logo
        
        for (uint16_t y = 0; y < height; y++) {
            // Copia riga da PROGMEM a RAM
            for (uint16_t x = 0; x < width; x++) {
                uint32_t idx = y * width + x;
                row_buffer[x] = pgm_read_word(&boot_logo_data[idx]);
            }
            // Invia riga completa in batch (MOLTO più veloce di pixel singoli!)
            display->writePixels(row_buffer, width, true, false);
        }
        
        display->endWrite();
    #endif
    
    unsigned long render_time = millis() - render_start;
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.print("[Display] Boot logo renderizzato in ");
        Serial.print(render_time);
        Serial.print(" ms (");
        #if BOOT_LOGO_FADE_ENABLED
        Serial.print("fade-in, ");
        #endif
        Serial.print(width * height);
        Serial.println(" pixel)");
        Serial.flush();
    }
    #endif
    #else
    // Fallback: mostra testo "MicroNav"
    display->setTextColor(COLOR_WHITE);
    display->setTextSize(2);
    
    // Centra testo "MicroNav"
    int16_t x1, y1;
    uint16_t w, h;
    display->getTextBounds("MicroNav", 0, 0, &x1, &y1, &w, &h);
    int16_t x = (DISPLAY_WIDTH - w) / 2;
    int16_t y = (DISPLAY_HEIGHT - h) / 2;
    
    display->setCursor(x, y);
    display->print("MicroNav");
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.println("[Display] Boot logo non disponibile, uso testo fallback");
        Serial.flush();
    }
    #endif
    #endif
    
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.println("[Display] showBootLogo: rendering completato");
        Serial.flush();
    }
    #endif
    
    // Disegna indicatore GPS in alto al centro (pallino verde/rosso)
    drawGPSIndicator(gps_has_fix, gps_satellites);
    
    // Aggiungi info GPS sotto il logo (senza re-render completo)
    drawGPSInfo();
    
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.println("[Display] showBootLogo: info GPS aggiunte");
        Serial.flush();
    }
    #endif
    
    // Mostra per tempo specificato
    delay(display_time_ms);
}

void DisplayController::showIdleScreen() {
    if (!is_initialized || !display) {
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            Serial.println("[Display] showIdleScreen: display non inizializzato!");
        }
        #endif
        return;
    }
    
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.println("[Display] showIdleScreen: aggiornamento info GPS...");
    }
    #endif
    
    // Assicura che backlight sia acceso
    if (DISPLAY_BL_PIN >= 0) {
        digitalWrite(DISPLAY_BL_PIN, HIGH);
    }
    
    // Aggiorna solo le info GPS (senza re-render completo, mantiene il logo)
    drawGPSInfo();
    
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.println("[Display] showIdleScreen: rendering completato");
    }
    #endif
    
    showing_alert = false;
}

void DisplayController::showSpeedcamAlert(const struct Speedcam& speedcam, float distance) {
    if (!is_initialized) return;
    
    showing_alert = true;
    alert_start_time = millis();
    
    // Disegna alert sopra schermata corrente
    drawSpeedcamAlertContent(speedcam, distance);
}

void DisplayController::hideSpeedcamAlert() {
    if (!is_initialized) return;
    
    showing_alert = false;
    
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.println("[Display] hideSpeedcamAlert: ritorno al boot logo");
    }
    #endif
    
    // Mostra boot logo invece della schermata idle
    showBootLogo(0);  // 0 = mostra senza delay
}

void DisplayController::update() {
    if (!is_initialized) return;
    
    // Verifica timeout alert
    if (showing_alert && (millis() - alert_start_time) > alert_display_time) {
        hideSpeedcamAlert();
    }
}

void DisplayController::updateGPSIndicator(bool has_fix, uint8_t satellites) {
    gps_has_fix = has_fix;
    gps_satellites = satellites;
    
    if (!showing_alert) {
        // Aggiorna indicatore visivo (cerchio in alto al centro)
        drawGPSIndicator(has_fix, satellites);
        // Aggiorna anche le info GPS testuali (senza re-render completo)
        drawGPSInfo();
    }
}

void DisplayController::drawSpeedcamAlertContent(const struct Speedcam& speedcam, float distance) {
    if (!display) return;
    
    // Dimensioni alert adattate per 240x240 (vs 320x240 originale)
    // Scala approssimativa: 75% (240/320)
    int16_t alert_width = 120;  // 160 * 0.75
    int16_t alert_height = 120; // 160 * 0.75
    int16_t alert_x = (DISPLAY_WIDTH - alert_width) / 2;  // Centrato
    int16_t alert_y = 60;
    
    // Background semi-trasparente (simulato con rettangolo grigio scuro)
    display->fillRect(0, 20, DISPLAY_WIDTH, DISPLAY_HEIGHT - 40, COLOR_DARK_GRAY);
    
    // Rounded rectangle rosso per alert (simulato con rettangolo normale)
    drawRoundedRectFilled(alert_x, alert_y, alert_width, alert_height, 6, COLOR_MICRONAV_RED_20);
    drawRoundedRect(alert_x, alert_y, alert_width, alert_height, 6, COLOR_MICRONAV_RED);
    
    // Tipo speedcam
    const char* type_text = (speedcam.type[0] == 'A') ? "T RED" : "VELOX";
    display->setTextColor(COLOR_WHITE);
    display->setTextSize(1);
    display->setCursor(alert_x + 10, alert_y + 20);
    display->print(type_text);
    
    // Stato (attivo/inattivo)
    const char* status_text = (speedcam.status == 'A') ? "attivo" : "inattivo";
    display->setCursor(alert_x + 10, alert_y + 35);
    display->print(status_text);
    
    // Distanza (grande)
    char distance_str[16];
    snprintf(distance_str, sizeof(distance_str), "%dm", (int)distance);
    display->setTextSize(2);
    display->setCursor(alert_x + 10, alert_y + 55);
    display->print(distance_str);
    
    // Indicatore visivo (cerchio)
    int16_t indicator_size = 38;  // 50 * 0.75
    int16_t indicator_x = alert_x + alert_width - indicator_size / 2 - 8;
    int16_t indicator_y = alert_y + 50;
    
    // Disegna cerchio con bordo
    drawCircleWithBorder(indicator_x, indicator_y, indicator_size / 2, COLOR_WHITE, COLOR_RED, 4);
    
    // Mostra limite velocità o icona semaforo
    if (speedcam.type[0] == 'A') {
        // Tipo A: mostra icona semaforo (per ora testo)
        display->setTextColor(COLOR_BLACK);
        display->setTextSize(1);
        display->setCursor(indicator_x - 8, indicator_y - 4);
        display->print("TL");
    } else if (speedcam.vmax[0] != '\0' && speedcam.vmax[0] != '/') {
        // Mostra limite velocità
        display->setTextColor(COLOR_BLACK);
        display->setTextSize(1);
        int16_t text_x = indicator_x - 6;
        int16_t text_y = indicator_y - 4;
        display->setCursor(text_x, text_y);
        display->print(speedcam.vmax);
    }
}

void DisplayController::drawGPSInfo() {
    if (!display) return;
    
    // Pulisci solo l'area delle info GPS (righe 170-200) per evitare artefatti
    display->fillRect(0, 165, DISPLAY_WIDTH, 35, COLOR_BLACK);
    
    // Informazioni GPS
    display->setTextColor(COLOR_WHITE);
    display->setTextSize(1);
    
    // Centra la scritta "GPS: ..." orizzontalmente usando la larghezza dello schermo
    const char* gps_status_text = gps_has_fix ? "GPS: Fix OK" : "GPS: In attesa...";
    int16_t x1, y1;
    uint16_t w, h;
    display->getTextBounds(gps_status_text, 0, 0, &x1, &y1, &w, &h);
    int16_t center_x = (display->width() - w) / 2;
    display->setCursor(center_x, 170);
    display->print(gps_status_text);
    
    if (gps_has_fix) {
        // Centra la scritta "Sat: X" rispetto allo schermo
        char sat_text[16];
        snprintf(sat_text, sizeof(sat_text), "Sat: %u", gps_satellites);
        display->getTextBounds(sat_text, 0, 0, &x1, &y1, &w, &h);
        center_x = (display->width() - w) / 2;
        display->setCursor(center_x, 185);
        display->print(sat_text);
    }
}

void DisplayController::drawGPSIndicator(bool has_fix, uint8_t satellites) {
    if (!display) return;
    
    // Disegna indicatore GPS in alto al centro (pallino verde/rosso)
    int16_t x = DISPLAY_WIDTH / 2;  // Centro orizzontale
    int16_t y = 15;  // In alto
    int16_t radius = 8;
    
    // Pulisci area del cerchio per evitare artefatti (disegna cerchio nero leggermente più grande)
    display->fillCircle(x, y, radius + 2, COLOR_BLACK);
    
    // Colore: verde se ha fix, rosso se in attesa
    uint16_t color = has_fix ? COLOR_GREEN : COLOR_RED;
    display->fillCircle(x, y, radius, color);
    display->drawCircle(x, y, radius, COLOR_BLACK);
}

void DisplayController::drawRoundedRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t radius, uint16_t color) {
    if (!display) return;
    
    // Simulazione rounded rectangle (Adafruit GFX non ha rounded rect nativo)
    // Disegna rettangolo normale per ora
    // TODO: Implementare rounded rectangle manualmente se necessario
    display->drawRect(x, y, w, h, color);
}

void DisplayController::drawRoundedRectFilled(int16_t x, int16_t y, int16_t w, int16_t h, int16_t radius, uint16_t color) {
    if (!display) return;
    
    // Simulazione rounded rectangle filled
    // Disegna rettangolo riempito per ora
    // TODO: Implementare rounded rectangle filled manualmente se necessario
    display->fillRect(x, y, w, h, color);
}

void DisplayController::drawCircleWithBorder(int16_t x, int16_t y, int16_t radius, uint16_t fill_color, uint16_t border_color, int16_t border_width) {
    if (!display) return;
    
    // Disegna cerchio riempito
    display->fillCircle(x, y, radius, fill_color);
    
    // Disegna bordo (cerchi concentrici)
    for (int16_t i = 0; i < border_width; i++) {
        display->drawCircle(x, y, radius - i, border_color);
    }
}

uint16_t DisplayController::color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

uint16_t DisplayController::fadeColor565(uint16_t color, float fade_factor) {
    // Clamp fade_factor tra 0.0 e 1.0
    if (fade_factor < 0.0f) fade_factor = 0.0f;
    if (fade_factor > 1.0f) fade_factor = 1.0f;
    
    // Estrai componenti RGB dal colore RGB565
    uint8_t r = (color >> 11) & 0x1F;  // 5 bit
    uint8_t g = (color >> 5) & 0x3F;   // 6 bit
    uint8_t b = color & 0x1F;          // 5 bit
    
    // Applica fade (mescola con nero = moltiplica per fade_factor)
    r = (uint8_t)(r * fade_factor);
    g = (uint8_t)(g * fade_factor);
    b = (uint8_t)(b * fade_factor);
    
    // Ricostruisci colore RGB565
    return ((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F);
}
