#include "utils.h"

// Raggio della Terra in metri
#define EARTH_RADIUS_M 6371000.0

double deg_to_rad(double deg) {
    return deg * (M_PI / 180.0);
}

double calculate_distance(double lat1, double lon1, double lat2, double lon2) {
    // Formula di Haversine per calcolare distanza tra due punti sulla sfera
    
    double lat1_rad = deg_to_rad(lat1);
    double lat2_rad = deg_to_rad(lat2);
    double delta_lat = deg_to_rad(lat2 - lat1);
    double delta_lon = deg_to_rad(lon2 - lon1);
    
    double a = sin(delta_lat / 2.0) * sin(delta_lat / 2.0) +
               cos(lat1_rad) * cos(lat2_rad) *
               sin(delta_lon / 2.0) * sin(delta_lon / 2.0);
    
    double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
    
    return EARTH_RADIUS_M * c;
}

bool is_valid_float(float value) {
    return !isnan(value) && !isinf(value) && value != 0.0;
}

float clamp(float value, float min_val, float max_val) {
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}
