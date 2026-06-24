#include "sd_logger.h"
#include "config.h"
#include <LilyGoLib.h>
#include <time.h>

static bool g_sdInitialized = false;
static File g_currentLogFile;
static char g_currentLogFileName[32];

// Helper pour obtenir le timestamp formaté
static String getTimestampString() {
    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(buffer);
}

// Helper pour obtenir le nom de fichier du jour
static String getLogFileName() {
    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    char buffer[32];
    strftime(buffer, sizeof(buffer), "/log_%Y%m%d.csv", &timeinfo);
    return String(buffer);
}

bool sdLoggerInit() {
#ifdef HAS_SD_CARD_SOCKET
    Serial.println("[SD] Initialisation SD Card...");
    
    int retry = 10;
    bool is_mount = false;
    do {
        is_mount = instance.installSD();
        if (!is_mount) {
            delay(1000);
        }
        retry--;
    } while (!is_mount && retry > 0);
    
    if (!is_mount) {
        Serial.println("[SD] ❌ Échec initialisation SD Card");
        return false;
    }
    
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("[SD] ❌ Aucune SD Card détectée");
        return false;
    }
    
    Serial.printf("[SD] ✅ SD Card détectée: Type=%d, Size=%lluMB\n",
                  cardType, SD.cardSize() / (1024 * 1024));
    
    // Créer le dossier logs s'il n'existe pas
    if (!SD.exists("/logs")) {
        SD.mkdir("/logs");
    }
    
    g_sdInitialized = true;
    return true;
#else
    Serial.println("[SD] ⚠ SD Card non supportée sur ce matériel");
    return false;
#endif
}

bool sdLogAlert(const char* type, const char* message, bool fromSupervisor) {
    if (!g_sdInitialized) return false;
    
    String fileName = "/logs" + getLogFileName();
    File logFile = SD.open(fileName, FILE_APPEND);
    
    if (!logFile) {
        Serial.println("[SD] ❌ Impossible d'ouvrir le fichier log");
        return false;
    }
    
    String timestamp = getTimestampString();
    String source = fromSupervisor ? "Superviseur" : "Système";
    
    logFile.printf("ALERT,%s,%s,%s,%s\n", 
                   timestamp.c_str(), 
                   type, 
                   source.c_str(), 
                   message);
    
    logFile.close();
    return true;
}

bool sdLogGPS(const LogGPSEvent& event) {
    if (!g_sdInitialized) return false;
    
    String fileName = "/logs" + getLogFileName();
    File logFile = SD.open(fileName, FILE_APPEND);
    
    if (!logFile) return false;
    
    String timestamp = getTimestampString();
    
    logFile.printf("GPS,%s,%.6f,%.6f,%.2f,%u,%u\n",
                   timestamp.c_str(),
                   event.lat,
                   event.lng,
                   event.speed,
                   event.satellites,
                   event.timestamp);
    
    logFile.close();
    return true;
}

bool sdLogNFC(const LogNFCEvent& event) {
    if (!g_sdInitialized) return false;
    
    String fileName = "/logs" + getLogFileName();
    File logFile = SD.open(fileName, FILE_APPEND);
    
    if (!logFile) return false;
    
    String timestamp = getTimestampString();
    String action = event.checkIn ? "CHECK_IN" : "CHECK_OUT";
    
    logFile.printf("NFC,%s,%s,%s,%u\n",
                   timestamp.c_str(),
                   event.tagId,
                   action.c_str(),
                   event.timestamp);
    
    logFile.close();
    return true;
}

bool sdLogSensor(const LogSensorEvent& event) {
    if (!g_sdInitialized) return false;
    
    String fileName = "/logs" + getLogFileName();
    File logFile = SD.open(fileName, FILE_APPEND);
    
    if (!logFile) return false;
    
    String timestamp = getTimestampString();
    
    logFile.printf("SENSOR,%s,%.1f,%u,%u,%u,%u\n",
                   timestamp.c_str(),
                   event.temperature,
                   event.steps,
                   event.battery,
                   event.motion,
                   event.timestamp);
    
    logFile.close();
    return true;
}

bool sdLogSOS() {
    if (!g_sdInitialized) return false;
    
    String fileName = "/logs" + getLogFileName();
    File logFile = SD.open(fileName, FILE_APPEND);
    
    if (!logFile) return false;
    
    String timestamp = getTimestampString();
    
    logFile.printf("SOS,%s,TRIGGERED\n", timestamp.c_str());
    
    logFile.close();
    return true;
}

bool sdLogSystem(const char* event) {
    if (!g_sdInitialized) return false;
    
    String fileName = "/logs" + getLogFileName();
    File logFile = SD.open(fileName, FILE_APPEND);
    
    if (!logFile) return false;
    
    String timestamp = getTimestampString();
    
    logFile.printf("SYSTEM,%s,%s\n", timestamp.c_str(), event);
    
    logFile.close();
    return true;
}

void sdLoggerRotateFiles() {
    if (!g_sdInitialized) return;
    
    // Supprimer les fichiers logs de plus de 30 jours
    File root = SD.open("/logs");
    if (!root) return;
    
    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            file = root.openNextFile();
            continue;
        }
        
        String name = file.name();
        if (name.startsWith("log_") && name.endsWith(".csv")) {
            // Extraire la date du nom de fichier
            // Format: log_YYYYMMDD.csv
            if (name.length() >= 16) {
                String dateStr = name.substring(4, 12);
                // TODO: Implémenter la vérification de date > 30 jours
            }
        }
        file = root.openNextFile();
    }
    
    root.close();
}

void sdLoggerFlush() {
    if (!g_sdInitialized) return;
#ifdef HAS_SD_CARD_SOCKET
    SD.end();
    instance.installSD();
#endif
}
