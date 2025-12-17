#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include <math.h>

/**
 * Calcola distanza tra due punti geografici usando formula di Haversine
 * 
 * @param lat1 Latitudine primo punto (gradi decimali)
 * @param lon1 Longitudine primo punto (gradi decimali)
 * @param lat2 Latitudine secondo punto (gradi decimali)
 * @param lon2 Longitudine secondo punto (gradi decimali)
 * @return Distanza in metri
 */
double calculate_distance(double lat1, double lon1, double lat2, double lon2);

/**
 * Converte gradi in radianti
 */
double deg_to_rad(double deg);

/**
 * Verifica se un valore è valido (non NaN, non infinito)
 */
bool is_valid_float(float value);

/**
 * Limita un valore tra min e max
 */
float clamp(float value, float min_val, float max_val);

#endif // UTILS_H
