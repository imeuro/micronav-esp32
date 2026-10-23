#ifndef ALERT_LEVEL_H
#define ALERT_LEVEL_H

// Colore di sfondo dell'alert in base a velocita' GPS e limite del POI.
// Nessuna dipendenza da Arduino, compilabile su host.

#include <stdlib.h>

enum AlertBgLevel {
    ALERT_BG_NONE = 0,    // nero: entro il limite, oppure nessun confronto possibile
    ALERT_BG_ORANGE = 1,  // oltre il limite di al massimo `margin_kmh`
    ALERT_BG_RED = 2      // oltre il limite di piu' di `margin_kmh`
};

// Limite in km/h dalla stringa vmax del POI ("50"); -1 se assente, "/" o non numerica
inline int alertLimitKmh(const char* vmax) {
    if (!vmax || vmax[0] < '0' || vmax[0] > '9') return -1;
    int v = atoi(vmax);
    return v > 0 ? v : -1;
}

// speed_valid false o limit_kmh < 0: nessun confronto
inline AlertBgLevel alertBackgroundLevel(float speed_kmh, bool speed_valid, int limit_kmh, int margin_kmh) {
    if (!speed_valid || limit_kmh < 0) return ALERT_BG_NONE;
    if (speed_kmh <= (float)limit_kmh) return ALERT_BG_NONE;
    if (speed_kmh <= (float)(limit_kmh + margin_kmh)) return ALERT_BG_ORANGE;
    return ALERT_BG_RED;
}

#endif
