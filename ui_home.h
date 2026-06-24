/**
 * @file      ui_home.h
 * @brief     Écran Dashboard principal – Mine Security Watch
 * @author    Mine Security Watch Team
 * @date      2026-06-24
 *
 * Affiche : nom mineur, heure, batterie, WiFi, LoRa,
 *           pas, température, bouton SOS.
 */

#pragma once

#include <lvgl.h>
#include "data_model.h"

/**
 * @brief Crée l'écran home (Dashboard).
 * @return Pointeur vers l'objet écran LVGL
 */
lv_obj_t* uiHomeCreate(lv_obj_t *parent = nullptr);

/**
 * @brief Met à jour les données affichées.
 * @param data  Données capteurs
 * @param net   État réseau
 */
void uiHomeUpdate(const SensorData &data, const NetworkStatus &net);

/**
 * @brief Met à jour l'heure affichée.
 * @param h Heures
 * @param m Minutes
 * @param s Secondes
 */
void uiHomeSetTime(int h, int m, int s);

/**
 * @brief Retourne le pointeur écran pour la navigation.
 */
lv_obj_t* uiHomeGetScreen();
