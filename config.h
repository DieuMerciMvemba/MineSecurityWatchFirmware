/**
 * @file      config.h
 * @brief     Configuration centrale du firmware Mine Security Watch
 * @author    Mine Security Watch Team
 * @date      2026-06-24
 *
 * ⚠️  MODIFIER CE FICHIER avant le déploiement sur le terrain.
 *     Ne jamais commiter les credentials WiFi en production.
 */

#pragma once

// ============================================================
//  Identité de la montre / mineur
// ============================================================
#define CFG_WORKER_ID       "M001"
#define CFG_WORKER_NAME     "Israel Lum"
#define CFG_WORKER_ZONE     "Zone B"
#define CFG_SITE_NAME       "Mine Katanga"

// ============================================================
//  WiFi
// ============================================================
#define CFG_WIFI_SSID       "LUMEYA"
#define CFG_WIFI_PASSWORD   "00000000"

// ============================================================
//  Serveur API REST
// ============================================================
#define CFG_API_HOST        "10.15.202.36"     // IP du serveur
#define CFG_API_PORT        3000
#define CFG_API_ENDPOINT    "/api/watch/data"
#define CFG_API_SOS         "/api/watch/sos"
#define CFG_API_TOKEN       "msw_token_2026"    // Optionnel

// ============================================================
//  LoRa SX1262
// ============================================================
#define CFG_LORA_FREQ       868.0       // MHz (868 EU / 915 US)
#define CFG_LORA_BW         125.0       // kHz
#define CFG_LORA_SF         9           // Spreading Factor (7–12)
#define CFG_LORA_CR         5           // Coding Rate (5 = 4/5)
#define CFG_LORA_PREAMBLE   8
#define CFG_LORA_POWER      14          // dBm
#define CFG_LORA_MY_ADDR    0x01        // Adresse LoRa de cette montre
#define CFG_LORA_BASE_ADDR  0xFF        // Adresse station de base

// ============================================================
//  Timings (ms)
// ============================================================
#define CFG_API_INTERVAL        30000   // Envoi données toutes les 30s
#define CFG_SENSOR_INTERVAL      1000   // Lecture capteurs toutes les 1s
#define CFG_WIFI_RECONNECT       5000   // Tentative reconnexion WiFi
#define CFG_LORA_HEARTBEAT      60000   // Heartbeat LoRa toutes les 60s
#define CFG_SOS_HOLD_MS          2000   // Appui long SOS = 2s
#define CFG_DISPLAY_TIMEOUT     30000   // Écran s'éteint après 30s
#define CFG_SLEEP_TIMEOUT       60000   // Light sleep après 60s

// ============================================================
//  Seuils
// ============================================================
#define CFG_FALL_THRESHOLD      2.5f    // g – détection chute
#define CFG_LOW_BATTERY         20      // % – alerte batterie faible
#define CFG_CRIT_BATTERY        10      // % – arrêt imminent

// ============================================================
//  FreeRTOS – Tailles des stacks (mots de 4 octets)
// ============================================================
#define CFG_STACK_UI            6144
#define CFG_STACK_SENSORS       4096
#define CFG_STACK_WIFI          4096
#define CFG_STACK_API           6144
#define CFG_STACK_LORA          4096
#define CFG_STACK_ALERTS        2048

// ============================================================
//  FreeRTOS – Priorités (0 = plus faible, 5 = plus haute)
// ============================================================
#define CFG_PRIO_UI             3
#define CFG_PRIO_SENSORS        4
#define CFG_PRIO_WIFI           2
#define CFG_PRIO_API            2
#define CFG_PRIO_LORA           3
#define CFG_PRIO_ALERTS         0   // Alertes = priorité maximale

// ============================================================
//  Numérotation des écrans
// ============================================================
#define SCREEN_HOME     0
#define SCREEN_HEALTH   1
#define SCREEN_NETWORK  2
#define SCREEN_ALERTS   3
#define SCREEN_SOS      4   // Écran plein rouge SOS/chute
#define SCREEN_GPS      5   // Écran GPS tracking
#define SCREEN_NFC      6   // Écran NFC authentification
#define SCREEN_COMPASS  7   // Écran boussole
#define SCREEN_POWER    8   // Écran gestion énergie
#define SCREEN_RADIO    9   // Écran walkie-talkie
