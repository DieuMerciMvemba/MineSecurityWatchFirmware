#ifndef UI_RADIO_H
#define UI_RADIO_H

#include <lvgl.h>

// Créer l'écran Radio
lv_obj_t* uiRadioCreate();

// Mettre à jour le statut radio
void uiRadioUpdateStatus(const char* status);

// Mettre à jour les messages reçus
void uiRadioAddMessage(const char* message, const char* sender);

// Afficher l'écran Radio
void uiRadioShow();

// Masquer l'écran Radio
void uiRadioHide();

// Démarrer transmission PTT
void uiRadioStartPTT();

// Arrêter transmission PTT
void uiRadioStopPTT();

#endif // UI_RADIO_H
