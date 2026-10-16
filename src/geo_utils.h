#ifndef GEO_UTILS_H
#define GEO_UTILS_H

// Geometria geografica pura: nessuna dipendenza da Arduino, compilabile su host.

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define GEO_EARTH_RADIUS_M 6371000.0
#define GEO_M_PER_DEG_LAT 111320.0

inline double geo_deg_to_rad(double deg) {
    return deg * (M_PI / 180.0);
}

// Distanza in metri (Haversine)
inline double geo_distance_m(double lat1, double lon1, double lat2, double lon2) {
    double lat1_rad = geo_deg_to_rad(lat1);
    double lat2_rad = geo_deg_to_rad(lat2);
    double delta_lat = geo_deg_to_rad(lat2 - lat1);
    double delta_lon = geo_deg_to_rad(lon2 - lon1);

    double a = sin(delta_lat / 2.0) * sin(delta_lat / 2.0) +
               cos(lat1_rad) * cos(lat2_rad) *
               sin(delta_lon / 2.0) * sin(delta_lon / 2.0);
    double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
    return GEO_EARTH_RADIUS_M * c;
}

// Bearing iniziale (gradi, 0 = nord, orario) dal punto 1 al punto 2
inline double geo_bearing_deg(double lat1, double lon1, double lat2, double lon2) {
    double lat1_rad = geo_deg_to_rad(lat1);
    double lat2_rad = geo_deg_to_rad(lat2);
    double delta_lon = geo_deg_to_rad(lon2 - lon1);

    double y = sin(delta_lon) * cos(lat2_rad);
    double x = cos(lat1_rad) * sin(lat2_rad) -
               sin(lat1_rad) * cos(lat2_rad) * cos(delta_lon);
    double bearing = atan2(y, x) * (180.0 / M_PI);
    return fmod(bearing + 360.0, 360.0);
}

// Differenza angolare assoluta tra due direzioni, in [0, 180]
inline double geo_angle_diff_deg(double a, double b) {
    double diff = fmod(fabs(a - b), 360.0);
    return diff > 180.0 ? 360.0 - diff : diff;
}

// true se la direzione verso il POI e' entro il semiangolo del cono di marcia
inline bool geo_in_cone(double course_deg, double bearing_to_poi_deg, double half_angle_deg) {
    return geo_angle_diff_deg(course_deg, bearing_to_poi_deg) <= half_angle_deg;
}

#endif // GEO_UTILS_H
