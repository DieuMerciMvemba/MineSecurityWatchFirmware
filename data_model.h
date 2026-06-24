/**
 * @file      data_model.h
 * @brief     Modèles de données partagés - Mine Security Watch
 * @author    Mine Security Watch Team
 * @date      2026-06-24
 *
 * Structures de données centrales utilisées par tous les modules.
 */

#pragma once

#include <Arduino.h>

// ============================================================
//  Constantes globales
// ============================================================
#define WORKER_ID           "M001"
#define WORKER_NAME         "Jean Mbeki"
#define WORKER_ZONE         "Zone B"

#define API_SEND_INTERVAL_MS    30000   // 30 secondes
#define SENSOR_READ_INTERVAL_MS  1000   //  1 seconde
#define WIFI_RECONNECT_DELAY_MS  5000   //  5 secondes
#define LORA_TX_INTERVAL_MS     60000   // 60 secondes

#define FALL_THRESHOLD      2.5f    // g – seuil accéléromètre chute
#define SOS_LONG_PRESS_MS   2000    // ms – appui long bouton SOS
#define LOW_BATTERY_LEVEL   20      // % – seuil batterie faible

// ============================================================
//  Enum : état de connexion
// ============================================================
typedef enum {
    NET_DISCONNECTED = 0,
    NET_CONNECTING,
    NET_CONNECTED,
    NET_ERROR
} NetworkState;

// ============================================================
//  Enum : type de mouvement détecté
// ============================================================
typedef enum {
    MOTION_STILL = 0,
    MOTION_WALKING,
    MOTION_RUNNING,
    MOTION_FALL
} MotionType;

// ============================================================
//  Enum : niveau de risque
// ============================================================
typedef enum {
    RISK_LOW = 0,
    RISK_MEDIUM,
    RISK_HIGH,
    RISK_CRITICAL
} RiskLevel;

// ============================================================
//  Enum : type d'alerte
// ============================================================
typedef enum {
    ALERT_NONE = 0,
    ALERT_SOS,
    ALERT_FALL,
    ALERT_LOW_BATTERY,
    ALERT_NETWORK_LOST,
    ALERT_SUPERVISOR,
    ALERT_EVACUATE,
    ALERT_RALLY_POINT
} AlertType;

// ============================================================
//  Struct : données capteurs consolidées
// ============================================================
struct SensorData {
    float   battery;        // % batterie (0–100)
    float   temperature;    // °C (température ambiante / skin)
    int32_t steps;          // nombre de pas
    MotionType motion;      // type de mouvement
    float   accelX;         // accélération X (g)
    float   accelY;         // accélération Y (g)
    float   accelZ;         // accélération Z (g)
    float   gyroX;          // gyro X (°/s)
    float   gyroY;          // gyro Y (°/s)
    float   gyroZ;          // gyro Z (°/s)
    double  latitude;       // GPS lat (0.0 si indisponible)
    double  longitude;      // GPS lon (0.0 si indisponible)
    bool    gpsValid;       // GPS fix valide
    uint32_t timestamp;     // millis() au moment de la lecture
};

// ============================================================
//  Struct : message d'alerte
// ============================================================
struct AlertMessage {
    AlertType   type;
    char        text[128];
    uint32_t    timestamp;
    bool        acknowledged;
    bool        fromSupervisor;  // true = superviseur, false = local
};

// ============================================================
//  Struct : état réseau
// ============================================================
struct NetworkStatus {
    NetworkState wifiState;
    char         ipAddress[20];
    bool         apiConnected;
    NetworkState loraState;
    int8_t       loraRssi;
    int8_t       lorSnr;
};

// ============================================================
//  Buffer circulaire alertes (max 20)
// ============================================================
#define MAX_ALERTS 20

// ============================================================
//  Constantes LVGL couleurs mine
// ============================================================
#define COLOR_BG_PRIMARY    lv_color_hex(0x0D0D0D)   // Noir profond
#define COLOR_ORANGE        lv_color_hex(0xFF6B00)   // Orange sécurité
#define COLOR_ORANGE_DIM    lv_color_hex(0x7A3300)   // Orange sombre
#define COLOR_RED_ALERT     lv_color_hex(0xFF1A1A)   // Rouge alerte
#define COLOR_GREEN_OK      lv_color_hex(0x00CC44)   // Vert état normal
#define COLOR_WHITE         lv_color_hex(0xFFFFFF)   // Blanc
#define COLOR_GRAY          lv_color_hex(0x888888)   // Gris
#define COLOR_DARK_GRAY     lv_color_hex(0x1E1E1E)   // Fond card
#define COLOR_BLUE          lv_color_hex(0x0099FF)   // Bleu info
