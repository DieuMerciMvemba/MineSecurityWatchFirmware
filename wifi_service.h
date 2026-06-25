/**
 * @file      wifi_service.h
 * @brief     Service WiFi – connexion et reconnexion automatique
 * @author    Mine Security Watch Team
 * @date      2026-06-24
 */

#pragma once

#include <Arduino.h>
#include "data_model.h"

/**
 * @brief Initialise et lance la connexion WiFi.
 */
void wifiInit();

/**
 * @brief Tente de se connecter au réseau configuré.
 * @return true si connexion établie
 */
bool connectWifi();

/**
 * @brief Force une reconnexion au WiFi.
 */
void reconnectWifi();

/**
 * @brief Retourne true si WiFi est connecté.
 */
bool isWifiConnected();

/**
 * @brief Retourne l'adresse IP sous forme de chaîne.
 */
String wifiGetIP();

/**
 * @brief Retourne l'état de connexion WiFi.
 */
NetworkState wifiGetState();

/**
 * @brief Tâche FreeRTOS – gestion WiFi + reconnexion automatique.
 */
void wifiTask(void *param);

/**
 * @brief Lance la tâche FreeRTOS WiFi.
 */
void startWifiTask();

/**
 * @brief Démarre le point d'accès AP avec portail captif.
 */
void wifiStartAP();

/**
 * @brief Arrête le point d'accès AP.
 */
void wifiStopAP();

/**
 * @brief Retourne l'adresse IP du point d'accès (AP).
 */
String wifiGetAPIP();

/**
 * @brief Gère les requêtes HTTP/DNS du portail captif (doit être appelé périodiquement).
 */
void wifiHandleWebServer();

