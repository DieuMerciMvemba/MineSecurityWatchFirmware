/**
 * @file      api_service.cpp
 * @brief     Implémentation API REST – HTTPClient ESP32
 * @author    Mine Security Watch Team
 * @date      2026-06-24
 *
 * Utilise la bibliothèque HTTPClient d'ESP32 Arduino.
 * Format JSON construit manuellement pour éviter ArduinoJson
 * et minimiser l'empreinte mémoire.
 */

#include "api_service.h"
#include "config.h"
#include "config_storage.h"
#include "sensor_service.h"
#include "wifi_service.h"
#include <LilyGoLib.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <time.h>

// ============================================================
//  Variables internes
// ============================================================
static bool s_apiOk = false;
static bool s_ntpSynced = false;
static uint32_t s_lastNtpSync = 0;

// ============================================================
//  Utilitaires internes
// ============================================================

/**
 * @brief Retourne l'horodatage ISO 8601 approximatif.
 *        Sans NTP, utilise millis() converti en secondes.
 */
static void getTimestamp(char *buf, size_t len) {
#ifdef LILYGO_WATCH_S3_PLUS
    RTC_DateTime dt = instance.rtc.getDateTime();
    snprintf(buf, len, "%04u-%02u-%02uT%02u:%02u:%02uZ",
             dt.getYear(), dt.getMonth(), dt.getDay(),
             dt.getHour(), dt.getMinute(), dt.getSecond());
#else
    uint32_t sec = millis() / 1000;
    uint32_t h   = (sec / 3600) % 24;
    uint32_t m   = (sec / 60) % 60;
    uint32_t s   = sec % 60;
    snprintf(buf, len, "2026-01-01T%02u:%02u:%02uZ", h, m, s);
#endif
}

/**
 * @brief Exécute un POST HTTP avec payload JSON.
 * @param endpoint  Chemin API (/api/watch/data)
 * @param payload   Corps JSON
 * @return true si succès (code 2xx)
 */
static bool httpPost(const char *endpoint, const String &payload) {
    if (!isWifiConnected()) {
        Serial.println("[API] WiFi non connecté, envoi ignoré");
        return false;
    }

    HTTPClient http;
    char url[128];
    snprintf(url, sizeof(url), "http://%s:%d%s",
             g_config.apiHost, g_config.apiPort, endpoint);

    Serial.printf("[API] POST %s\n", url);

    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    // Optionnel : token d'authentification
    // http.addHeader("Authorization", "Bearer " CFG_API_TOKEN);

    int code = http.POST(payload);
    http.end();

    if (code >= 200 && code < 300) {
        Serial.printf("[API] ✅ Réponse %d\n", code);
        s_apiOk = true;
        return true;
    } else {
        Serial.printf("[API] ❌ Erreur HTTP %d\n", code);
        s_apiOk = false;
        return false;
    }
}

// ============================================================
//  Implémentation publique
// ============================================================

void apiInit() {
    s_apiOk = false;
    Serial.println("[API] Service initialisé");
}

bool apiSendData(const SensorData &data) {
    char ts[32];
    getTimestamp(ts, sizeof(ts));

    char payload[512];
    snprintf(payload, sizeof(payload),
        "{"
        "\"workerId\":\"%s\","
        "\"battery\":%.1f,"
        "\"temperature\":%.1f,"
        "\"steps\":%ld,"
        "\"motion\":\"%s\","
        "\"lat\":%.6f,"
        "\"lng\":%.6f,"
        "\"timestamp\":\"%s\""
        "}",
        g_config.workerId,
        data.battery,
        data.temperature,
        data.steps,
        sensorMotionStr(data.motion),
        data.latitude,
        data.longitude,
        ts
    );

    return httpPost(g_config.apiEndpoint, String(payload));
}

bool apiSendSOS(const SensorData &data) {
    char ts[32];
    getTimestamp(ts, sizeof(ts));

    char payload[256];
    snprintf(payload, sizeof(payload),
        "{"
        "\"workerId\":\"%s\","
        "\"type\":\"SOS\","
        "\"battery\":%.1f,"
        "\"lat\":%.6f,"
        "\"lng\":%.6f,"
        "\"timestamp\":\"%s\""
        "}",
        g_config.workerId,
        data.battery,
        data.latitude,
        data.longitude,
        ts
    );

    Serial.println("[API] 🆘 Envoi SOS au serveur");
    return httpPost(g_config.apiSosEndpoint, String(payload));
}

bool apiSendFallAlert(const SensorData &data) {
    char ts[32];
    getTimestamp(ts, sizeof(ts));

    char payload[256];
    snprintf(payload, sizeof(payload),
        "{"
        "\"workerId\":\"%s\","
        "\"type\":\"FALL\","
        "\"battery\":%.1f,"
        "\"lat\":%.6f,"
        "\"lng\":%.6f,"
        "\"timestamp\":\"%s\""
        "}",
        g_config.workerId,
        data.battery,
        data.latitude,
        data.longitude,
        ts
    );

    Serial.println("[API] ⚠️  Envoi alerte CHUTE au serveur");
    return httpPost(g_config.apiSosEndpoint, String(payload));
}

bool apiIsConnected() {
    return s_apiOk;
}

// ============================================================
//  Tâche FreeRTOS – envoi périodique
// ============================================================
void apiTask(void *param) {
    Serial.println("[API] Tâche démarrée");

    for (;;) {
        if (isWifiConnected()) {
            SensorData data = sensorGetLatest();
            apiSendData(data);
        }
        vTaskDelay(pdMS_TO_TICKS(CFG_API_INTERVAL));
    }
}

void startApiTask() {
    xTaskCreatePinnedToCore(
        apiTask,
        "ApiTask",
        CFG_STACK_API,
        nullptr,
        CFG_PRIO_API,
        nullptr,
        0   // Core 0
    );
}

// ============================================================
//  Fonctions NTP
// ============================================================

bool apiSyncNTP() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[NTP] ❌ WiFi non connecté");
        return false;
    }

    Serial.println("[NTP] Synchronisation NTP...");
    
    // Configuration du serveur NTP
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    
    // Attendre la synchronisation
    int retry = 0;
    while (retry < 10) {
        time_t now = time(nullptr);
        if (now > 10000) {  // Si le timestamp est valide (> 1970)
            struct tm timeinfo;
            localtime_r(&now, &timeinfo);
            
#ifdef LILYGO_WATCH_S3_PLUS
            instance.rtc.setDateTime(timeinfo);
#endif
            
            char buffer[64];
            strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
            Serial.printf("[NTP] ✅ Sync réussi: %s\n", buffer);
            
            s_ntpSynced = true;
            s_lastNtpSync = millis();
            return true;
        }
        delay(500);
        retry++;
    }
    
    Serial.println("[NTP] ❌ Échec synchronisation");
    return false;
}

bool apiIsNtpSynced() {
    return s_ntpSynced;
}

uint32_t apiGetLastNtpSync() {
    return s_lastNtpSync;
}
