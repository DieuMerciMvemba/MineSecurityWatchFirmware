/**
 * @file      battery_service.h
 * @brief     Service de gestion de la batterie
 * @author    Mine Security Watch Team
 * @date      2026-06-24
 */

#pragma once

#include <Arduino.h>

/**
 * @brief Initialise le service batterie.
 */
void batteryInit();

/**
 * @brief Retourne le niveau de batterie en pourcentage (0–100).
 */
float batteryGetPercent();

/**
 * @brief Retourne true si la batterie est en charge.
 */
bool batteryIsCharging();

/**
 * @brief Retourne true si le niveau est sous le seuil critique.
 */
bool batteryIsCritical();

/**
 * @brief Retourne true si le niveau est bas (alerte).
 */
bool batteryIsLow();

/**
 * @brief Retourne la tension batterie en millivolts.
 */
uint16_t batteryGetVoltage();
