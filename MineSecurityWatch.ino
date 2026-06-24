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
#include "sd_logger.h"

// ============================================================
//  Variables globales d'état
// ============================================================

/** Écran courant (0=Home, 1=Santé, 2=Réseau, 3=Alertes, 4=SOS, 5=GPS, 6=NFC, 7=Compass, 8=Power, 9=Radio) */
static int           g_currentScreen  = SCREEN_HOME;
static lv_obj_t     *g_screens[10]    = {nullptr};

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
    if (screen < 0 || screen > 4) return;
    if (screen == g_currentScreen && screen != SCREEN_SOS) return;

    lv_obj_t *target = g_screens[screen];
    if (!target) return;

    // Déterminer la direction de l'animation
    lv_scr_load_anim_t anim;
    if (screen == SCREEN_SOS || screen == SCREEN_ALERTS) {
        anim = LV_SCR_LOAD_ANIM_FADE_IN;
    } else if (screen > g_currentScreen) {
        anim = LV_SCR_LOAD_ANIM_MOVE_LEFT;
    } else {
        anim = LV_SCR_LOAD_ANIM_MOVE_RIGHT;
    }

    if (xSemaphoreTake(g_lvglMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        lv_screen_load_anim(target, anim, 250, 0, false);
        xSemaphoreGive(g_lvglMutex);
    }

    g_currentScreen = screen;
    Serial.printf("[NAV] → Écran %d\n", screen);
}

// ============================================================
//  Gestion des swipes (navigation tactile)
// ============================================================

static void screen_gesture_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_GESTURE) return;

    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());

    // Swipe gauche → écran suivant
    if (dir == LV_DIR_LEFT) {
        int next = g_currentScreen + 1;
        if (next > SCREEN_RADIO) next = SCREEN_HOME;
        uiNavigateTo(next);
    }
    // Swipe droit → écran précédent
    else if (dir == LV_DIR_RIGHT) {
        int prev = g_currentScreen - 1;
        if (prev < SCREEN_HOME) prev = SCREEN_RADIO;
        uiNavigateTo(prev);
    }
}

static void registerSwipeHandlers() {
    for (int i = 0; i <= SCREEN_RADIO; i++) {
        if (g_screens[i]) {
            lv_obj_add_event_cb(g_screens[i], screen_gesture_cb,
                                LV_EVENT_GESTURE, nullptr);
            lv_obj_add_flag(g_screens[i], LV_OBJ_FLAG_GESTURE_BUBBLE);
        }
    }
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
    instance.vibrate(500);  // 500ms
#endif

    // Écran rouge SOS
    uiEmergencyShowSOS();

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
    instance.vibrate(1000);
#endif

    // Écran rouge CHUTE
    uiEmergencyShowFall();

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
                instance.vibrate(200);
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

            // Heure (sans NTP : secondes depuis boot)
            uint32_t sec  = now / 1000;
            int h = (sec / 3600) % 24;
            int m = (sec / 60)   % 60;
            int s = sec          % 60;

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
        nullptr,
        1   // Core 1 = rendu graphique
    );
}

// ============================================================
//  SETUP
// ============================================================

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n");
    Serial.println("╔══════════════════════════════════════╗");
    Serial.println("║   MINE SECURITY WATCH  v1.0.0        ║");
    Serial.println("║   LilyGO T-Watch S3 Plus             ║");
    Serial.println("╚══════════════════════════════════════╝");
    Serial.printf("  Worker : %s (%s)\n", CFG_WORKER_NAME, CFG_WORKER_ID);
    Serial.printf("  Zone   : %s\n", CFG_WORKER_ZONE);
    Serial.println();

    // --------------------------------------------------------
    //  Initialisation matérielle LilyGoLib
    // --------------------------------------------------------
    Serial.println("[INIT] Initialisation LilyGoLib...");
    instance.begin();
    Serial.println("[INIT] ✅ LilyGoLib OK");

    // Luminosité maximale
    instance.setBrightness(DEVICE_MAX_BRIGHTNESS_LEVEL);

    // --------------------------------------------------------
    //  LVGL
    // --------------------------------------------------------
    Serial.println("[INIT] Initialisation LVGL...");
    beginLvglHelper(instance);
    Serial.println("[INIT] ✅ LVGL OK");

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

    g_screens[SCREEN_HOME]    = uiHomeCreate();
    g_screens[SCREEN_HEALTH]  = uiHealthCreate();
    g_screens[SCREEN_NETWORK] = uiNetworkCreate();
    g_screens[SCREEN_ALERTS]  = uiAlertCreate();
    g_screens[SCREEN_SOS]     = uiEmergencyCreate();
    g_screens[SCREEN_GPS]     = uiGpsCreate();
    g_screens[SCREEN_NFC]     = uiNfcCreate();
    g_screens[SCREEN_COMPASS] = uiCompassCreate();
    g_screens[SCREEN_POWER]   = uiPowerCreate();
    g_screens[SCREEN_RADIO]   = uiRadioCreate();

    // Enregistrement des gestionnaires de swipe
    registerSwipeHandlers();

    // Afficher l'écran Home au démarrage
    lv_screen_load(g_screens[SCREEN_HOME]);
    g_currentScreen = SCREEN_HOME;

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
