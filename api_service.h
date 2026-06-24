/**
 * @file      api_service.h
 * @brief     Service API REST – envoi données au serveur
 * @author    Mine Security Watch Team
 * @date      2026-06-24
 */

#pragma once

#include <Arduino.h>
#include "data_model.h"

/**
 * @brief Initialise le client HTTP.
 */
void apiInit();

/**
 * @brief Envoie les données capteurs au serveur REST.
 * @param data Données à envoyer
 * @return true si HTTP 200
 */
bool apiSendData(const SensorData &data);

/**
 * @brief Envoie une alerte SOS immédiate.
 * @param data Dernières données capteurs
 * @return true si HTTP 200
 */
bool apiSendSOS(const SensorData &data);

/**
 * @brief Envoie une alerte de chute au serveur.
 * @param data Dernières données capteurs
 * @return true si HTTP 200
 */
bool apiSendFallAlert(const SensorData &data);

/**
 * @brief Retourne true si le serveur API est joignable.
 */
bool apiIsConnected();

/**
 * @brief Tâche FreeRTOS – envoi périodique automatique.
 */
void apiTask(void *param);

/**
 * @brief Lance la tâche FreeRTOS API.
 */
void startApiTask();

/**
 * @brief Synchronisation NTP via WiFi.
 * @return true si sync réussi
 */
bool apiSyncNTP();

/**
 * @brief Retourne true si NTP a été synchronisé.
 */
bool apiIsNtpSynced();

/**
 * @brief Retourne le timestamp du dernier sync NTP.
 */
uint32_t apiGetLastNtpSync();
