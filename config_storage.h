/**
 * @file      config_storage.h
 * @brief     Gestion de la persistance de la configuration en NVS (Preferences)
 * @author    Mine Security Watch Team
 * @date      2026-06-25
 */

#pragma once
#include <Arduino.h>

struct SystemConfig {
    char wifiSsid[33];
    char wifiPassword[65];
    char apiHost[65];
    uint16_t apiPort;
    char apiEndpoint[65];
    char apiSosEndpoint[65];
    char workerName[33];
    char workerId[17];
    char workerZone[17];
    char siteName[33];
    bool apEnabled;
};

extern SystemConfig g_config;

/**
 * @brief Initialise le stockage de configuration, charge les valeurs ou applique les valeurs par défaut.
 */
void configInit();

/**
 * @brief Charge la configuration depuis les Preferences (NVS).
 */
void configLoad();

/**
 * @brief Sauvegarde la configuration courante dans les Preferences (NVS).
 */
void configSave();

/**
 * @brief Réinitialise la configuration aux valeurs par défaut de config.h.
 */
void configResetDefaults();
