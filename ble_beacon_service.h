/**
 * @file      ble_beacon_service.h
 * @brief     Détection légère des balises Ulinzi Box (Localisation Souterraine IPS)
 *            Sans pile Bluedroid en SRAM interne afin de préserver le DMA SPI de l'écran LVGL.
 */

#pragma once

#include <Arduino.h>

#define MAX_TRACKED_BEACONS 8
#define BEACON_STALE_MS     15000UL

struct BleBeaconReading {
    char     id[24];
    float    smoothedRssi;
    int8_t   lastRssi;
    uint32_t lastSeenMs;
    bool     active;
};

/**
 * @brief Initialise le module de suivi des balises Ulinzi Box.
 */
void bleScanInit();

/**
 * @brief Démarre le suivi périodique (géré de manière asynchrone dans la tâche Wi-Fi).
 */
void startBleScanTask();

/**
 * @brief Traitement non-bloquant du scan des balises Ulinzi Box (appelé depuis wifiTask).
 */
void bleScanPoll();

/**
 * @brief Récupère les balises Ulinzi Box actives triées par RSSI décroissant.
 */
int bleGetTopBeacons(BleBeaconReading *out, int maxCount);

/**
 * @brief Écrit le tableau JSON des balises dans outJson (ex: [{"id":"ULINZI-BOX-01","rssi":-64.0}]).
 */
void bleFormatBeaconsJson(char *outJson, size_t maxLen, int maxBeacons = 5);
