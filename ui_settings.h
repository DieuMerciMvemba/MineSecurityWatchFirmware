/**
 * @file      ui_settings.h
 * @brief     Interface utilisateur LVGL des Paramètres (Verrouillage + Paramètres AP)
 * @author    Mine Security Watch Team
 * @date      2026-06-25
 */

#pragma once
#include <lvgl.h>

/**
 * @brief Crée l'écran des réglages (y compris la page de code d'accès)
 * @param parent L'objet parent (généralement une tuile du tileview)
 * @return L'objet conteneur principal de l'écran
 */
lv_obj_t* uiSettingsCreate(lv_obj_t *parent);

/**
 * @brief Met à jour l'affichage de l'écran des réglages
 * @param apActive Indique si l'AP est actif
 * @param apIP Adresse IP de l'AP
 * @param staIP Adresse IP de la Station (WiFi local)
 */
void uiSettingsUpdate(bool apActive, const char* apIP, const char* staIP);

/**
 * @brief Affiche l'écran de verrouillage des paramètres par défaut
 */
void uiSettingsResetLock();
