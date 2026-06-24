/**
 * @file      lora_service.h
 * @brief     Service LoRa SX1262 – émission, réception, accusé
 * @author    Mine Security Watch Team
 * @date      2026-06-24
 */

#pragma once

#include <Arduino.h>
#include "data_model.h"

/**
 * @brief Initialise la radio LoRa SX1262.
 * @return true si succès
 */
bool loraInit();

/**
 * @brief Envoie un message texte via LoRa.
 * @param msg Message à envoyer (max 255 chars)
 * @return true si émission réussie
 */
bool loraSend(const char *msg);

/**
 * @brief Envoie une trame SOS LoRa.
 * @param data Données capteurs actuelles
 * @return true si émission réussie
 */
bool loraSendSOS(const SensorData &data);

/**
 * @brief Envoie une alerte de chute LoRa.
 * @param data Données capteurs actuelles
 * @return true si émission réussie
 */
bool loraSendFall(const SensorData &data);

/**
 * @brief Retourne true si un message est disponible en réception.
 */
bool loraAvailable();

/**
 * @brief Lit le dernier message reçu.
 * @param buf   Buffer de destination
 * @param len   Taille du buffer
 * @return Nombre d'octets lus
 */
int loraRead(char *buf, size_t len);

/**
 * @brief Retourne le RSSI du dernier paquet reçu.
 */
int8_t loraGetRSSI();

/**
 * @brief Retourne le SNR du dernier paquet reçu.
 */
int8_t loraGetSNR();

/**
 * @brief Retourne l'état de la connexion LoRa.
 */
NetworkState loraGetState();

/**
 * @brief Tâche FreeRTOS – réception continue + heartbeat.
 */
void loraTask(void *param);

/**
 * @brief Lance la tâche FreeRTOS LoRa.
 */
void startLoraTask();

/**
 * @brief Callback appelé depuis loraTask quand un message arrive.
 *        Implémenter dans le .ino ou alerts_service.
 */
void onLoraMessageReceived(const char *msg, int rssi);
