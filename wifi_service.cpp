/**
 * @file      wifi_service.cpp
 * @brief     Implémentation du service WiFi avec reconnexion automatique
 * @author    Mine Security Watch Team
 * @date      2026-06-24
 */

#include "wifi_service.h"
#include "config.h"
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// ============================================================
//  Variables internes
// ============================================================
static NetworkState s_wifiState = NET_DISCONNECTED;

// ============================================================
//  Implémentation
// ============================================================

void wifiInit() {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    Serial.println("[WIFI] Service initialisé");
    connectWifi();
}

bool connectWifi() {
    if (WiFi.isConnected()) {
        s_wifiState = NET_CONNECTED;
        return true;
    }

    s_wifiState = NET_CONNECTING;
    Serial.printf("[WIFI] Connexion à %s ...\n", CFG_WIFI_SSID);

    WiFi.begin(CFG_WIFI_SSID, CFG_WIFI_PASSWORD);

    uint32_t start = millis();
    while (!WiFi.isConnected() && (millis() - start) < 10000) {
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    if (WiFi.isConnected()) {
        s_wifiState = NET_CONNECTED;
        Serial.printf("[WIFI] ✅ Connecté – IP: %s\n", WiFi.localIP().toString().c_str());
        return true;
    } else {
        s_wifiState = NET_DISCONNECTED;
        Serial.println("[WIFI] ❌ Échec de connexion");
        return false;
    }
}

void reconnectWifi() {
    Serial.println("[WIFI] Reconnexion...");
    WiFi.disconnect(false);
    vTaskDelay(pdMS_TO_TICKS(500));
    connectWifi();
}

bool isWifiConnected() {
    return WiFi.isConnected();
}

String wifiGetIP() {
    if (WiFi.isConnected()) {
        return WiFi.localIP().toString();
    }
    return String("0.0.0.0");
}

NetworkState wifiGetState() {
    if (WiFi.isConnected()) s_wifiState = NET_CONNECTED;
    return s_wifiState;
}

// ============================================================
//  Tâche FreeRTOS – reconnexion automatique
// ============================================================
void wifiTask(void *param) {
    Serial.println("[WIFI] Tâche démarrée");

    for (;;) {
        if (!WiFi.isConnected()) {
            s_wifiState = NET_DISCONNECTED;
            Serial.println("[WIFI] Connexion perdue, tentative de reconnexion...");
            reconnectWifi();
        } else {
            s_wifiState = NET_CONNECTED;
        }
        vTaskDelay(pdMS_TO_TICKS(CFG_WIFI_RECONNECT));
    }
}

void startWifiTask() {
    xTaskCreatePinnedToCore(
        wifiTask,
        "WiFiTask",
        CFG_STACK_WIFI,
        nullptr,
        CFG_PRIO_WIFI,
        nullptr,
        0   // Core 0 – même core que la radio
    );
}
