#ifndef UI_GPS_H
#define UI_GPS_H

#include <lvgl.h>

// Créer l'écran GPS
lv_obj_t* uiGpsCreate();

// Mettre à jour les données GPS
void uiGpsUpdate(double lat, double lng, float speed, uint8_t satellites);

// Afficher l'écran GPS
void uiGpsShow();

// Masquer l'écran GPS
void uiGpsHide();

#endif // UI_GPS_H
