#ifndef UI_NFC_H
#define UI_NFC_H

#include <lvgl.h>

// Créer l'écran NFC
lv_obj_t* uiNfcCreate(lv_obj_t *parent = nullptr);

// Mettre à jour le statut NFC
void uiNfcUpdateStatus(const char* status);

// Afficher popup de détection NFC
void uiNfcShowPopup(const char* tagId, bool checkIn);

// Afficher l'écran NFC
void uiNfcShow();

// Masquer l'écran NFC
void uiNfcHide();

#endif // UI_NFC_H
