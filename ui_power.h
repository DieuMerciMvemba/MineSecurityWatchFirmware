#ifndef UI_POWER_H
#define UI_POWER_H

#include <lvgl.h>

// Créer l'écran Power
lv_obj_t* uiPowerCreate(lv_obj_t *parent = nullptr);

// Afficher l'écran Power
void uiPowerShow();

// Masquer l'écran Power
void uiPowerHide();

// Entrer en mode sleep
void uiPowerSleep();

// Entrer en mode éco
void uiPowerEcoMode(bool enable);

// Shutdown
void uiPowerShutdown();

#endif // UI_POWER_H
