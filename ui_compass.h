#ifndef UI_COMPASS_H
#define UI_COMPASS_H

#include <lvgl.h>

// Créer l'écran boussole
lv_obj_t* uiCompassCreate();

// Mettre à jour les données IMU
void uiCompassUpdate(float heading, float roll, float pitch);

// Afficher l'écran boussole
void uiCompassShow();

// Masquer l'écran boussole
void uiCompassHide();

#endif // UI_COMPASS_H
