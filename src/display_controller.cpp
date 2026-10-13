#include "display_controller.h"
#include "speedcam_controller.h"

// Include boot logo array (se il file esiste, definisce BOOT_LOGO_DATA_AVAILABLE all'inizio)
// Se il file non esiste, la compilazione fallirà - genera con: python3 convert_assets.py
#include "boot_logo.h"

// Include font personalizzati se abilitati in config.h
#ifdef CUSTOM_FONT_ENABLED
// Include diretti - il preprocessore C++ non può usare macro per gli include
// Se CUSTOM_FONT_NAME è definito, include il font normale
#ifdef CUSTOM_FONT_NAME
// Include font normale (big: 16px Black, small: 8px Bold)
#include "geist_font_big.h"
#include "geist_font_small.h"
#endif

// Se CUSTOM_FONT_MONO_NAME è definito, include il font monospaced
#ifdef CUSTOM_FONT_MONO_NAME
#include "geist_mono_font.h"
#endif
#endif

// Nota: Implementato con libreria Adafruit_GC9A01

DisplayController::DisplayController() :
    display(nullptr),
    is_initialized(false),
    showing_alert(false),
    alert_start_time(0),
    alert_display_time(5000),  // 5 secondi default
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
    
    // Crea oggetto display GC9A01 tramite LovyanGFX, con la configurazione
    // pannello verificata su hardware reale (cfg.invert = true incluso: vedi
    // src/lgfx_panel_config.h e openspec/changes/migrate-display-lovyangfx/).
    //
    // NOTA STORICA: in precedenza si usava Adafruit_GC9A01A, ma non
    // inizializzava correttamente questo pannello specifico (mancava
    // l'inversione colore, tra altro). Dopo uno spegnimento/riaccensione
    // completo (non un semplice reset software) lo schermo restava nero e
    // nessun disegno (testo, forme) era mai visibile. Verificato su
    // hardware reale che LovyanGFX, con questa config, risolve il problema.
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.print("[Display] Creazione oggetto display (CS=");
        Serial.print(DISPLAY_CS_PIN);
        Serial.print(", DC=");
        Serial.print(DISPLAY_DC_PIN);
        Serial.print(", RST=");
        Serial.print(DISPLAY_RST_PIN);
        Serial.print(", SPI hardware MOSI=");
        Serial.print(DISPLAY_MOSI_PIN);
        Serial.print(", SCK=");
        Serial.print(DISPLAY_SCK_PIN);
        Serial.println(")...");
    }
    #endif

    display = new LGFX();

    if (!display) {
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            Serial.println("[Display] ERRORE: Errore creazione oggetto display");
        }
        #endif
        return false;
    }

    // Inizializza display (init() restituisce bool)
    // IMPORTANTE: init() può richiedere tempo e potrebbe causare reset watchdog
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.println("[Display] Chiamata display->init()...");
        Serial.flush();
    }
    #endif

    // Feed watchdog prima di init() per evitare reset
    #ifdef ESP32
    yield();  // Feed watchdog
    #endif

    bool init_ok = display->init();

    #ifdef ESP32
    yield();  // Feed watchdog dopo init()
    #endif

    if (!init_ok) {
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            Serial.println("[Display] ERRORE: display->init() fallito!");
        }
        #endif
        return false;
    }

    // Pulisci IMMEDIATAMENTE lo schermo per evitare puntini casuali all'avvio
    // Fallo prima di qualsiasi altra operazione e PRIMA di accendere il backlight
    display->fillScreen(COLOR_BLACK);
    delay(10);  // Delay per permettere l'aggiornamento

    delay(50);  // Delay per stabilizzazione hardware dopo pulizia

    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.println("[Display] display->init() completato");
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
    
    // Imposta font personalizzato se abilitato (default: font normale)
    #ifdef CUSTOM_FONT_ENABLED
    #ifdef CUSTOM_FONT_NAME
    setNormalFont();
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.print("[Display] Font normale impostato");
        #ifdef CUSTOM_FONT_MONO_NAME
        Serial.print(", font mono disponibile");
        #endif
        Serial.println();
    }
    #endif
    #endif
    #endif
    
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
                display->writePixels(row_buffer, width, true);
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
            display->writePixels(row_buffer, width, true);
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

    // Centra testo "MicroNav"
    display->drawCenterString("MicroNav", DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2);
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
    int16_t alert_width = 240;  // 160 * 0.75
    int16_t alert_height = 80; // 160 * 0.75
    int16_t alert_x = (DISPLAY_WIDTH - alert_width) / 2;  // Centrato
    int16_t alert_y = (DISPLAY_HEIGHT - alert_height) / 2;
    
    // Background semi-trasparente (simulato con rettangolo grigio scuro)
    // display->fillRect(0, 20, DISPLAY_WIDTH, DISPLAY_HEIGHT - 40, COLOR_DARK_GRAY);
    
    // Pulisce l'area dell'alert (sfondo nero, nessun bordo/riquadro visibile)
    drawRoundedRectFilled(alert_x, alert_y, alert_width, alert_height, 12, COLOR_BLACK);
    
    // NOTA: LovyanGFX usa datum "top_left" di default (Y = cima del testo),
    // a differenza di Adafruit GFX che con i font custom (GFXfont) posiziona
    // il cursore sulla baseline. Le coordinate Y qui sotto sono quindi
    // "cima del testo", non baseline come nella versione Adafruit originale.

    // Tipo speedcam (usa font small)
    const char* type_text = (speedcam.type[0] == 'A') ? "T RED" : "VELOX";
    setSmallFont();
    display->setTextColor(COLOR_WHITE);
    display->setCursor(alert_x + 20, alert_y + 12);
    display->print(type_text);

    // Stato (attivo/inattivo) - usa font small per testo più piccolo
    const char* status_text = (speedcam.status == 'A') ? "attivo" : "inattivo";
    setSmallFont();
    display->setTextColor(COLOR_LIGHT_GRAY);
    display->setCursor(alert_x + 20, alert_y + 28);
    display->print(status_text);

    // Distanza (grande) - usa font normale, allineata a sinistra come il resto
    // della colonna. Posizionata per restare dentro la fascia rossa (altezza
    // alert_height=80), con margine dal bordo inferiore.
    char distance_str[16];
    snprintf(distance_str, sizeof(distance_str), "%dm", (int)distance);
    setNormalFont();
    display->setTextColor(COLOR_WHITE);
    display->setCursor(alert_x + 20, alert_y + 46);
    display->print(distance_str);

    // Indicatore visivo (cerchio)
    int16_t indicator_size = 66;  // 50 * 0.75
    int16_t indicator_x = alert_x + alert_width - indicator_size / 2 - 20;
    int16_t indicator_y = alert_y + 40;

    // Disegna cerchio con bordo
    drawCircleWithBorder(indicator_x, indicator_y, indicator_size / 2, COLOR_WHITE, COLOR_MICRONAV_RED, 8);

    // Mostra limite velocità o icona semaforo, centrati nel cerchio con
    // drawCenterString (centra orizzontalmente in base alla larghezza reale
    // del testo, invece di un offset fisso stimato a occhio)
    if (speedcam.type[0] == 'A') {
        // Tipo A: mostra icona semaforo (per ora testo) - usa font normale
        setNormalFont();
        display->setTextColor(COLOR_BLACK);
        display->drawCenterString("TL", indicator_x, indicator_y - 15);
    } else if (speedcam.vmax[0] != '\0' && speedcam.vmax[0] != '/') {
        // Mostra limite velocità - usa font normale
        setNormalFont();
        display->setTextColor(COLOR_BLACK);
        display->drawCenterString(speedcam.vmax, indicator_x, indicator_y - 13);
    }
}

