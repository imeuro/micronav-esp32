#ifndef HEADING_TRACKER_H
#define HEADING_TRACKER_H

// Stima dell'heading di marcia: course GPS se affidabile, altrimenti bearing
// tra fix distanti, con hold temporizzato. Nessuna dipendenza da Arduino.

#include "config.h"
#include "geo_utils.h"

class HeadingTracker {
public:
    HeadingTracker() :
        has_anchor(false), anchor_lat(0.0), anchor_lng(0.0),
        has_heading(false), heading(0.0f), last_valid_ms(0), valid(false) {}

    void reset() { *this = HeadingTracker(); }

    // Da chiamare a ogni aggiornamento di posizione valida.
    // course_valid: il GPS ha fornito un course valido e recente (RMC).
    void update(unsigned long now_ms, double lat, double lng,
                float speed_kmh, bool course_valid, float course_deg) {
        const bool moving = speed_kmh >= SPEEDCAM_MIN_SPEED_KMH;

        if (!has_anchor) {
            anchor_lat = lat;
            anchor_lng = lng;
            has_anchor = true;
        } else if (geo_distance_m(anchor_lat, anchor_lng, lat, lng) >= SPEEDCAM_HEADING_FALLBACK_MIN_M) {
            if (moving && !course_valid) {
                setHeading((float)geo_bearing_deg(anchor_lat, anchor_lng, lat, lng), now_ms);
            }
            anchor_lat = lat;
            anchor_lng = lng;
        }

        if (moving && course_valid) {
            setHeading(course_deg, now_ms);
        }

        valid = has_heading && (now_ms - last_valid_ms) <= (unsigned long)SPEEDCAM_HEADING_HOLD_MS;
    }

    bool isValid() const { return valid; }
    float getCourse() const { return heading; }

private:
    bool has_anchor;
    double anchor_lat, anchor_lng;
    bool has_heading;
    float heading;
    unsigned long last_valid_ms;
    bool valid;

    void setHeading(float deg, unsigned long now_ms) {
        heading = (float)fmod((double)deg + 360.0, 360.0);
        has_heading = true;
        last_valid_ms = now_ms;
    }
};

#endif // HEADING_TRACKER_H
