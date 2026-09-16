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

#define SerialGPS Serial1

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

    // ---------------------------------------------------------------
    //  Détection du protocole GPS + récupération NMEA si nécessaire
    // ---------------------------------------------------------------
    //
    // CONTEXTE : un appel précédent à gps.init() a basculé le module UBlox
    // en protocole binaire UBX à 38400 baud et SAUVEGARDÉ cette config dans
    // la flash du module → il reboot toujours en UBX même après power-off.
    //
    // STRATÉGIE :
    //  1. Tester les baudrates connus (38400 en premier)
    //  2. Lire ~100 octets et chercher '$' (NMEA) ou 0xB5 0x62 (UBX sync)
    //  3. Si UBX → envoyer UBX-CFG-PRT pour forcer le module en NMEA @ 9600 baud
    //  4. Si NMEA → déjà bon, rien à faire
    // ---------------------------------------------------------------

    {
        // Commande UBX-CFG-PRT : UART1, 9600 baud, sortie NMEA uniquement
        // Payload 20 octets ; checksum calculé (CK_A=0xA1, CK_B=0x0E)
        static const uint8_t ubxSwitchToNmea[] = {
            0xB5, 0x62,             // Sync chars UBX
            0x06, 0x00,             // Class CFG, ID PRT
            0x14, 0x00,             // Length = 20 bytes
            0x01,                   // portID = UART1
            0x00,                   // reserved
            0x00, 0x00,             // txReady = disabled
            0xD0, 0x08, 0x00, 0x00, // mode = 8N1
            0x80, 0x25, 0x00, 0x00, // baudRate = 9600
            0x07, 0x00,             // inProtoMask  = UBX+NMEA+RTCM
            0x02, 0x00,             // outProtoMask = NMEA seulement
            0x00, 0x00,             // flags
            0x00, 0x00,             // reserved
            0xA1, 0x0E              // Checksum CK_A, CK_B
        };

        uint32_t bauds[] = {38400, 115200, 57600, 9600};
        bool gpsNmea = false;

        for (int bi = 0; bi < 4 && !gpsNmea; bi++) {
            SerialGPS.updateBaudRate(bauds[bi]);
            delay(80);
            // Vider le buffer UART (résidus du baudrate précédent)
            while (SerialGPS.available()) SerialGPS.read();
            delay(150);

            // Lire jusqu'à 100 octets en 600 ms pour identifier le protocole
            uint8_t buf[100];
            int n = 0;
            uint32_t t0 = millis();
            while (millis() - t0 < 600 && n < 100) {
                if (SerialGPS.available()) buf[n++] = SerialGPS.read();
            }

            if (n == 0) {
                Serial.printf("[GPS] Aucun octet reçu à %u baud\n", bauds[bi]);
                continue;
            }

            // Recherche de signatures de protocole dans le buffer
            bool hasNmea = false, hasUbx = false;
            for (int j = 0; j < n; j++) {
                if (buf[j] == '$') { hasNmea = true; break; }
                if (j + 1 < n && buf[j] == 0xB5 && buf[j+1] == 0x62) { hasUbx = true; break; }
            }

            if (hasNmea) {
                Serial.printf("[GPS] ✅ NMEA détecté à %u baud\n", bauds[bi]);
                // Envoi de la configuration profil Piéton / Portatif (sensibilité accrue pour montre)
                static const uint8_t ubxSetPedestrian[] = {
                    0xB5, 0x62,                         // Sync
                    0x06, 0x8A,                         // UBX-CFG-VALSET
                    0x09, 0x00,                         // Length = 9 octets
                    0x00,                               // Version
                    0x07,                               // Layers = RAM + BBR + Flash
                    0x00, 0x00,                         // Reserved
                    0x21, 0x00, 0x11, 0x20,             // CFG-NAVSPG-DYNMODEL (0x20110021)
                    0x03,                               // 3 = Pedestrian
                    0xF5, 0x7C                          // Checksum CK_A, CK_B
                };
                SerialGPS.write(ubxSetPedestrian, sizeof(ubxSetPedestrian));
                SerialGPS.flush();
                Serial.println("[GPS] ✅ Profil dynamique configuré : Piéton/Montre (sensibilité accrue)");
                gpsNmea = true;

            } else if (hasUbx) {
                Serial.printf("[GPS] ⚠️ UBX binaire détecté à %u baud → envoi commande NMEA...\n", bauds[bi]);
                // Envoyer la commande de basculement NMEA @ 9600
                SerialGPS.write(ubxSwitchToNmea, sizeof(ubxSwitchToNmea));
                SerialGPS.flush();
                delay(300); // Attendre que le module se reconfigure

                // Passer Serial1 à 9600 baud (nouveau baudrate du module)
                SerialGPS.updateBaudRate(9600);
                delay(100);
                // Vider les octets résiduels
                while (SerialGPS.available()) SerialGPS.read();
                Serial.println("[GPS] ✅ Module GPS basculé en NMEA @ 9600 baud");
                gpsNmea = true;

            } else {
                Serial.printf("[GPS] %d octets reçus à %u baud mais protocole inconnu (ni NMEA ni UBX)\n",
                              n, bauds[bi]);
            }
        }

        if (!gpsNmea) {
            Serial.println("[GPS] ⚠️ Module GPS non détecté — vérifier câblage");
        }
    }
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

    // --- GPS Matériel ---
    double lat = 0.0, lon = 0.0;
    float alt = 0.0f, spd = 0.0f, hdop = 0.0f;
    uint8_t sats = 0;
    bool gpsValid = false;

