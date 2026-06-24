/**
 * @file      sensor_service.cpp
 * @brief     Implémentation du service capteurs
 * @author    Mine Security Watch Team
 * @date      2026-06-24
 *
 * Utilise LilyGoLib pour accéder à l'IMU (accéléromètre/gyroscope).
 * La détection de chute s'appuie sur l'amplitude totale de l'accélération.
 * Les pas sont calculés par intégration d'un algorithme simple de comptage
 * basé sur les pics d'accélération.
 */

#include "sensor_service.h"
#include "config.h"
#include <LilyGoLib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

// ============================================================
//  Variables internes
// ============================================================
static SensorData       s_latest;
static SemaphoreHandle_t s_mutex       = nullptr;
static bool              s_fallFlag    = false;

// Compteur de pas interne
static int32_t  s_steps          = 0;
static float    s_prevAccelMag   = 1.0f;
static bool     s_stepHigh       = false;
static const float STEP_THRESHOLD_HIGH = 1.20f;  // g
static const float STEP_THRESHOLD_LOW  = 0.90f;  // g

// ============================================================
//  Utilitaire interne : magnitude vecteur 3D
// ============================================================
static inline float magnitude(float x, float y, float z) {
    return sqrtf(x*x + y*y + z*z);
}

// ============================================================
//  Détection de pas (algorithme peak-detection)
// ============================================================
static void detectSteps(float mag) {
    if (!s_stepHigh && mag > STEP_THRESHOLD_HIGH) {
        s_stepHigh = true;
    } else if (s_stepHigh && mag < STEP_THRESHOLD_LOW) {
        s_stepHigh = false;
        s_steps++;
    }
}

// ============================================================
//  Détermination du type de mouvement
// ============================================================
static MotionType detectMotion(float mag) {
    if (mag > CFG_FALL_THRESHOLD) return MOTION_FALL;
    if (mag > 1.40f)              return MOTION_RUNNING;
    if (mag > 1.05f)              return MOTION_WALKING;
    return MOTION_STILL;
}

// ============================================================
//  Implémentation publique
// ============================================================

#include <Preferences.h>

bool sensorInit() {
    s_mutex = xSemaphoreCreateMutex();
    if (!s_mutex) return false;

    memset(&s_latest, 0, sizeof(s_latest));
    s_latest.timestamp = millis();

    // Restaurer les pas depuis la NVS
    Preferences prefs;
    prefs.begin("msw", true);
    s_steps = prefs.getInt("steps", 0);
    prefs.end();
    Serial.printf("[SENSOR] Pas restaurés depuis la NVS : %ld\n", s_steps);

#ifdef LILYGO_WATCH_S3_PLUS
    // Configurer et activer l'accéléromètre BMA423
    instance.sensor.configAccelerometer();
    instance.sensor.enableAccelerometer();
#endif

    Serial.println("[SENSOR] Service capteurs initialisé");
    return true;
}

void sensorRead(SensorData &out) {
    // --- Lecture IMU via LilyGoLib ---
    float ax = 0, ay = 0, az = 0;
    float gx = 0, gy = 0, gz = 0;

#ifdef LILYGO_WATCH_S3_PLUS
    int16_t rawX = 0, rawY = 0, rawZ = 0;
    instance.sensor.getAccelerometer(rawX, rawY, rawZ);
    // Convertir en g (BMA423 en mode 4G par défaut donne 2048 LSB par G)
    ax = (float)rawX * (4.0f / 2048.0f);
    ay = (float)rawY * (4.0f / 2048.0f);
    az = (float)rawZ * (4.0f / 2048.0f);

    // Pas de gyroscope matériel sur la T-Watch S3 Plus
    gx = 0.0f;
    gy = 0.0f;
    gz = 0.0f;
#else
    // Valeurs simulées si capteur indisponible (dev)
    ax = 0.0f + (random(-10, 10) / 100.0f);
    ay = 0.0f + (random(-10, 10) / 100.0f);
    az = 1.0f + (random(-5,  5) / 100.0f);
#endif

    float mag = magnitude(ax, ay, az);

    // Détection de pas
    int32_t prevSteps = s_steps;
    detectSteps(mag);
    if (s_steps != prevSteps && (s_steps % 5 == 0)) {
        Preferences prefs;
        prefs.begin("msw", false);
        prefs.putInt("steps", s_steps);
        prefs.end();
    }

    // Détection de mouvement / chute
    MotionType motion = detectMotion(mag);
    if (motion == MOTION_FALL) {
        s_fallFlag = true;
        Serial.println("[SENSOR] ⚠️  CHUTE DÉTECTÉE !");
    }

    // --- Lecture batterie ---
    float battPct = 0.0f;
#ifdef LILYGO_WATCH_S3_PLUS
    battPct = (float)instance.pmu.getBatteryPercent();
#else
    battPct = 85.0f;  // Simulé
#endif

    // --- Température ---
    float temp = 36.5f;
#ifdef LILYGO_WATCH_S3_PLUS
    // Lecture de la température intégrée de la puce BMA423
    temp = instance.sensor.getTemperature(SensorBMA423::TEMP_DEG);
#endif

    // --- GPS (optionnel) ---
    double lat = 0.0, lon = 0.0;
    bool gpsValid = false;
#ifdef LILYGO_WATCH_S3_PLUS
    if (instance.gps.location.isValid()) {
        lat = instance.gps.location.lat();
        lon = instance.gps.location.lng();
        gpsValid = true;
    }
#endif

    // --- Mise à jour de la structure ---
    out.battery     = battPct;
    out.temperature = temp;
    out.steps       = s_steps;
    out.motion      = motion;
    out.accelX      = ax;
    out.accelY      = ay;
    out.accelZ      = az;
    out.gyroX       = gx;
    out.gyroY       = gy;
    out.gyroZ       = gz;
    out.latitude    = lat;
    out.longitude   = lon;
    out.gpsValid    = gpsValid;
    out.timestamp   = millis();

    // --- Mise à jour thread-safe ---
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        s_latest = out;
        xSemaphoreGive(s_mutex);
    }
}

SensorData sensorGetLatest() {
    SensorData copy;
    if (s_mutex && xSemaphoreTake(s_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        copy = s_latest;
        xSemaphoreGive(s_mutex);
    }
    return copy;
}

bool sensorCheckFall() {
    if (s_fallFlag) {
        s_fallFlag = false;
        return true;
    }
    return false;
}

const char* sensorMotionStr(MotionType m) {
    switch (m) {
        case MOTION_STILL:   return "still";
        case MOTION_WALKING: return "walking";
        case MOTION_RUNNING: return "running";
        case MOTION_FALL:    return "fall";
        default:             return "unknown";
    }
}

// ============================================================
//  Tâche FreeRTOS
// ============================================================
void sensorTask(void *param) {
    SensorData data;
    Serial.println("[SENSOR] Tâche démarrée");

    for (;;) {
        sensorRead(data);
        vTaskDelay(pdMS_TO_TICKS(CFG_SENSOR_INTERVAL));
    }
}

void startSensorTask() {
    xTaskCreatePinnedToCore(
        sensorTask,
        "SensorTask",
        CFG_STACK_SENSORS,
        nullptr,
        CFG_PRIO_SENSORS,
        nullptr,
        1   // Core 1 (Core 0 = radio)
    );
}

void sensorSaveState() {
    Preferences prefs;
    prefs.begin("msw", false);
    prefs.putInt("steps", s_steps);
    prefs.end();
    Serial.printf("[SENSOR] État sauvegardé manuellement (Pas : %ld)\n", s_steps);
}
