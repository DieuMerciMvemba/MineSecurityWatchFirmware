/**
 * @file      MineSecurityWatch.ino
 * @brief     Firmware principal – Mine Security Watch
 *            LilyGO T-Watch S3 Plus / ESP32-S3
 * @author    Mine Security Watch Team
 * @date      2026-06-24
 * @version   1.0.0
 *
 * Architecture :
 *   - 4 écrans LVGL navigables par swipe (Home, Santé, Réseau, Alertes)
 *   - 1 écran d'urgence plein rouge (SOS / Chute)
 *   - 6 tâches FreeRTOS (UI, Capteurs, WiFi, API, LoRa, Alertes)
 *   - Détection de chute par accéléromètre
 *   - Bouton SOS (appui long 2s)
 *   - Envoi WiFi/API REST + LoRa
 *
 * Instructions de compilation Arduino IDE :
 *   1. Board   : LilyGO T-Watch S3 Plus (ou ESP32S3 Dev Module)
 *   2. Library : LilyGoLib (officielle)
 *   3. USB CDC on Boot : Enabled (pour Serial.print)
 *   4. Flash Size : 16MB
 *   5. Partition  : Huge APP (3MB No OTA)
 *
 * @note  Modifier config.h avant déploiement terrain.
 */

// ============================================================
//  Bibliothèques
// ============================================================
#include <LilyGoLib.h>
#include <LV_Helper.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <sys/time.h>
#include <time.h>

// ============================================================
//  Modules du projet
// ============================================================
#include "config.h"
#include "data_model.h"
#include "sensor_service.h"
#include "battery_service.h"
#include "wifi_service.h"
#include "api_service.h"
#include "lora_service.h"
#include "ui_home.h"
#include "ui_health.h"
#include "ui_network.h"
#include "ui_alert.h"
#include "ui_gps.h"
#include "ui_nfc.h"
#include "ui_compass.h"
#include "ui_power.h"
#include "ui_radio.h"
#include "ui_settings.h"
#include "ui_images.h"
#include "config_storage.h"
#include "sd_logger.h"


// ============================================================
//  Variables globales d'état
// ============================================================

/** Écran courant (0=Home, 1=Santé, 2=Réseau, 3=Alertes, 4=SOS, 5=GPS, 6=NFC, 7=Compass, 8=Power, 9=Radio) */
static int           g_currentScreen  = SCREEN_HOME;
static lv_obj_t     *g_screens[11]    = {nullptr};
static lv_obj_t     *g_mainScreen    = nullptr;

/** Queue des alertes internes (FreeRTOS) */
static QueueHandle_t g_alertQueue;

/** Buffer circulaire des alertes affichées */
static AlertMessage  g_alerts[MAX_ALERTS];
static int           g_alertCount = 0;
static SemaphoreHandle_t g_alertMutex;

/** État réseau global */
static NetworkStatus g_netStatus;
static SemaphoreHandle_t g_netMutex;

/** Mutex LVGL (accès depuis plusieurs tâches) */
static SemaphoreHandle_t g_lvglMutex;
static TaskHandle_t      g_uiTaskHandle = nullptr;

/** Horodatage interne (secondes depuis boot) */
static uint32_t g_bootTime = 0;

// ============================================================
//  Prototypes
// ============================================================
void uiNavigateTo(int screen);
void onSOSPressed();
void triggerSOS();
void triggerFall();
void addAlert(AlertType type, const char *text, bool fromSupervisor);
void alertTask(void *param);
void startAlertTask();

// ============================================================
//  Navigation LVGL
// ============================================================

/**
 * @brief Navigue vers l'écran spécifié avec animation.
 *        Thread-safe via mutex LVGL.
 */
