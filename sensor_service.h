/**
 * @file      sensor_service.h
 * @brief     Service de lecture des capteurs (IMU, batterie, GPS)
 * @author    Mine Security Watch Team
 * @date      2026-06-24
 */

#pragma once

#include <Arduino.h>
#include "data_model.h"

// ============================================================
//  API publique
// ============================================================

/**
 * @brief Initialise les capteurs (IMU, etc.)
 * @return true si l'initialisation réussit
 */
bool sensorInit();

/**
 * @brief Lit tous les capteurs et remplit la structure SensorData.
 *        Doit être appelée périodiquement (toutes les 1s).
 * @param out Structure SensorData à remplir
 */
void sensorRead(SensorData &out);

/**
 * @brief Retourne la dernière lecture de capteurs disponible.
 *        Thread-safe (mutex interne).
 * @return Copie de la dernière SensorData
 */
SensorData sensorGetLatest();

/**
 * @brief Détecte si une chute a été enregistrée depuis la dernière
 *        vérification. Réinitialise le flag après lecture.
 * @return true si chute détectée
 */
bool sensorCheckFall();

/**
 * @brief Retourne la chaîne de mouvement pour l'API REST.
 * @param m Type de mouvement
 * @return Chaîne "still" / "walking" / "running" / "fall"
 */
const char* sensorMotionStr(MotionType m);

/**
 * @brief Tâche FreeRTOS – lecture continue des capteurs.
 *        Ne pas appeler directement; lancée par startSensorTask().
 */
void sensorTask(void *param);

/**
 * @brief Lance la tâche FreeRTOS de lecture des capteurs.
 */
void startSensorTask();

/**
 * @brief Force la sauvegarde de l'état des capteurs (comme le nombre de pas) dans la NVS.
 */
void sensorSaveState();
