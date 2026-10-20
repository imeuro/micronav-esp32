// Test host (senza Arduino): geometria, heading, state machine.
// Build: g++ -std=c++17 -DMICRONAV_HOST_TEST -I src tests/host/test_detection.cpp -o /tmp/test_detection && /tmp/test_detection
#include <cstdio>
#include <cmath>
#include <vector>
#include "heading_tracker.h"
#include "speedcam_tracker.h"

struct Poi { uint32_t id; float lat; float lng; uint16_t dir = SPEEDCAM_DIR_NONE; };

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)

// Sposta un punto di dist_m lungo bearing_deg
static void offset(double lat, double lng, double bearing_deg, double dist_m, double& olat, double& olng) {
    double b = geo_deg_to_rad(bearing_deg);
    olat = lat + (dist_m * cos(b)) / GEO_M_PER_DEG_LAT;
    olng = lng + (dist_m * sin(b)) / (GEO_M_PER_DEG_LAT * cos(geo_deg_to_rad(lat)));
}

static const double LAT0 = 45.0, LNG0 = 9.0;
static const float RADIUS = 1000.0f;

static void test_geometry() {
    CHECK(fabs(geo_bearing_deg(45, 9, 46, 9) - 0.0) < 0.01);
    CHECK(fabs(geo_bearing_deg(45, 9, 45, 10) - 90.0) < 0.5);
    CHECK(fabs(geo_bearing_deg(45, 9, 44, 9) - 180.0) < 0.01);
    CHECK(fabs(geo_bearing_deg(45, 9, 45, 8) - 270.0) < 0.5);
    CHECK(geo_angle_diff_deg(350, 10) == 20);
    CHECK(geo_angle_diff_deg(10, 350) == 20);
    CHECK(geo_angle_diff_deg(0, 180) == 180);
    CHECK(geo_in_cone(0, 59, 60));
    CHECK(!geo_in_cone(0, 61, 60));
    CHECK(geo_in_cone(350, 40, 60));
    double olat, olng;
    offset(LAT0, LNG0, 90, 500, olat, olng);
    CHECK(fabs(geo_distance_m(LAT0, LNG0, olat, olng) - 500) < 2);
}

static void test_heading() {
    HeadingTracker h;
    h.update(0, LAT0, LNG0, 50, true, 90);
    CHECK(h.isValid() && h.getCourse() == 90);
    // rallentamento: hold
    h.update(5000, LAT0, LNG0, 2, true, 0);
    CHECK(h.isValid() && h.getCourse() == 90);
    // hold scaduto
    h.update(11000, LAT0, LNG0, 2, true, 0);
    CHECK(!h.isValid());
    // course non valido ma in movimento: bearing tra fix
    HeadingTracker g;
    double lat2, lng2;
    g.update(0, LAT0, LNG0, 50, false, 0);
    CHECK(!g.isValid());
    offset(LAT0, LNG0, 180, 30, lat2, lng2);
    g.update(1000, lat2, lng2, 50, false, 0);
    CHECK(g.isValid() && fabs(g.getCourse() - 180) < 1);
    // fermo con course rumoroso: mai heading nuovo
    HeadingTracker f;
    f.update(0, LAT0, LNG0, 0, true, 123);
    CHECK(!f.isValid());
}

// Percorre una strada lungo `course` partendo a start_m prima del POI, a passi di step_m
struct Drive {
    SpeedcamTracker t;
    std::vector<Poi> pois;
    double lat, lng;
    float course;
    bool heading_valid;
    Drive(double la, double ln, float c) : lat(la), lng(ln), course(c), heading_valid(true) {}
    TrackerResult step(double dist_m) {
        offset(lat, lng, course, dist_m, lat, lng);
        return t.update(pois.data(), (int)pois.size(), lat, lng, heading_valid, course, RADIUS);
    }
};

static void test_frontal_then_passed() {
    // strada verso est, POI a 800 m davanti
    double plat, plng;
    offset(LAT0, LNG0, 90, 800, plat, plng);
    Drive d(LAT0, LNG0, 90);
    d.pois.push_back({1, (float)plat, (float)plng});

    bool shown = false, hidden_after_pass = false;
    int shows_after_pass = 0;
    bool passed = false;
    for (int i = 0; i < 40; i++) {  // 40 passi da 40 m: 1600 m
        TrackerResult r = d.step(40);
        double dist = geo_distance_m(d.lat, d.lng, plat, plng);
        double along = geo_bearing_deg(d.lat, d.lng, plat, plng);
        bool behind = geo_angle_diff_deg(90, along) > 90;
        if (behind) passed = true;
        if (r.index >= 0) { shown = true; if (passed && dist > 40 + 25) shows_after_pass++; }
        if (passed && r.hide) hidden_after_pass = true;
    }
    CHECK(shown);
    CHECK(hidden_after_pass);
    CHECK(shows_after_pass == 0);
}

