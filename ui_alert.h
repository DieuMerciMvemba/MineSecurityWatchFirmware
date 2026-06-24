/**
 * @file      ui_alert.h
 * @brief     Écran Alertes + Écran SOS/Chute plein rouge
 * @author    Mine Security Watch Team
 * @date      2026-06-24
 */

#pragma once

#include <lvgl.h>
#include "data_model.h"

/**
 * @brief Crée l'écran liste des alertes.
 */
lv_obj_t* uiAlertCreate();

/**
 * @brief Met à jour la liste des alertes.
 * @param alerts  Tableau d'alertes
 * @param count   Nombre d'alertes valides
 */
void uiAlertUpdate(const AlertMessage *alerts, int count);

/**
 * @brief Ajoute une alerte à l'écran.
 */
void uiAlertAdd(const AlertMessage &alert);

lv_obj_t* uiAlertGetScreen();

// -------------------------------------------------------
//  Écran plein rouge – SOS / Chute
// -------------------------------------------------------

/**
 * @brief Crée l'écran d'urgence (fond rouge, texte SOS ou CHUTE).
 */
lv_obj_t* uiEmergencyCreate();

/**
 * @brief Affiche l'écran SOS.
 */
void uiEmergencyShowSOS();

/**
 * @brief Affiche l'écran chute.
 */
void uiEmergencyShowFall();

/**
 * @brief Masque l'écran d'urgence (retour normal).
 */
void uiEmergencyHide();

lv_obj_t* uiEmergencyGetScreen();
