#include "json_parser.h"

JSONParser::JSONParser() {
}

JSONParser::~JSONParser() {
}

bool JSONParser::isLittleFSMounted() {
    // Specifica esplicitamente il nome della partizione "littlefs"
    // Primo tentativo: monta senza formattare
    if (LittleFS.begin(false, "/littlefs", 5, "littlefs")) {
        return true;
    }
    
    // Se il mount fallisce, prova a formattare e rimontare
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.println("[JSON] LittleFS non montato, tentativo di formattazione...");
    }
    #endif
    
    // Formatta e monta (true = formatOnFail)
    if (LittleFS.begin(true, "/littlefs", 5, "littlefs")) {
        return true;
    }
    
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.println("[JSON] ERRORE: Impossibile montare LittleFS");
    }
    #endif
    return false;
}

int JSONParser::loadFromFile(const char* filename, Speedcam* speedcams, int max_count) {
    // Specifica esplicitamente il nome della partizione "littlefs"
    // Primo tentativo: monta senza formattare
    if (!LittleFS.begin(false, "/littlefs", 5, "littlefs")) {
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            Serial.println("[JSON] LittleFS non montato, tentativo di formattazione...");
        }
        #endif
        
        // Se il mount fallisce, formatta e rimontare (true = formatOnFail)
        if (!LittleFS.begin(true, "/littlefs", 5, "littlefs")) {
            #ifdef DEBUG_ENABLED
            if (DEBUG_ENABLED) {
                Serial.println("[JSON] ERRORE: Impossibile montare LittleFS anche dopo formattazione");
                Serial.println("[JSON] Verifica che la partizione 'littlefs' sia presente nella tabella partizioni");
            }
            #endif
            return -1;
        }
        
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            Serial.println("[JSON] LittleFS formattato e montato con successo");
        }
        #endif
    }
    
    File file = LittleFS.open(filename, "r");
    if (!file) {
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            Serial.print("[JSON] File non trovato: ");
            Serial.println(filename);
        }
        #endif
        return -1;
    }
    
    // Leggi file in buffer (streaming per file grandi)
    // Per file molto grandi, usiamo streaming parser
    size_t file_size = file.size();
    
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.print("[JSON] Dimensione file: ");
        Serial.print(file_size);
        Serial.println(" bytes");
    }
    #endif
    
    // Alloca buffer per JSON (limite memoria)
    // Per file grandi, usiamo streaming
    const size_t buffer_size = min((size_t)8192, file_size);
    char* buffer = (char*)malloc(buffer_size + 1);
    
    if (!buffer) {
        file.close();
        return -1;
    }
    
    // Per file molto grandi, usiamo approccio streaming
    // Per ora, leggiamo tutto il file (se possibile)
    int loaded_count = 0;
    
    if (file_size <= buffer_size) {
        // File piccolo, leggilo tutto
        file.readBytes(buffer, file_size);
        buffer[file_size] = '\0';
        
        loaded_count = loadFromString(buffer, speedcams, max_count);
    } else {
        // File grande: usa parser streaming incrementale
        // Processa il JSON carattere per carattere senza caricare tutto in memoria
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            Serial.println("[JSON] File grande, uso parser streaming incrementale");
        }
        #endif
        
        // Usa ArduinoJson streaming con documento più grande per array
        // Calcola dimensione documento basata su max_count speedcam
        // Ogni speedcam richiede ~200-300 bytes nel documento JSON
        const size_t doc_size = min((size_t)65536, (size_t)(max_count * 300));
        
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            Serial.print("[JSON] Dimensione documento streaming: ");
            Serial.print(doc_size);
            Serial.println(" bytes");
        }
        #endif
        
        // Riavvia il file dall'inizio
        file.seek(0);
        
        // Usa JsonStreamingParser o deserializeJson con stream
        // ArduinoJson v7 supporta streaming nativo
        DynamicJsonDocument doc(doc_size);
        
        // Configura per ignorare overflow (processa solo quello che può)
        DeserializationError error = deserializeJson(doc, file);
        
        if (error && error.code() != DeserializationError::Ok) {
            #ifdef DEBUG_ENABLED
            if (DEBUG_ENABLED) {
                Serial.print("[JSON] Errore parsing: ");
                Serial.println(error.c_str());
                if (error.code() == DeserializationError::NoMemory) {
                    Serial.println("[JSON] Memoria insufficiente, provo con parser incrementale...");
                }
            }
            #endif
            
            // Se errore di memoria, prova parser incrementale manuale
            if (error.code() == DeserializationError::NoMemory) {
                free(buffer);
                file.seek(0);
                loaded_count = loadFromFileIncremental(file, speedcams, max_count);
                file.close();
                return loaded_count;
            }
            
            free(buffer);
            file.close();
            return -1;
        }
        
        JsonArray result = doc["result"];
        if (!result) {
            free(buffer);
            file.close();
            return -1;
        }
        
        for (JsonObject obj : result) {
            if (loaded_count >= max_count) break;
            
            if (parseSpeedcam(obj, speedcams[loaded_count])) {
                loaded_count++;
            }
        }
    }
    
    free(buffer);
    file.close();
    
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.print("[JSON] Speedcam caricate: ");
        Serial.println(loaded_count);
    }
    #endif
    
    return loaded_count;
}