void DisplayController::drawGPSInfo() {
    if (!display) return;
    
    // Pulisci solo l'area delle info GPS (righe 170-200) per evitare artefatti
    display->fillRect(0, 170, DISPLAY_WIDTH, 35, COLOR_BLACK);
    
    // Informazioni GPS
    setSmallFont();
    display->setTextColor(COLOR_WHITE);
    display->setTextSize(1);
    
    // Centra la scritta "GPS: ..." orizzontalmente usando la larghezza dello schermo
    const char* gps_status_text = gps_has_fix ? "GPS: Fix OK" : "GPS: In attesa...";
    int16_t center_x = display->width() / 2;
    display->drawCenterString(gps_status_text, center_x, 190);

    if (gps_has_fix) {
        // Centra la scritta "Sat: X" rispetto allo schermo
        char sat_text[16];
        snprintf(sat_text, sizeof(sat_text), "Sat: %u", gps_satellites);
        display->drawCenterString(sat_text, center_x, 205);
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

    display->drawRoundRect(x, y, w, h, radius, color);
}

void DisplayController::drawRoundedRectFilled(int16_t x, int16_t y, int16_t w, int16_t h, int16_t radius, uint16_t color) {
    if (!display) return;

    display->fillRoundRect(x, y, w, h, radius, color);
}

void DisplayController::drawCircleWithBorder(int16_t x, int16_t y, int16_t radius, uint16_t fill_color, uint16_t border_color, int16_t border_width) {
    if (!display) return;

    // Due cerchi pieni sovrapposti invece di cerchi concentrici: disegnare
    // N outline con drawCircle() a raggi decrescenti lascia dei "buchi" agli
    // angoli (artefatto dell'algoritmo di Bresenham su raggi ravvicinati).
    // Un cerchio pieno del colore del bordo, con sopra uno più piccolo del
    // colore di riempimento, da' lo stesso effetto visivo senza artefatti.
    display->fillCircle(x, y, radius, border_color);
    if (radius - border_width > 0) {
        display->fillCircle(x, y, radius - border_width, fill_color);
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

void DisplayController::setNormalFont() {
    #ifdef CUSTOM_FONT_ENABLED
    #ifdef CUSTOM_FONT_NAME
    if (display) {
        // Usa font big (16px Black) come font normale
        display->setFont(&geist_font_big_font);
    }
    #endif
    #endif
}

void DisplayController::setMonoFont() {
    #ifdef CUSTOM_FONT_ENABLED
    #ifdef CUSTOM_FONT_MONO_NAME
    if (display) {
        // Usa riferimento diretto al font mono (geist_mono_font)
        display->setFont(&geist_mono_font);
    }
    #else
    // Se font mono non disponibile, usa font normale
    setNormalFont();
    #endif
    #else
    // Se font personalizzati non abilitati, usa font di sistema
    if (display) {
        display->setFont();
    }
    #endif
}

void DisplayController::setSmallFont() {
    #ifdef CUSTOM_FONT_ENABLED
    #ifdef CUSTOM_FONT_NAME
    if (display) {
        // Usa font small (8px Bold) per testo più piccolo
        display->setFont(&geist_font_small_font);
    }
    #endif
    #endif
}
