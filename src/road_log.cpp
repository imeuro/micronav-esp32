#include "road_log.h"
#include <LittleFS.h>
#include <stdarg.h>

namespace {
    const size_t BUF_SIZE = 1024;
    char buf[BUF_SIZE];
    size_t buf_len = 0;
    unsigned long last_flush = 0;
    bool ready = false;
    bool stopped_full = false;

    size_t freeBytes() {
        return LittleFS.totalBytes() - LittleFS.usedBytes();
    }

    void rotateIfNeeded() {
        File f = LittleFS.open(ROAD_LOG_PATH, "r");
        size_t size = f ? f.size() : 0;
        if (f) f.close();
        if (size < ROAD_LOG_MAX_BYTES) return;
        LittleFS.remove(ROAD_LOG_OLD_PATH);
        LittleFS.rename(ROAD_LOG_PATH, ROAD_LOG_OLD_PATH);
    }
}

namespace RoadLog {

void begin() {
#if ROAD_LOG_ENABLED
    ready = LittleFS.begin(false, "/littlefs", 5, "littlefs");
    stopped_full = false;
    buf_len = 0;
    last_flush = millis();
    if (ready) {
        event("BOOT t=%lu heap=%u free_fs=%u", millis(), (unsigned)ESP.getFreeHeap(), (unsigned)freeBytes());
        flush();
    }
#endif
}

void event(const char* fmt, ...) {
#if ROAD_LOG_ENABLED
    if (!ready || stopped_full) return;
    char line[200];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(line, sizeof(line) - 1, fmt, args);
    va_end(args);
    if (n <= 0) return;
    if (n > (int)sizeof(line) - 2) n = sizeof(line) - 2;
    line[n++] = '\n';

    if (buf_len + n > BUF_SIZE) flush();
    if (buf_len + n > BUF_SIZE) return;  // flush fallito
    memcpy(buf + buf_len, line, n);
    buf_len += n;
#endif
}

void flush() {
#if ROAD_LOG_ENABLED
    if (!ready || buf_len == 0) return;
    last_flush = millis();

    if (freeBytes() < ROAD_LOG_MIN_FREE_BYTES) {
        stopped_full = true;
        buf_len = 0;
        Serial.println("[RoadLog] Spazio libero insufficiente, log fermato");
        return;
    }
    rotateIfNeeded();
    File f = LittleFS.open(ROAD_LOG_PATH, "a");
    if (!f) return;
    f.write((const uint8_t*)buf, buf_len);
    f.close();
    buf_len = 0;
#endif
}

void update() {
#if ROAD_LOG_ENABLED
    if (buf_len > 0 && millis() - last_flush >= ROAD_LOG_FLUSH_MS) flush();
#endif
}

void dump(Print& s) {
#if ROAD_LOG_ENABLED
    flush();
    s.println("---ROADLOG-BEGIN---");
    const char* paths[2] = { ROAD_LOG_OLD_PATH, ROAD_LOG_PATH };
    for (int i = 0; i < 2; i++) {
        File f = LittleFS.open(paths[i], "r");
        if (!f) continue;
        uint8_t chunk[128];
        while (f.available()) {
            size_t n = f.read(chunk, sizeof(chunk));
            s.write(chunk, n);
            delay(1);  // lascia svuotare il buffer CDC
        }
        f.close();
    }
    s.println("---ROADLOG-END---");
#endif
}

void clear() {
#if ROAD_LOG_ENABLED
    buf_len = 0;
    LittleFS.remove(ROAD_LOG_PATH);
    LittleFS.remove(ROAD_LOG_OLD_PATH);
    stopped_full = false;
    Serial.println("[RoadLog] Log cancellato");
#endif
}

} // namespace RoadLog