int JSONParser::loadFromString(const char* json_string, Speedcam* speedcams, int max_count) {
    // Usa DynamicJsonDocument per parsing
    // Dimensione basata su dimensione stringa (max 32KB per sicurezza)
    const size_t doc_size = min((size_t)32768, (size_t)(strlen(json_string) * 1.2));
    DynamicJsonDocument doc(doc_size);
    
    DeserializationError error = deserializeJson(doc, json_string);
    
    if (error) {
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            Serial.print("[JSON] Errore parsing: ");
            Serial.println(error.c_str());
        }
        #endif
        return -1;
    }
    
    JsonArray result = doc["result"];
    if (!result) {
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            Serial.println("[JSON] Campo 'result' non trovato");
        }
        #endif
        return -1;
    }
    
    int loaded_count = 0;
    
    for (JsonObject obj : result) {
        if (loaded_count >= max_count) {
            #ifdef DEBUG_ENABLED
            if (DEBUG_ENABLED) {
                Serial.print("[JSON] Raggiunto limite massimo speedcam: ");
                Serial.println(max_count);
            }
            #endif
            break;
        }
        
        if (parseSpeedcam(obj, speedcams[loaded_count])) {
            loaded_count++;
        }
    }
    
    return loaded_count;
}

bool JSONParser::parseSpeedcam(JsonObject obj, Speedcam& speedcam) {
    // ID
    if (obj.containsKey("id")) {
        speedcam.id = obj["id"].as<uint32_t>();
    }
    
    // Coordinate
    if (obj.containsKey("lat") && obj.containsKey("lng")) {
        speedcam.lat = obj["lat"].as<float>();
        speedcam.lng = obj["lng"].as<float>();
    } else {
        return false;  // Coordinate obbligatorie
    }
    
    // Tipo
    if (obj.containsKey("type")) {
        safeStringCopy(speedcam.type, obj["type"].as<const char*>(), 4);
    }
    
    // Velocità massima
    if (obj.containsKey("vmax")) {
        safeStringCopy(speedcam.vmax, obj["vmax"].as<const char*>(), 4);
    }
    
    // Status
    if (obj.containsKey("status")) {
        const char* status_str = obj["status"].as<const char*>();
        if (status_str && strlen(status_str) > 0) {
            speedcam.status = status_str[0];
        }
    }
    
    // Art
    if (obj.containsKey("art")) {
        const char* art_str = obj["art"].as<const char*>();
        if (art_str && strlen(art_str) > 0) {
            speedcam.art = art_str[0];
        }
    }
    
    return true;
}

int JSONParser::filterByBoundingBox(Speedcam* speedcams, int count, 
                                   float min_lat, float max_lat, 
                                   float min_lng, float max_lng) {
    int filtered_count = 0;
    
    for (int i = 0; i < count; i++) {
        if (speedcams[i].lat >= min_lat && speedcams[i].lat <= max_lat &&
            speedcams[i].lng >= min_lng && speedcams[i].lng <= max_lng) {
            
            // Sposta speedcam valida all'inizio
            if (filtered_count != i) {
                speedcams[filtered_count] = speedcams[i];
            }
            filtered_count++;
        }
    }
    
    return filtered_count;
}

void JSONParser::safeStringCopy(char* dest, const char* src, size_t max_len) {
    if (!src || !dest) return;
    
    size_t len = strlen(src);
    if (len >= max_len) {
        len = max_len - 1;
    }
    
    strncpy(dest, src, len);
    dest[len] = '\0';
}