void uiNavigateTo(int screen) {
    if (screen < 0 || screen > SCREEN_SETTINGS) return;
    if (screen == g_currentScreen && screen != SCREEN_SOS) return;

    // Définir la position (col, row) de chaque écran dans le tileview
    // SCREEN_HOME (0)     -> col 1, row 1
    // SCREEN_HEALTH (1)   -> col 2, row 1
    // SCREEN_NETWORK (2)  -> col 6, row 1
    // SCREEN_ALERTS (3)   -> col 0, row 1
    // SCREEN_SOS (4)      -> col 1, row 0
    // SCREEN_GPS (5)      -> col 3, row 1
    // SCREEN_NFC (6)      -> col 7, row 1
    // SCREEN_COMPASS (7)  -> col 4, row 1
    // SCREEN_POWER (8)    -> col 1, row 2
    // SCREEN_RADIO (9)    -> col 5, row 1
    // SCREEN_SETTINGS (10)-> col 1, row 3
    int cols[] = { 1, 2, 6, 0, 1, 3, 7, 4, 1, 5, 1 };
    int rows[] = { 1, 1, 1, 1, 0, 1, 1, 1, 2, 1, 3 };

    int col = cols[screen];
    int row = rows[screen];

    bool isUiTask = (xTaskGetCurrentTaskHandle() == g_uiTaskHandle);
    bool hasMutex = false;

    if (isUiTask) {
        hasMutex = true;
    } else {
        if (xSemaphoreTake(g_lvglMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            hasMutex = true;
        }
    }

    if (hasMutex) {
        lv_tileview_set_tile_by_index(g_mainScreen, col, row, LV_ANIM_ON);
        if (!isUiTask) {
            xSemaphoreGive(g_lvglMutex);
        }
    }

    g_currentScreen = screen;
    Serial.printf("[NAV] → Écran %d (Col:%d, Row:%d)\n", screen, col, row);
}

// ============================================================
//  Gestion des swipes (navigation par Tileview native)
// ============================================================

static void tileview_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *tv = (lv_obj_t*)lv_event_get_target(e);
        lv_obj_t *tile = lv_tileview_get_tile_act(tv);

        // Retrouver quel écran correspond à cette tuile
        for (int i = 0; i <= SCREEN_SETTINGS; i++) {
            if (g_screens[i] && lv_obj_get_parent(g_screens[i]) == tile) {
                g_currentScreen = i;
                Serial.printf("[TILE] Écran actif mis à jour : %d\n", i);
                break;
            }
        }
    }
}

void registerSwipeHandlers() {
    // La navigation par glissement est maintenant gérée nativement par le conteneur lv_tileview
}

// ============================================================
//  SOS – Bouton appui long
// ============================================================

/**
 * @brief Callback depuis ui_home.cpp quand le bouton SOS est pressé.
 */
void onSOSPressed() {
    Serial.println("[MAIN] 🆘 SOS déclenché !");
    triggerSOS();
}

/**
 * @brief Déclenche la séquence SOS complète :
 *        vibration + écran rouge + API + LoRa + alerte locale.
 */
void triggerSOS() {
    // Vibration
#ifdef LILYGO_WATCH_S3_PLUS
    instance.vibrator();
#endif

    // Écran rouge SOS
    uiEmergencyShowSOS();
    uiNavigateTo(SCREEN_SOS);

    // Données capteurs actuelles
    SensorData data = sensorGetLatest();

    // Envoi API (dans la même tâche/appel direct car priorité critique)
    apiSendSOS(data);

    // Envoi LoRa
    loraSendSOS(data);

    // Alerte locale
    addAlert(ALERT_SOS, "SOS envoyé ! En attente de secours.", false);

    Serial.println("[MAIN] SOS : API + LoRa + Alerte envoyés");
}

// ============================================================
//  Chute – Détection et action
// ============================================================

/**
 * @brief Déclenche la séquence d'alerte chute.
 */
void triggerFall() {
    Serial.println("[MAIN] ⚠️  CHUTE : Déclenchement alerte");

    // Vibration prolongée
#ifdef LILYGO_WATCH_S3_PLUS
    instance.vibrator();
#endif

    // Écran rouge CHUTE
    uiEmergencyShowFall();
    uiNavigateTo(SCREEN_SOS);

    // Données capteurs
    SensorData data = sensorGetLatest();

    // Envoi API + LoRa
    apiSendFallAlert(data);
    loraSendFall(data);

    // Alerte locale
    addAlert(ALERT_FALL, "Chute détectée ! Alerte envoyée.", false);
}

// ============================================================
//  Gestion des alertes
// ============================================================

struct AlertQueueItem {
    AlertType type;
    char      text[128];
    bool      fromSupervisor;
};

void addAlert(AlertType type, const char *text, bool fromSupervisor) {
    AlertQueueItem item;
    item.type           = type;
    item.fromSupervisor = fromSupervisor;
    strncpy(item.text, text, sizeof(item.text) - 1);
    item.text[sizeof(item.text) - 1] = '\0';

    xQueueSend(g_alertQueue, &item, pdMS_TO_TICKS(10));
}

/**
 * @brief Tâche FreeRTOS – traitement des alertes de la queue.
 */
