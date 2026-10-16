#include "utils.h"

double deg_to_rad(double deg) {
    return deg * (M_PI / 180.0);
}

double calculate_distance(double lat1, double lon1, double lat2, double lon2) {
    return geo_distance_m(lat1, lon1, lat2, lon2);
}

bool is_valid_float(float value) {
    return !isnan(value) && !isinf(value) && value != 0.0;
}

float clamp(float value, float min_val, float max_val) {
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}