int JSONParser::loadFromFileIncremental(File& file, Speedcam* speedcams, int max_count) {
    // Parser incrementale: estrae ogni oggetto speedcam e lo parsa individualmente
    // Questo evita di dover caricare tutto il JSON in memoria
    
    int loaded_count = 0;
    bool found_result_array = false;
    int brace_depth = 0;
    int bracket_depth = 0;
    bool in_string = false;
    bool escape_next = false;
    
    // Buffer per oggetto JSON corrente (max 512 bytes per oggetto)
    const size_t obj_buffer_size = 512;
    char* obj_buffer = (char*)malloc(obj_buffer_size);
    if (!obj_buffer) {
        #ifdef DEBUG_ENABLED
        if (DEBUG_ENABLED) {
            Serial.println("[JSON] ERRORE: Memoria insufficiente per buffer oggetto");
        }
        #endif
        return -1;
    }
    
    int obj_pos = 0;
    bool in_object = false;
    
    // Cerca l'array "result"
    String search_buffer = "";
    const char* result_key = "\"result\"";
    int result_key_pos = 0;
    
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.println("[JSON] Parser incrementale: ricerca array 'result'...");
    }
    #endif
    
    // Leggi file carattere per carattere
    while (file.available() && loaded_count < max_count) {
        char c = file.read();
        
        // Cerca "result" prima di iniziare a parsare
        if (!found_result_array) {
            if (c == result_key[result_key_pos]) {
                result_key_pos++;
                if (result_key_pos == strlen(result_key)) {
                    // Trovato "result", ora cerca l'array [
                    result_key_pos = 0;
                    // Continua a leggere fino a trovare [
                    while (file.available()) {
                        char next_c = file.read();
                        if (next_c == '[') {
                            found_result_array = true;
                            #ifdef DEBUG_ENABLED
                            if (DEBUG_ENABLED) {
                                Serial.println("[JSON] Array 'result' trovato, inizio parsing oggetti...");
                            }
                            #endif
                            break;
                        } else if (next_c == ' ' || next_c == '\t' || next_c == '\n' || next_c == '\r' || next_c == ':') {
                            continue;  // Ignora whitespace e :
                        } else {
                            // Non è quello che cercavamo, riprova
                            result_key_pos = 0;
                            break;
                        }
                    }
                    continue;
                }
            } else {
                result_key_pos = 0;
            }
            continue;
        }
        
        // Gestisci escape in stringhe
        if (escape_next) {
            if (in_object && obj_pos < obj_buffer_size - 1) {
                obj_buffer[obj_pos++] = '\\';
                obj_buffer[obj_pos++] = c;
            }
            escape_next = false;
            continue;
        }
        
        if (c == '\\' && in_string) {
            escape_next = true;
            if (in_object && obj_pos < obj_buffer_size - 1) {
                obj_buffer[obj_pos++] = c;
            }
            continue;
        }
        
        // Gestisci stringhe
        if (c == '"') {
            in_string = !in_string;
            if (in_object && obj_pos < obj_buffer_size - 1) {
                obj_buffer[obj_pos++] = c;
            }
            continue;
        }
        
        if (in_string) {
            if (in_object && obj_pos < obj_buffer_size - 1) {
                obj_buffer[obj_pos++] = c;
            }
            continue;
        }
        
        // Gestisci strutture JSON
        if (c == '{') {
            brace_depth++;
            if (brace_depth == 1) {
                // Inizio nuovo oggetto speedcam
                in_object = true;
                obj_pos = 0;
                obj_buffer[obj_pos++] = c;
            } else if (in_object && obj_pos < obj_buffer_size - 1) {
                obj_buffer[obj_pos++] = c;
            }
        } else if (c == '}') {
            brace_depth--;
            if (in_object && obj_pos < obj_buffer_size - 1) {
                obj_buffer[obj_pos++] = c;
            }
            
            if (brace_depth == 0 && in_object) {
                // Fine oggetto speedcam, parsalo
                obj_buffer[obj_pos] = '\0';
                
                // Parsa oggetto con ArduinoJson
                DynamicJsonDocument doc(512);
                DeserializationError error = deserializeJson(doc, obj_buffer);
                
                if (!error) {
                    Speedcam sc;
                    if (parseSpeedcam(doc.as<JsonObject>(), sc)) {
                        // Verifica coordinate valide
                        if (sc.lat != 0.0 || sc.lng != 0.0) {
                            speedcams[loaded_count++] = sc;
                            
                            #ifdef DEBUG_ENABLED
                            if (DEBUG_ENABLED && loaded_count % 100 == 0) {
                                Serial.print("[JSON] Caricate ");
                                Serial.print(loaded_count);
                                Serial.println(" speedcam...");
                            }
                            #endif
                        }
                    }
                }
                
                in_object = false;
                obj_pos = 0;
            }
        } else if (c == '[') {
            bracket_depth++;
            if (in_object && obj_pos < obj_buffer_size - 1) {
                obj_buffer[obj_pos++] = c;
            }
        } else if (c == ']') {
            bracket_depth--;
            if (bracket_depth == 0) {
                // Fine array result
                break;
            }
            if (in_object && obj_pos < obj_buffer_size - 1) {
                obj_buffer[obj_pos++] = c;
            }
        } else {
            // Altri caratteri
            if (in_object && obj_pos < obj_buffer_size - 1) {
                obj_buffer[obj_pos++] = c;
            }
        }
    }
    
    free(obj_buffer);
    
    #ifdef DEBUG_ENABLED
    if (DEBUG_ENABLED) {
        Serial.print("[JSON] Parser incrementale completato: ");
        Serial.print(loaded_count);
        Serial.println(" speedcam caricate");
    }
    #endif
    
    return loaded_count;
}