void alertTask(void *param) {
    AlertQueueItem item;
    Serial.println("[ALERT] Tâche démarrée");

    for (;;) {
        if (xQueueReceive(g_alertQueue, &item, pdMS_TO_TICKS(500)) == pdTRUE) {
            // Ajouter au buffer circulaire
            if (xSemaphoreTake(g_alertMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                if (g_alertCount < MAX_ALERTS) {
                    g_alerts[g_alertCount].type           = item.type;
                    g_alerts[g_alertCount].fromSupervisor = item.fromSupervisor;
                    g_alerts[g_alertCount].timestamp      = millis();
                    g_alerts[g_alertCount].acknowledged   = false;
                    strncpy(g_alerts[g_alertCount].text, item.text,
                            sizeof(g_alerts[g_alertCount].text) - 1);
                    g_alertCount++;
                } else {
                    // Buffer plein : décaler
                    memmove(&g_alerts[0], &g_alerts[1],
                            sizeof(AlertMessage) * (MAX_ALERTS - 1));
                    g_alerts[MAX_ALERTS-1].type           = item.type;
                    g_alerts[MAX_ALERTS-1].fromSupervisor = item.fromSupervisor;
                    g_alerts[MAX_ALERTS-1].timestamp      = millis();
                    strncpy(g_alerts[MAX_ALERTS-1].text, item.text,
                            sizeof(g_alerts[MAX_ALERTS-1].text) - 1);
                }
                xSemaphoreGive(g_alertMutex);
            }

            // Mise à jour UI (depuis tâche → via mutex LVGL)
            if (xSemaphoreTake(g_lvglMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                AlertMessage alert;
                alert.type           = item.type;
                alert.fromSupervisor = item.fromSupervisor;
                alert.timestamp      = millis();
                alert.acknowledged   = false;
                strncpy(alert.text, item.text, sizeof(alert.text) - 1);
                uiAlertAdd(alert);
                xSemaphoreGive(g_lvglMutex);
            }

            Serial.printf("[ALERT] Nouvelle alerte: %s\n", item.text);

            // Vibration brève pour toute alerte superviseur
            if (item.fromSupervisor) {
#ifdef LILYGO_WATCH_S3_PLUS
                instance.vibrator();
#endif
            }
        }
    }
}

void startAlertTask() {
    xTaskCreatePinnedToCore(
        alertTask,
        "AlertTask",
        CFG_STACK_ALERTS,
        nullptr,
        CFG_PRIO_ALERTS,
        nullptr,
        1
    );
}

// ============================================================
//  Callback LoRa réception
// ============================================================

/**
 * @brief Override du weak callback – traitement des messages reçus.
 */
void onLoraMessageReceived(const char *msg, int rssi) {
    Serial.printf("[LORA] Message reçu (%d dBm): %s\n", rssi, msg);

    // Déterminer le type d'alerte
    AlertType aType = ALERT_SUPERVISOR;
    if (strstr(msg, "EVAC") || strstr(msg, "EVACUEZ")) {
        aType = ALERT_EVACUATE;
    } else if (strstr(msg, "RDVPOINT") || strstr(msg, "RENDEZ-VOUS")) {
        aType = ALERT_RALLY_POINT;
    }

    addAlert(aType, msg, true);
}

// ============================================================
//  Tâche UI principale
// ============================================================

static void uiTask(void *param) {
    Serial.println("[UI] Tâche démarrée");
    uint32_t lastUpdate = 0;

    for (;;) {
        uint32_t now = millis();

        // Mise à jour LVGL (rate ~60fps max)
        if (xSemaphoreTake(g_lvglMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            lv_timer_handler();
            xSemaphoreGive(g_lvglMutex);
        }

        // Mise à jour des données affichées (toutes les secondes)
        if (now - lastUpdate > 1000) {
            lastUpdate = now;

            SensorData   data = sensorGetLatest();
            NetworkStatus net;
            if (xSemaphoreTake(g_netMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                net = g_netStatus;
                xSemaphoreGive(g_netMutex);
            }

            // Récupérer l'heure système courante (initialisée depuis la RTC ou NTP)
            time_t rawTime = time(nullptr);
            struct tm timeinfo;
            localtime_r(&rawTime, &timeinfo);
            int h = timeinfo.tm_hour;
            int m = timeinfo.tm_min;
            int s = timeinfo.tm_sec;

            if (xSemaphoreTake(g_lvglMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                switch (g_currentScreen) {
                    case SCREEN_HOME:
                        uiHomeUpdate(data, net);
                        uiHomeSetTime(h, m, s);
                        break;
                    case SCREEN_HEALTH:
                        uiHealthUpdate(data);
                        break;
                    case SCREEN_NETWORK:
                        uiNetworkUpdate(net);
                        break;
                    case SCREEN_GPS:
                        uiGpsUpdate(data.latitude, data.longitude, 0.0f, data.gpsValid ? 4 : 0);
                        break;
                    case SCREEN_COMPASS:
                        {
                            float pitch = atan2f(-data.accelX, sqrtf(data.accelY * data.accelY + data.accelZ * data.accelZ)) * 180.0f / M_PI;
                            float roll = atan2f(data.accelY, data.accelZ) * 180.0f / M_PI;
                            static float heading = 0.0f;
                            if (data.motion == MOTION_WALKING || data.motion == MOTION_RUNNING) {
                                heading += (data.motion == MOTION_RUNNING) ? 2.5f : 1.0f;
                                if (heading >= 360.0f) heading -= 360.0f;
                            }
                            uiCompassUpdate(heading, roll, pitch);
                        }
                        break;
                    case SCREEN_POWER:
                        uiPowerUpdate(data.battery, batteryGetVoltage());
                        break;
                    case SCREEN_SETTINGS:
                        uiSettingsUpdate(g_config.apEnabled, wifiGetAPIP().c_str(), wifiGetIP().c_str());
                        break;

                    default:
                        break;
                }
                xSemaphoreGive(g_lvglMutex);
            }

            // Vérification chute
            if (sensorCheckFall()) {
                triggerFall();
            }

            // Mise à jour état réseau
            if (xSemaphoreTake(g_netMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                g_netStatus.wifiState    = wifiGetState();
                g_netStatus.loraState    = loraGetState();
                g_netStatus.apiConnected = apiIsConnected();
                g_netStatus.loraRssi     = loraGetRSSI();
                String ip = wifiGetIP();
                strncpy(g_netStatus.ipAddress, ip.c_str(),
                        sizeof(g_netStatus.ipAddress) - 1);
                xSemaphoreGive(g_netMutex);
            }

            // Alerte batterie faible
            if (batteryIsLow() && !batteryIsCritical()) {
                static uint32_t lastBatAlert = 0;
                if (now - lastBatAlert > 300000) {  // Toutes les 5min max
                    addAlert(ALERT_LOW_BATTERY, "Batterie faible !", false);
                    lastBatAlert = now;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5));  // ~200Hz max, LVGL régule
    }
}

static void startUITask() {
    xTaskCreatePinnedToCore(
        uiTask,
        "UITask",
        CFG_STACK_UI,
        nullptr,
        CFG_PRIO_UI,
        &g_uiTaskHandle,
        1   // Core 1 = rendu graphique
    );
}

// ============================================================
//  SETUP
// ============================================================

void setup() {
    Serial.begin(115200);
    delay(500);

    // Initialiser la configuration depuis la NVS
    configInit();

    Serial.println("\n");
    Serial.println("╔══════════════════════════════════════╗");
    Serial.println("║   MINE SECURITY WATCH  v1.0.0        ║");
    Serial.println("║   LilyGO T-Watch S3 Plus             ║");
    Serial.println("╚══════════════════════════════════════╝");
    Serial.printf("  Worker : %s (%s)\n", g_config.workerName, g_config.workerId);
    Serial.printf("  Zone   : %s\n", g_config.workerZone);
    Serial.println();

    // --------------------------------------------------------
    //  Initialisation matérielle LilyGoLib
    // --------------------------------------------------------
    Serial.println("[INIT] Initialisation LilyGoLib...");
    instance.begin();
    Serial.println("[INIT] ✅ LilyGoLib OK");

    // Initialiser le temps système ESP32 depuis la puce RTC matérielle
#ifdef LILYGO_WATCH_S3_PLUS
    if (instance.rtc.isClockIntegrityGuaranteed()) {
        RTC_DateTime dt = instance.rtc.getDateTime();
        struct tm timeinfo = dt.toUnixTime();
        time_t t = mktime(&timeinfo);
        struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
        settimeofday(&tv, nullptr);
        
        char timeBuf[64];
        strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &timeinfo);
        Serial.printf("[INIT] RTC matérielle lue : %s\n", timeBuf);
    } else {
        Serial.println("[INIT] ⚠ RTC non initialisée ou oscillateur arrêté");
    }
#endif

    // Luminosité maximale
    instance.setBrightness(DEVICE_MAX_BRIGHTNESS_LEVEL);

    // --------------------------------------------------------
    //  LVGL
    // --------------------------------------------------------
    Serial.println("[INIT] Initialisation LVGL...");
    beginLvglHelper(instance);
    Serial.println("[INIT] ✅ LVGL OK");

    // --------------------------------------------------------
    //  Splash Screen (3 secondes)
    // --------------------------------------------------------
    Serial.println("[INIT] Affichage Écran de Démarrage...");
    lv_obj_t *splashScreen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(splashScreen, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(splashScreen, LV_OPA_COVER, 0);

    // Image logo : img_logo_480x222 (480x222)
    lv_obj_t *logoImg = lv_image_create(splashScreen);
    lv_image_set_src(logoImg, &img_logo_480x222);
    lv_obj_align(logoImg, LV_ALIGN_CENTER, 0, -40);

    // Barre de chargement
    lv_obj_t *loadingBar = lv_bar_create(splashScreen);
    lv_obj_set_size(loadingBar, 240, 15);
    lv_obj_align(loadingBar, LV_ALIGN_CENTER, 0, 100);
    lv_obj_set_style_bg_color(loadingBar, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_color(loadingBar, lv_color_hex(0xFF6B00), LV_PART_INDICATOR);
    lv_bar_set_value(loadingBar, 0, LV_ANIM_OFF);

    lv_screen_load(splashScreen);

    // Animer la barre sur 3 secondes (3000 ms)
    uint32_t startMs = millis();
    while (millis() - startMs < 3000) {
        float progress = (float)(millis() - startMs) / 3000.0f;
        int barVal = progress * 100;
        if (barVal > 100) barVal = 100;
        
        lv_bar_set_value(loadingBar, barVal, LV_ANIM_OFF);
        
        lv_timer_handler();
        delay(10);
    }


    // --------------------------------------------------------
    //  Mutex et queues FreeRTOS
    // --------------------------------------------------------
    g_lvglMutex  = xSemaphoreCreateMutex();
    g_alertMutex = xSemaphoreCreateMutex();
    g_netMutex   = xSemaphoreCreateMutex();
    g_alertQueue = xQueueCreate(20, sizeof(AlertQueueItem));

    memset(&g_netStatus, 0, sizeof(g_netStatus));
    memset(g_alerts,     0, sizeof(g_alerts));

    // --------------------------------------------------------
    //  Construction des écrans LVGL
    // --------------------------------------------------------
    Serial.println("[INIT] Construction des écrans...");

    // --- Construction du conteneur Tileview de navigation 2D (Method Flutter/Factory) ---
    g_mainScreen = lv_tileview_create(nullptr);
    lv_obj_set_style_bg_color(g_mainScreen, lv_color_hex(0x0A0A0A), 0);
    lv_obj_set_style_bg_opa(g_mainScreen, LV_OPA_COVER, 0);
    lv_obj_add_event_cb(g_mainScreen, tileview_event_cb, LV_EVENT_VALUE_CHANGED, nullptr);

    // Ajouter les tuiles (tiles) : (col, row, allowed_dirs)
    lv_obj_t *tile_alerts   = lv_tileview_add_tile(g_mainScreen, 0, 1, LV_DIR_HOR);
    lv_obj_t *tile_home     = lv_tileview_add_tile(g_mainScreen, 1, 1, LV_DIR_ALL); // Home permet horizontal + vertical
    lv_obj_t *tile_health   = lv_tileview_add_tile(g_mainScreen, 2, 1, LV_DIR_HOR);
    lv_obj_t *tile_gps      = lv_tileview_add_tile(g_mainScreen, 3, 1, LV_DIR_HOR);
    lv_obj_t *tile_compass  = lv_tileview_add_tile(g_mainScreen, 4, 1, LV_DIR_HOR);
    lv_obj_t *tile_radio    = lv_tileview_add_tile(g_mainScreen, 5, 1, LV_DIR_HOR);
    lv_obj_t *tile_network  = lv_tileview_add_tile(g_mainScreen, 6, 1, LV_DIR_HOR);
    lv_obj_t *tile_nfc      = lv_tileview_add_tile(g_mainScreen, 7, 1, LV_DIR_HOR);

    lv_obj_t *tile_sos      = lv_tileview_add_tile(g_mainScreen, 1, 0, LV_DIR_VER); // SOS en haut de Home
    lv_obj_t *tile_power    = lv_tileview_add_tile(g_mainScreen, 1, 2, LV_DIR_VER); // Power en bas de Home
    lv_obj_t *tile_settings = lv_tileview_add_tile(g_mainScreen, 1, 3, LV_DIR_NONE); // Pas de swipe direct

    // Construction des écrans LVGL directement sur leurs tuiles respectives
    g_screens[SCREEN_ALERTS]   = uiAlertCreate(tile_alerts);
    g_screens[SCREEN_HOME]     = uiHomeCreate(tile_home);
    g_screens[SCREEN_HEALTH]   = uiHealthCreate(tile_health);
    g_screens[SCREEN_GPS]      = uiGpsCreate(tile_gps);
    g_screens[SCREEN_COMPASS]  = uiCompassCreate(tile_compass);
    g_screens[SCREEN_RADIO]    = uiRadioCreate(tile_radio);
    g_screens[SCREEN_NETWORK]  = uiNetworkCreate(tile_network);
    g_screens[SCREEN_NFC]      = uiNfcCreate(tile_nfc);
    g_screens[SCREEN_SOS]      = uiEmergencyCreate(tile_sos);
    g_screens[SCREEN_POWER]    = uiPowerCreate(tile_power);
    g_screens[SCREEN_SETTINGS] = uiSettingsCreate(tile_settings);

    // Ajuster la taille des écrans pour remplir exactement les tuiles
    for (int i = 0; i <= SCREEN_SETTINGS; i++) {
        if (g_screens[i]) {
            lv_obj_set_size(g_screens[i], LV_HOR_RES, LV_VER_RES);
            lv_obj_set_pos(g_screens[i], 0, 0);
        }
    }

    // Afficher le Tileview et se positionner sur Home (col 1, row 1)
    lv_screen_load(g_mainScreen);
    lv_obj_update_layout(g_mainScreen); // Force le calcul du layout pour pouvoir scroller immédiatement
    lv_tileview_set_tile_by_index(g_mainScreen, 1, 1, LV_ANIM_OFF);
    g_currentScreen = SCREEN_HOME;

    // Supprimer l'écran de démarrage temporaire
    lv_obj_delete(splashScreen);

    Serial.println("[INIT] ✅ Écrans OK");

    // --------------------------------------------------------
    //  Services
    // --------------------------------------------------------
    Serial.println("[INIT] Démarrage des services...");

    batteryInit();
    sensorInit();
    loraInit();
    apiInit();
    
    // Initialisation SD Card pour logging
    if (sdLoggerInit()) {
        Serial.println("[INIT] ✅ SD Card logging activé");
        sdLogSystem("Démarrage système");
    } else {
        Serial.println("[INIT] ⚠ SD Card non disponible");
    }
    
    // Sync NTP si WiFi connecté
    if (WiFi.status() == WL_CONNECTED) {
        if (apiSyncNTP()) {
            Serial.println("[INIT] ✅ NTP synchronisé");
        }
    }

    // --------------------------------------------------------
    //  Tâches FreeRTOS
    // --------------------------------------------------------
    Serial.println("[INIT] Lancement des tâches FreeRTOS...");

    startSensorTask();
    startWifiTask();
    startApiTask();
    startLoraTask();
    startAlertTask();
    startUITask();

    Serial.println("[INIT] ✅ Toutes les tâches lancées");
    Serial.println("[INIT] ✅ Mine Security Watch opérationnel !\n");

    g_bootTime = millis();

    // Message de bienvenue
    addAlert(ALERT_NONE, "Mine Security Watch démarré", false);
}

// ============================================================
//  LOOP – Minimal (LVGL géré par UITask)
// ============================================================

void loop() {
    // La boucle principale est intentionnellement vide.
    // Tout le traitement se fait dans les tâches FreeRTOS.
    // On surveille uniquement les alertes critiques hardware.

    // Vérification connexion WiFi perdue (notification UI)
    static bool s_prevWifi = false;
    bool curWifi = isWifiConnected();
    if (s_prevWifi && !curWifi) {
        addAlert(ALERT_NETWORK_LOST, "Connexion WiFi perdue !", false);
    }
    s_prevWifi = curWifi;

    // Suspendre la tâche loop (pas nécessaire de tourner rapidement)
    vTaskDelay(pdMS_TO_TICKS(1000));
}
