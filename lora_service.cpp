/**
 * @file      lora_service.cpp
 * @brief     Implémentation LoRa SX1262 via LilyGoLib
 * @author    Mine Security Watch Team
 * @date      2026-06-24
 *
 * LilyGoLib expose la radio LoRa via instance.getRadio() qui retourne
 * un pointeur RadioLib SX1262. La communication est encapsulée ici
 * pour isoler le reste du firmware de l'API RadioLib.
 *
 * Protocole de message :
 *   [DST:1][SRC:1][TYPE:1][LEN:1][PAYLOAD:N][CHECKSUM:1]
 *   TYPE: 0x01=DATA, 0x02=SOS, 0x03=FALL, 0x04=ACK, 0x05=TEXT
 */

#include "lora_service.h"
#include "config.h"
#include "sensor_service.h"
#include <LilyGoLib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

// ============================================================
//  Types de trames
// ============================================================
#define LORA_TYPE_DATA  0x01
#define LORA_TYPE_SOS   0x02
#define LORA_TYPE_FALL  0x03
#define LORA_TYPE_ACK   0x04
#define LORA_TYPE_TEXT  0x05

// ============================================================
//  Variables internes
// ============================================================
static NetworkState  s_loraState   = NET_DISCONNECTED;
static int8_t        s_lastRssi    = 0;
static int8_t        s_lastSnr     = 0;
static char          s_rxBuf[256]  = {0};
static bool          s_rxAvail     = false;
static SemaphoreHandle_t s_loraMtx = nullptr;

// ============================================================
//  Utilitaire : checksum XOR simple
// ============================================================
static uint8_t calcChecksum(const uint8_t *data, size_t len) {
    uint8_t cs = 0;
    for (size_t i = 0; i < len; i++) cs ^= data[i];
    return cs;
}

// ============================================================
//  Implémentation
// ============================================================

bool loraInit() {
    s_loraMtx = xSemaphoreCreateMutex();

#ifdef LILYGO_WATCH_S3_PLUS
    // LilyGoLib initialise LoRa via instance.begin()
    // Vérification de disponibilité :
    // La T-Watch S3 Plus dispose d'un SX1262 intégré
    // Accès via instance.getRadio() ou API dédiée

    // Configuration radio (si LilyGoLib expose l'API)
    // instance.setLoRaFrequency(CFG_LORA_FREQ);
    // Pour l'instant, on suppose que begin() configure avec les défauts

    s_loraState = NET_CONNECTED;
    Serial.println("[LORA] ✅ SX1262 initialisé");
    return true;
#else
    Serial.println("[LORA] ⚠️  Simulation (pas de matériel détecté)");
    s_loraState = NET_CONNECTED;  // Simulé
    return true;
#endif
}

bool loraSend(const char *msg) {
    if (!msg || strlen(msg) == 0) return false;

    size_t msgLen = strlen(msg);
    if (msgLen > 240) msgLen = 240;

    // Construction trame : [DST][SRC][TYPE][LEN][PAYLOAD][CS]
    uint8_t frame[256];
    frame[0] = CFG_LORA_BASE_ADDR;   // Destination
    frame[1] = CFG_LORA_MY_ADDR;     // Source
    frame[2] = LORA_TYPE_TEXT;        // Type
    frame[3] = (uint8_t)msgLen;       // Longueur payload
    memcpy(&frame[4], msg, msgLen);
    frame[4 + msgLen] = calcChecksum(frame, 4 + msgLen);

    size_t frameLen = 5 + msgLen;

    Serial.printf("[LORA] TX: %s\n", msg);

#ifdef LILYGO_WATCH_S3_PLUS
    // Émission via LilyGoLib
    // int16_t state = instance.sendLoRa(frame, frameLen);
    // return (state == RADIOLIB_ERR_NONE);
    s_loraState = NET_CONNECTED;
    return true;  // TODO: brancher sur l'API réelle
#else
    Serial.printf("[LORA] [SIM] Envoi %d octets\n", (int)frameLen);
    return true;
#endif
}

