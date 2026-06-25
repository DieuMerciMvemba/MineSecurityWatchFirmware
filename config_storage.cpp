/**
 * @file      config_storage.cpp
 * @brief     Implémentation de la gestion de la persistance de la configuration en NVS
 * @author    Mine Security Watch Team
 * @date      2026-06-25
 */

#include "config_storage.h"
#include "config.h"
#include <Preferences.h>

SystemConfig g_config;

void configInit() {
    Preferences prefs;
    prefs.begin("msw_config", false);
    
    bool initialized = prefs.getBool("init", false);
    prefs.end();

    if (!initialized) {
        Serial.println("[CONFIG] Première initialisation, écriture des valeurs par défaut...");
        configResetDefaults();
    } else {
        configLoad();
    }
}

void configLoad() {
    Preferences prefs;
    prefs.begin("msw_config", true); // Mode lecture seule

    prefs.getBytes("config", &g_config, sizeof(SystemConfig));
    
    prefs.end();
    Serial.printf("[CONFIG] Configuration chargée. Mineur: %s (%s), Zone: %s, Site: %s, AP: %s, API: %s:%d\n",
                  g_config.workerName, g_config.workerId, g_config.workerZone, g_config.siteName,
                  g_config.apEnabled ? "ON" : "OFF", g_config.apiHost, g_config.apiPort);
}

void configSave() {
    Preferences prefs;
    prefs.begin("msw_config", false); // Mode écriture

    prefs.putBytes("config", &g_config, sizeof(SystemConfig));
    prefs.putBool("init", true);

    prefs.end();
    Serial.println("[CONFIG] Configuration sauvegardée avec succès en NVS.");
}

void configResetDefaults() {
    memset(&g_config, 0, sizeof(SystemConfig));
    
    // Valeurs par défaut depuis config.h
    strncpy(g_config.wifiSsid, CFG_WIFI_SSID, sizeof(g_config.wifiSsid) - 1);
    strncpy(g_config.wifiPassword, CFG_WIFI_PASSWORD, sizeof(g_config.wifiPassword) - 1);
    strncpy(g_config.apiHost, CFG_API_HOST, sizeof(g_config.apiHost) - 1);
    g_config.apiPort = CFG_API_PORT;
    strncpy(g_config.apiEndpoint, CFG_API_ENDPOINT, sizeof(g_config.apiEndpoint) - 1);
    strncpy(g_config.apiSosEndpoint, CFG_API_SOS, sizeof(g_config.apiSosEndpoint) - 1);
    strncpy(g_config.workerName, CFG_WORKER_NAME, sizeof(g_config.workerName) - 1);
    strncpy(g_config.workerId, CFG_WORKER_ID, sizeof(g_config.workerId) - 1);
    strncpy(g_config.workerZone, CFG_WORKER_ZONE, sizeof(g_config.workerZone) - 1);
    strncpy(g_config.siteName, CFG_SITE_NAME, sizeof(g_config.siteName) - 1);
    g_config.apEnabled = false; // Désactivé par défaut
    
    configSave();
}