static void test_opposite_and_behind() {
    // POI dietro: strada verso est, POI 500 m a ovest
    double plat, plng;
    offset(LAT0, LNG0, 270, 500, plat, plng);
    Drive d(LAT0, LNG0, 90);
    d.pois.push_back({1, (float)plat, (float)plng});
    TrackerResult r = d.t.update(d.pois.data(), 1, d.lat, d.lng, true, 90, RADIUS);
    CHECK(r.index < 0 && !r.hide);

    // Direzione opposta (verso ovest) con POI davanti a est: dietro di noi
    Drive o(LAT0, LNG0, 270);
    offset(LAT0, LNG0, 90, 500, plat, plng);
    o.pois.push_back({2, (float)plat, (float)plng});
    r = o.t.update(o.pois.data(), 1, o.lat, o.lng, true, 270, RADIUS);
    CHECK(r.index < 0);
}

static void test_parallel_road() {
    // Strada parallela a 600 m a nord, POI nel raggio ma laterale (angolo ~ 90)
    double plat, plng, tlat, tlng;
    offset(LAT0, LNG0, 0, 600, tlat, tlng);
    offset(tlat, tlng, 90, 50, plat, plng);
    SpeedcamTracker t;
    Poi p{3, (float)plat, (float)plng};
    TrackerResult r = t.update(&p, 1, LAT0, LNG0, true, 90, RADIUS);
    CHECK(r.index < 0);
}

static void test_behind_and_ahead() {
    // POI dietro a 200 m, POI davanti a 700 m: deve vincere quello davanti
    double blat, blng, alat, alng;
    offset(LAT0, LNG0, 270, 200, blat, blng);
    offset(LAT0, LNG0, 90, 700, alat, alng);
    SpeedcamTracker t;
    Poi p[2] = {{10, (float)blat, (float)blng}, {11, (float)alat, (float)alng}};
    TrackerResult r = t.update(p, 2, LAT0, LNG0, true, 90, RADIUS);
    CHECK(r.index == 1);
}

static void test_no_realert_after_pass() {
    double plat, plng;
    offset(LAT0, LNG0, 90, 300, plat, plng);
    Drive d(LAT0, LNG0, 90);
    d.pois.push_back({1, (float)plat, (float)plng});
    int shows = 0;
    // arriva, passa a 5 m e prosegue fino a 600 m oltre; poi un cambio di heading (curva) lo riporta in cono
    for (int i = 0; i < 30; i++) {
        TrackerResult r = d.step(30);
        if (r.index >= 0) shows++;
    }
    int shows_at_pass = shows;
    // Heading ruotato verso il POI: e' ancora dentro raggio*1.2, non deve riavvisare
    d.course = 270;
    TrackerResult r = d.t.update(d.pois.data(), 1, d.lat, d.lng, true, 270, RADIUS);
    CHECK(r.index < 0);
    CHECK(shows_at_pass > 0);
}

static void test_heading_unknown() {
    double plat, plng;
    offset(LAT0, LNG0, 90, 500, plat, plng);
    SpeedcamTracker t;
    Poi p{1, (float)plat, (float)plng};
    // heading ignoto: nessun nuovo alert
    TrackerResult r = t.update(&p, 1, LAT0, LNG0, false, 0, RADIUS);
    CHECK(r.index < 0);
    // heading valido: alert
    r = t.update(&p, 1, LAT0, LNG0, true, 90, RADIUS);
    CHECK(r.index == 0);
    // heading perso mentre si segue il POI: si continua sul trend di distanza
    double lat2, lng2;
    offset(LAT0, LNG0, 90, 100, lat2, lng2);
    r = t.update(&p, 1, lat2, lng2, false, 0, RADIUS);
    CHECK(r.index == 0);
    // poi ci si allontana senza heading: superato
    offset(lat2, lng2, 90, 450, lat2, lng2);  // 50 m oltre il POI
    r = t.update(&p, 1, lat2, lng2, false, 0, RADIUS);
    offset(lat2, lng2, 90, 60, lat2, lng2);
    r = t.update(&p, 1, lat2, lng2, false, 0, RADIUS);
    CHECK(r.index < 0 && r.hide);
}

static void test_near_field() {
    // Entro near-field il cono non si applica: POI laterale a 20 m viene segnalato
    double plat, plng;
    offset(LAT0, LNG0, 0, 20, plat, plng);
    SpeedcamTracker t;
    Poi p{1, (float)plat, (float)plng};
    TrackerResult r = t.update(&p, 1, LAT0, LNG0, true, 90, RADIUS);
    CHECK(r.index == 0);
}

