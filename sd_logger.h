#ifndef SD_LOGGER_H
#define SD_LOGGER_H

#include <Arduino.h>
#include <LilyGoLib.h>
#include <SD.h>

// Types d'événements loggés
enum LogEventType {
    LOG_ALERT,
    LOG_GPS,
    LOG_NFC,
    LOG_SENSOR,
    LOG_SOS,
    LOG_SYSTEM
};

// Structure pour événement GPS
struct LogGPSEvent {
    double lat;
    double lng;
    float speed;
    uint8_t satellites;
    uint32_t timestamp;
};

// Structure pour événement NFC
struct LogNFCEvent {
    char tagId[32];
    bool checkIn;
    uint32_t timestamp;
};

// Structure pour événement capteur
struct LogSensorEvent {
    float temperature;
    uint32_t steps;
    uint8_t battery;
    uint8_t motion;
    uint32_t timestamp;
};

// Structure pour événement système
struct LogSystemEvent {
    char event[64];
    uint32_t timestamp;
};

// Initialisation SD Card
bool sdLoggerInit();

// Logging des différents types d'événements
bool sdLogAlert(const char* type, const char* message, bool fromSupervisor);
bool sdLogGPS(const LogGPSEvent& event);
bool sdLogNFC(const LogNFCEvent& event);
bool sdLogSensor(const LogSensorEvent& event);
bool sdLogSOS();
bool sdLogSystem(const char* event);

// Gestion des fichiers
void sdLoggerRotateFiles();
void sdLoggerFlush();

#endif // SD_LOGGER_H
