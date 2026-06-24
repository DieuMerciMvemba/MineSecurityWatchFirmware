/**
 * @file      battery_service.cpp
 * @brief     Implémentation du service batterie via LilyGoLib
 * @author    Mine Security Watch Team
 * @date      2026-06-24
 */

#include "battery_service.h"
#include "config.h"
#include <LilyGoLib.h>

void batteryInit() {
    // LilyGoLib initialise le gestionnaire de batterie dans instance.begin()
    Serial.println("[BATTERY] Service batterie initialisé");
}

float batteryGetPercent() {
#ifdef LILYGO_WATCH_S3_PLUS
    return (float)instance.getBatteryPercent();
#else
    return 85.0f;  // Simulé
#endif
}

bool batteryIsCharging() {
#ifdef LILYGO_WATCH_S3_PLUS
    return instance.isCharging();
#else
    return false;
#endif
}

bool batteryIsCritical() {
    return batteryGetPercent() <= (float)CFG_CRIT_BATTERY;
}

bool batteryIsLow() {
    return batteryGetPercent() <= (float)CFG_LOW_BATTERY;
}

uint16_t batteryGetVoltage() {
#ifdef LILYGO_WATCH_S3_PLUS
    return instance.getBatteryVoltage();
#else
    return 3800;  // mV simulé
#endif
}