static void test_direction() {
    // POI a 20 m davanti, direzione 270 (carreggiata opposta) mentre andiamo verso est: scartato anche in near-field
    double plat, plng;
    offset(LAT0, LNG0, 90, 20, plat, plng);
    SpeedcamTracker t;
    Poi opp{1, (float)plat, (float)plng, 270};
    CHECK(t.update(&opp, 1, LAT0, LNG0, true, 90, RADIUS).index < 0);
    // stessa posizione, direzione 90 (la nostra): alert
    Poi same{2, (float)plat, (float)plng, 90};
    CHECK(t.update(&same, 1, LAT0, LNG0, true, 90, RADIUS).index == 0);
    // direzione entro tolleranza (curva): alert
    Poi curve{3, (float)plat, (float)plng, 140};
    SpeedcamTracker t2;
    CHECK(t2.update(&curve, 1, LAT0, LNG0, true, 90, RADIUS).index == 0);
    // senza direzione nel DB: comportamento invariato
    Poi nodir{4, (float)plat, (float)plng};
    SpeedcamTracker t3;
    CHECK(t3.update(&nodir, 1, LAT0, LNG0, true, 90, RADIUS).index == 0);
    // heading ignoto con direzione nota: nessun nuovo alert
    SpeedcamTracker t4;
    CHECK(t4.update(&same, 1, LAT0, LNG0, false, 0, RADIUS).index < 0);
}

static void test_narrowing_cone() {
    // POI a 400 m in avanti con scarto laterale 100 m (angolo ~14 gradi, limite ~20): alert
    double plat, plng, tlat, tlng;
    offset(LAT0, LNG0, 90, 400, tlat, tlng);
    offset(tlat, tlng, 0, 100, plat, plng);
    SpeedcamTracker t;
    Poi near_axis{1, (float)plat, (float)plng};
    CHECK(t.update(&near_axis, 1, LAT0, LNG0, true, 90, RADIUS).index == 0);

    // Strada laterale: stesso POI a 400 m ma scarto laterale 250 m (angolo ~32, limite ~20): niente alert
    offset(tlat, tlng, 0, 250, plat, plng);
    Poi side{2, (float)plat, (float)plng};
    SpeedcamTracker t2;
    CHECK(t2.update(&side, 1, LAT0, LNG0, true, 90, RADIUS).index < 0);

    // Avvicinandosi la strada laterale a 250 m non entra mai in cono oltre il campo vicino
    Drive d(LAT0, LNG0, 90);
    offset(LAT0, LNG0, 90, 800, tlat, tlng);
    offset(tlat, tlng, 0, 250, plat, plng);
    d.pois.push_back({3, (float)plat, (float)plng});
    bool any = false;
    for (int i = 0; i < 40; i++) { if (d.step(40).index >= 0) any = true; }
    CHECK(!any);

    CHECK(speedcam_cone_half_angle(2000) == SPEEDCAM_CONE_MIN_DEG);
    CHECK(speedcam_cone_half_angle(10) == SPEEDCAM_CONE_HALF_ANGLE_DEG);
}

static void test_hide_reason_and_display() {
    double plat, plng;
    offset(LAT0, LNG0, 90, 300, plat, plng);
    Drive d(LAT0, LNG0, 90);
    d.pois.push_back({7, (float)plat, (float)plng});
    int reason = 0; uint32_t rid = 0;
    for (int i = 0; i < 20; i++) {
        TrackerResult r = d.step(30);
        if (r.hide_reason != HIDE_NONE) { reason = r.hide_reason; rid = r.hide_id; }
    }
    CHECK(reason == HIDE_BEHIND || reason == HIDE_RECEDING);
    CHECK(rid == 7);

    CHECK(geo_display_distance_m(870) == 900);
    CHECK(geo_display_distance_m(520) == 500);
    CHECK(geo_display_distance_m(480) == 500);
    CHECK(geo_display_distance_m(233) == 250);
    CHECK(geo_display_distance_m(97) == 100);
    CHECK(geo_display_distance_m(43) == 40);
    CHECK(geo_display_distance_m(4) == 0);

    // proiezione in avanti: 30 m lungo 90 gradi
    double lat2, lng2;
    geo_project(LAT0, LNG0, 90, 30, lat2, lng2);
    CHECK(fabs(geo_distance_m(LAT0, LNG0, lat2, lng2) - 30) < 0.5);
    CHECK(fabs(geo_bearing_deg(LAT0, LNG0, lat2, lng2) - 90) < 0.1);
}

int main() {
    test_narrowing_cone();
    test_hide_reason_and_display();
    test_direction();
    test_geometry();
    test_heading();
    test_frontal_then_passed();
    test_opposite_and_behind();
    test_parallel_road();
    test_behind_and_ahead();
    test_no_realert_after_pass();
    test_heading_unknown();
    test_near_field();
    if (failures == 0) printf("OK: tutti i test passati\n");
    return failures ? 1 : 0;
}
