#ifndef SPEEDCAM_TRACKER_H
#define SPEEDCAM_TRACKER_H

// State machine di tracciamento del POI rilevante (CANDIDATE -> PASSED).
// Nessuna dipendenza da Arduino: il tipo POI e' un template che espone id, lat, lng.

#include <stdint.h>
#include "config.h"
#include "geo_utils.h"

enum TrackerHideReason {
    HIDE_NONE = 0,
    HIDE_BEHIND = 1,     // POI alle spalle (angolo oltre SPEEDCAM_PASSED_ANGLE_DEG)
    HIDE_RECEDING = 2,   // distanza cresciuta oltre min_distance + isteresi
    HIDE_GONE = 3,       // POI non piu' in database (ricarica)
    HIDE_LOST = 4        // POI uscito da cono/raggio o heading perso
};

struct TrackerResult {
    int index;       // indice del POI da mostrare, -1 se nessuno
    float distance;  // distanza dal POI da mostrare
    bool hide;       // true se l'alert mostrato va nascosto (nessun POI da mostrare)
    uint32_t hide_id;      // id del POI che ha smesso di essere tracciato (se hide_reason != 0)
    int hide_reason;       // TrackerHideReason
};

// Semiangolo del cono in funzione della distanza: stretto lontano (la strada percorsa
// e' quasi in asse col POI), largo vicino. Limita gli alert su strade laterali.
inline double speedcam_cone_half_angle(double distance_m) {
    double a = atan2((double)SPEEDCAM_CONE_LATERAL_M, distance_m) * (180.0 / M_PI);
    if (a < SPEEDCAM_CONE_MIN_DEG) a = SPEEDCAM_CONE_MIN_DEG;
    if (a > SPEEDCAM_CONE_HALF_ANGLE_DEG) a = SPEEDCAM_CONE_HALF_ANGLE_DEG;
    return a;
}

class SpeedcamTracker {
public:
    SpeedcamTracker() : tracking(false), tracked_id(0), min_distance(0.0f),
                        has_passed(false), passed_id(0) {}

    void reset() { *this = SpeedcamTracker(); }
    bool isTracking() const { return tracking; }

    template <typename T>
    TrackerResult update(const T* cams, int count, double lat, double lng,
                         bool heading_valid, float course, float radius) {
        TrackerResult result = {-1, 0.0f, false, 0, HIDE_NONE};

        // 1. Un POI superato torna eleggibile solo uscendo da radius * isteresi
        if (has_passed) {
            int pi = indexOf(cams, count, passed_id);
            if (pi < 0 || geo_distance_m(lat, lng, cams[pi].lat, cams[pi].lng) > radius * SPEEDCAM_RELEASE_FACTOR) {
                has_passed = false;
            }
        }

        // 2. POI corrente: ancora in avvicinamento o superato?
        if (tracking) {
            int ti = indexOf(cams, count, tracked_id);
            if (ti < 0) {
                tracking = false;
                result.hide = true;
                result.hide_id = tracked_id;
                result.hide_reason = HIDE_GONE;
            } else {
                float d = (float)geo_distance_m(lat, lng, cams[ti].lat, cams[ti].lng);
                if (d < min_distance) min_distance = d;

                bool behind = heading_valid && d > SPEEDCAM_NEAR_FIELD_M &&
                    geo_angle_diff_deg(course, geo_bearing_deg(lat, lng, cams[ti].lat, cams[ti].lng)) > SPEEDCAM_PASSED_ANGLE_DEG;
                bool receding = d > min_distance + SPEEDCAM_PASSED_HYSTERESIS_M;

                if (behind || receding) {
                    has_passed = true;
                    passed_id = tracked_id;
                    tracking = false;
                    result.hide = true;
                    result.hide_id = tracked_id;
                    result.hide_reason = behind ? HIDE_BEHIND : HIDE_RECEDING;
                }
            }
        }

        // 3. Selezione: il piu' vicino tra quelli nel raggio e nel cono, escluso il superato
        const double lat_span_deg = radius / GEO_M_PER_DEG_LAT;
        const double lng_span_deg = lat_span_deg / fmax(cos(geo_deg_to_rad(lat)), 0.01);
        int best = -1;
        float best_distance = radius + 1.0f;

        for (int i = 0; i < count; i++) {
            const T& sc = cams[i];
            if (has_passed && sc.id == passed_id) continue;
            if (!(sc.lat == sc.lat) || !(sc.lng == sc.lng)) continue;  // NaN

            // Pre-reject economico prima di Haversine
            if (fabs(sc.lat - lat) > lat_span_deg || fabs(sc.lng - lng) > lng_span_deg) continue;

            float d = (float)geo_distance_m(lat, lng, sc.lat, sc.lng);
            if (d > radius || d >= best_distance) continue;

            // Direzione del POI (anche entro near-field: la carreggiata opposta e' vicina)
            if (sc.dir != SPEEDCAM_DIR_NONE) {
                if (heading_valid) {
                    double poi_dir = fmod((double)sc.dir + SPEEDCAM_DIR_OFFSET_DEG, 360.0);
                    if (geo_angle_diff_deg(course, poi_dir) > SPEEDCAM_DIR_TOLERANCE_DEG) continue;
                } else if (!(tracking && sc.id == tracked_id)) {
                    continue;  // senza heading non si puo' verificare la direzione
                }
            }

            if (d > SPEEDCAM_NEAR_FIELD_M) {
                if (heading_valid) {
                    if (!geo_in_cone(course, geo_bearing_deg(lat, lng, sc.lat, sc.lng), speedcam_cone_half_angle(d))) continue;
                } else if (!(tracking && sc.id == tracked_id)) {
                    continue;  // heading ignoto: nessun nuovo alert, si segue solo il POI corrente
                }
            }

            best = i;
            best_distance = d;
        }

        if (best >= 0) {
            if (!tracking || tracked_id != cams[best].id) {
                tracking = true;
                tracked_id = cams[best].id;
                min_distance = best_distance;
            }
            result.index = best;
            result.distance = best_distance;
            result.hide = false;
        } else if (tracking) {
            // Il POI corrente non e' piu' valido (fuori cono/raggio): alert via
            tracking = false;
            result.hide = true;
            result.hide_id = tracked_id;
            result.hide_reason = HIDE_LOST;
        }

        return result;
    }

private:
    bool tracking;
    uint32_t tracked_id;
    float min_distance;
    bool has_passed;
    uint32_t passed_id;

    template <typename T>
    static int indexOf(const T* cams, int count, uint32_t id) {
        for (int i = 0; i < count; i++) {
            if (cams[i].id == id) return i;
        }
        return -1;
    }
};

#endif // SPEEDCAM_TRACKER_H