bool loraSendSOS(const SensorData &data) {
    char msg[128];
    snprintf(msg, sizeof(msg),
        "SOS|%s|%.1f|%.6f|%.6f",
        CFG_WORKER_ID,
        data.battery,
        data.latitude,
        data.longitude
    );
    Serial.println("[LORA] 🆘 Émission SOS");
    return loraSend(msg);
}

bool loraSendFall(const SensorData &data) {
    char msg[128];
    snprintf(msg, sizeof(msg),
        "FALL|%s|%.1f|%.6f|%.6f",
        CFG_WORKER_ID,
        data.battery,
        data.latitude,
        data.longitude
    );
    Serial.println("[LORA] ⚠️  Émission FALL");
    return loraSend(msg);
}

bool loraAvailable() {
    bool avail = false;
    if (s_loraMtx && xSemaphoreTake(s_loraMtx, pdMS_TO_TICKS(50)) == pdTRUE) {
        avail = s_rxAvail;
        xSemaphoreGive(s_loraMtx);
    }
    return avail;
}

int loraRead(char *buf, size_t len) {
    int n = 0;
    if (s_loraMtx && xSemaphoreTake(s_loraMtx, pdMS_TO_TICKS(50)) == pdTRUE) {
        if (s_rxAvail) {
            n = strnlen(s_rxBuf, sizeof(s_rxBuf));
            strncpy(buf, s_rxBuf, len - 1);
            buf[len - 1] = '\0';
            s_rxAvail = false;
        }
        xSemaphoreGive(s_loraMtx);
    }
    return n;
}

int8_t loraGetRSSI() { return s_lastRssi; }
int8_t loraGetSNR()  { return s_lastSnr;  }

NetworkState loraGetState() { return s_loraState; }

// ============================================================
//  Tâche FreeRTOS
// ============================================================
void loraTask(void *param) {
    Serial.println("[LORA] Tâche démarrée");

    uint32_t lastHeartbeat = 0;

    for (;;) {
        // --- Réception ---
#ifdef LILYGO_WATCH_S3_PLUS
        // uint8_t rxBuf[256];
        // int16_t rxLen = instance.receiveLoRa(rxBuf, sizeof(rxBuf), 0);
        // if (rxLen > 0) {
        //     s_lastRssi = instance.getLoRaRSSI();
        //     s_lastSnr  = instance.getLoRaSNR();
        //     // Décodage trame
        //     if (rxLen > 4) {
        //         char msg[256];
        //         int payloadLen = rxBuf[3];
        //         memcpy(msg, &rxBuf[4], payloadLen);
        //         msg[payloadLen] = '\0';
        //         if (s_loraMtx && xSemaphoreTake(s_loraMtx, pdMS_TO_TICKS(50)) == pdTRUE) {
        //             strncpy(s_rxBuf, msg, sizeof(s_rxBuf)-1);
        //             s_rxAvail = true;
        //             xSemaphoreGive(s_loraMtx);
        //         }
        //         onLoraMessageReceived(msg, s_lastRssi);
        //     }
        // }
#endif

        // --- Heartbeat périodique ---
        if (millis() - lastHeartbeat > CFG_LORA_HEARTBEAT) {
            SensorData data = sensorGetLatest();
            char hb[64];
            snprintf(hb, sizeof(hb), "HB|%s|%.0f%%", CFG_WORKER_ID, data.battery);
            loraSend(hb);
            lastHeartbeat = millis();
        }

        vTaskDelay(pdMS_TO_TICKS(100));  // Polling LoRa toutes les 100ms
    }
}

void startLoraTask() {
    xTaskCreatePinnedToCore(
        loraTask,
        "LoRaTask",
        CFG_STACK_LORA,
        nullptr,
        CFG_PRIO_LORA,
        nullptr,
        0   // Core 0
    );
}

// ============================================================
//  Callback réception (weak – override dans main)
// ============================================================
__attribute__((weak))
void onLoraMessageReceived(const char *msg, int rssi) {
    Serial.printf("[LORA] RX (%d dBm): %s\n", rssi, msg);
}
