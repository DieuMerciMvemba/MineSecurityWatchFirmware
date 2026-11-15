/**
 * @file      ble_beacon_service.cpp
 * @brief     Détection des balises Ulinzi Box avec lissage RSSI (EMA)
 *
 * Utilise le scan passif asynchrone 802.11 (WiFi.scanNetworks async) sur les balises
 * émises en continu par chaque Ulinzi Box (SSID "ULINZI-XXXXXX" / "BOX-XXX").
 * Avantage critique sur T-Watch S3 Plus :
 *   - 0 Ko de pile Bluedroid en SRAM interne (préserve 100 % du DMA SPI de l'écran ST7789 et des calques LVGL)
 *   - 0 tâche FreeRTOS supplémentaire (exécuté en machine à états non-bloquante dans wifiTask)
 */

#include "ble_beacon_service.h"
#include "config_storage.h"
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <string.h>
#include <strings.h>

static BleBeaconReading  s_beacons[MAX_TRACKED_BEACONS];
static SemaphoreHandle_t s_bleMutex   = nullptr;
static uint32_t          s_lastScanMs = 0;
static uint8_t           s_scanCycle  = 0;
static bool              s_enabled    = false;

static bool isUlinziSsid(const char *ssid) {
    if (!ssid || !ssid[0]) return false;
    return (strncasecmp(ssid, "ULINZI", 6) == 0 ||
            strncasecmp(ssid, "BOX-", 4) == 0);
}

static void recordBeacon(const char *id, int rssi) {
    if (!id || !id[0] || !s_bleMutex) return;
    uint32_t now = millis();

    if (xSemaphoreTake(s_bleMutex, pdMS_TO_TICKS(50)) != pdTRUE) return;

    int targetIdx = -1;
    int oldestIdx = 0;
    uint32_t oldestMs = UINT32_MAX;

    for (int i = 0; i < MAX_TRACKED_BEACONS; i++) {
        if (s_beacons[i].active && strncmp(s_beacons[i].id, id, sizeof(s_beacons[i].id)) == 0) {
            targetIdx = i;
            break;
        }
        if (!s_beacons[i].active && targetIdx == -1) {
            targetIdx = i;
        }
        if (s_beacons[i].lastSeenMs < oldestMs) {
            oldestMs = s_beacons[i].lastSeenMs;
            oldestIdx = i;
        }
    }

    if (targetIdx == -1) targetIdx = oldestIdx;

    BleBeaconReading &b = s_beacons[targetIdx];
    if (!b.active || strncmp(b.id, id, sizeof(b.id)) != 0 || (now - b.lastSeenMs) > BEACON_STALE_MS) {
        strncpy(b.id, id, sizeof(b.id) - 1);
        b.id[sizeof(b.id) - 1] = '\0';
        b.smoothedRssi = (float)rssi;
    } else {
        // Lissage exponentiel alpha=0.45 pour atténuer les réflexions en galerie
        b.smoothedRssi = 0.45f * (float)rssi + 0.55f * b.smoothedRssi;
    }
    b.lastRssi = (int8_t)rssi;
    b.lastSeenMs = now;
    b.active = true;

    xSemaphoreGive(s_bleMutex);
}

void bleScanInit() {
    memset(s_beacons, 0, sizeof(s_beacons));
    s_bleMutex = xSemaphoreCreateMutex();
    s_lastScanMs = millis();
    s_scanCycle = 0;
    Serial.println("[IPS] Détecteur de balises Ulinzi Box initialisé (0 Ko SRAM Bluedroid)");
}

void startBleScanTask() {
    s_enabled = true;
}

void bleScanPoll() {
    if (!s_enabled || !s_bleMutex || g_config.apEnabled) return;

    int16_t state = WiFi.scanComplete();
    if (state >= 0) {
        for (int16_t i = 0; i < state; i++) {
            String ssid = WiFi.SSID(i);
            if (isUlinziSsid(ssid.c_str())) {
                recordBeacon(ssid.c_str(), WiFi.RSSI(i));
            }
        }
        WiFi.scanDelete();
        s_lastScanMs = millis();
        return;
    }

    if (state == WIFI_SCAN_RUNNING) {
        return;
    }

    // Ne lancer un scan asynchrone que lorsque le Wi-Fi STA est connecté et stable
    uint32_t now = millis();
    if (WiFi.status() == WL_CONNECTED && (now - s_lastScanMs >= 3500)) {
        s_lastScanMs = now;
        s_scanCycle++;
        uint8_t ch = (s_scanCycle % 3 != 0) ? (uint8_t)WiFi.channel() : 0;
        // Scan asynchrone passif court (70 ms sur canal actif ou 50 ms/canal)
        WiFi.scanNetworks(true, false, true, (ch > 0 ? 70 : 50), ch);
    }
}

int bleGetTopBeacons(BleBeaconReading *out, int maxCount) {
    if (!out || maxCount <= 0 || !s_bleMutex) return 0;
    uint32_t now = millis();
    BleBeaconReading temp[MAX_TRACKED_BEACONS];
    int count = 0;

    if (xSemaphoreTake(s_bleMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        for (int i = 0; i < MAX_TRACKED_BEACONS; i++) {
            if (s_beacons[i].active) {
                if ((now - s_beacons[i].lastSeenMs) > BEACON_STALE_MS) {
                    s_beacons[i].active = false;
                } else {
                    temp[count++] = s_beacons[i];
                }
            }
        }
        xSemaphoreGive(s_bleMutex);
    }

    // Tri par RSSI décroissant (signal le plus fort en premier)
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (temp[j].smoothedRssi > temp[i].smoothedRssi) {
                BleBeaconReading swap = temp[i];
                temp[i] = temp[j];
                temp[j] = swap;
            }
        }
    }

    int n = count < maxCount ? count : maxCount;
    for (int i = 0; i < n; i++) out[i] = temp[i];
    return n;
}

void bleFormatBeaconsJson(char *outJson, size_t maxLen, int maxBeacons) {
    if (!outJson || maxLen < 3) return;
    BleBeaconReading top[MAX_TRACKED_BEACONS];
    int n = bleGetTopBeacons(top, maxBeacons > MAX_TRACKED_BEACONS ? MAX_TRACKED_BEACONS : maxBeacons);

    size_t pos = 0;
    pos += snprintf(outJson + pos, maxLen - pos, "[");
    for (int i = 0; i < n && pos < maxLen - 2; i++) {
        pos += snprintf(
            outJson + pos,
            maxLen - pos,
            "%s{\"id\":\"%s\",\"rssi\":%.1f}",
            (i > 0 ? "," : ""),
            top[i].id,
            top[i].smoothedRssi
        );
    }
    snprintf(outJson + pos, maxLen - pos, "]");
}