#ifdef LILYGO_WATCH_S3_PLUS
    // Récupération des métriques satellites (disponibles même avant le fix 3D complet)
    if (instance.gps.satellites.isValid()) {
        sats = (uint8_t)instance.gps.satellites.value();
    }
    if (instance.gps.speed.isValid()) {
        spd = (float)instance.gps.speed.kmph();
    }
    if (instance.gps.altitude.isValid()) {
        alt = (float)instance.gps.altitude.meters();
    }
    if (instance.gps.hdop.isValid()) {
        hdop = (float)instance.gps.hdop.hdop();
    }

    if (instance.gps.location.isValid()) {
        lat = instance.gps.location.lat();
        lon = instance.gps.location.lng();
        gpsValid = true;
        Serial.printf("[GPS] ✅ Fix OK : Lat=%.6f, Lon=%.6f | Sats=%u | Alt=%.1fm | HDOP=%.1f\n",
                      lat, lon, sats, alt, hdop);
    } else {
        // Log throttlé : 1 fois toutes les 30s pour ne pas saturer la console
        static uint32_t s_lastNoFixLog = 0;
        uint32_t nowMs = millis();
        if (nowMs - s_lastNoFixLog >= 30000) {
            s_lastNoFixLog = nowMs;
            if (sats > 0) {
                Serial.printf("[GPS] 🛰️ %u satellite(s) en vue (HDOP: %.1f) — synchronisation en cours (min. 4 requis pour position)...\n",
                              sats, hdop);
            } else {
                Serial.println("[GPS] ⚠️ Recherche de signaux satellites... (Placez la montre près d'une fenêtre ou à ciel ouvert pour le fix)");
            }
        }
    }

    // Synchronisation automatique de l'horloge système dès que l'heure satellite est reçue
    if (instance.gps.date.isValid() && instance.gps.date.year() > 2020 && instance.gps.time.isValid() && instance.gps.time.age() < 2000) {
        static bool s_gpsTimeSynced = false;
        if (!s_gpsTimeSynced) {
            s_gpsTimeSynced = true;
            struct tm utc_tm = {0};
            utc_tm.tm_year = instance.gps.date.year() - 1900;
            utc_tm.tm_mon  = instance.gps.date.month() - 1;
            utc_tm.tm_mday = instance.gps.date.day();
            utc_tm.tm_hour = instance.gps.time.hour();
            utc_tm.tm_min  = instance.gps.time.minute();
            utc_tm.tm_sec  = instance.gps.time.second();
            time_t t = mktime(&utc_tm);
            struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
            settimeofday(&tv, nullptr);
            // Synchronisation du RTC matériel
            instance.rtc.hwClockWrite();
            Serial.printf("[GPS] ✅ Horloge système & RTC synchronisés par satellite : %02d:%02d:%02d UTC\n",
                          utc_tm.tm_hour, utc_tm.tm_min, utc_tm.tm_sec);
        }
    }
#else
    // Valeurs simulées en dev
    sats = 6;
    spd = (motion == MOTION_WALKING) ? 4.5f : (motion == MOTION_RUNNING ? 9.2f : 0.0f);
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
    out.altitude    = alt;
    out.speed       = spd;
    out.satellites  = sats;
    out.hdop        = hdop;
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
    Serial.println("[SENSOR] Tâche démarrée (Capteurs + GPS streaming)");
    uint32_t lastSensorTime = 0;
    uint32_t lastGpsDiag    = 0;

    for (;;) {
#ifdef LILYGO_WATCH_S3_PLUS
        // Alimentation agressive du parseur NMEA (pattern "smartDelay" de GPSFullExample).
        // Le module GPS envoie des trames à ~1Hz (NMEA GGA, RMC, etc.).
        // Chaque trame fait ~80 octets → le buffer UART (128 o) peut saturer en 1.6s
        // si on ne vide pas assez vite. On vide à chaque itération de 5ms.
        while (SerialGPS.available()) {
            instance.gps.encode(SerialGPS.read());
        }

        // Log diagnostique toutes les 10s : affiche l'état réel du décodage NMEA
        uint32_t now = millis();
        if (now - lastGpsDiag >= 10000) {
            lastGpsDiag = now;
            uint32_t chars   = instance.gps.charsProcessed();
            uint32_t passed  = instance.gps.passedChecksum();
            uint32_t fails   = instance.gps.failedChecksum();
            uint32_t sats    = instance.gps.satellites.isValid() ? instance.gps.satellites.value() : 0;
            bool     fix     = instance.gps.location.isValid();
            float    hdopVal = instance.gps.hdop.isValid() ? (float)instance.gps.hdop.hdop() : 99.9f;

            Serial.printf("[GPS] Octets RX: %u | Phrases NMEA: %u | Sats: %u | HDOP: %.1f | Fix: %s | Err: %u\n",
                          chars, passed, sats, hdopVal, fix ? "✅ OUI" : "⏳ RECHERCHE", fails);

            if (chars < 10) {
                Serial.println("[GPS] ⚠️ AUCUN octet reçu depuis Serial1 → vérifier câblage RX du GPS");
            }
        }
#else
        uint32_t now = millis();
#endif

        if (now - lastSensorTime >= CFG_SENSOR_INTERVAL) {
            lastSensorTime = now;
            sensorRead(data);
        }

        // 5ms : suffisant pour laisser la main aux autres tâches FreeRTOS
        // tout en vidant le buffer UART GPS avant tout risque d'overflow
        vTaskDelay(pdMS_TO_TICKS(5));
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
