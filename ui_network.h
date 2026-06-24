/**
 * @file      ui_network.h
 * @brief     Écran Réseau – WiFi, IP, API, LoRa
 * @author    Mine Security Watch Team
 * @date      2026-06-24
 */

#pragma once

#include <lvgl.h>
#include "data_model.h"

lv_obj_t* uiNetworkCreate(lv_obj_t *parent = nullptr);
void uiNetworkUpdate(const NetworkStatus &net);
lv_obj_t* uiNetworkGetScreen();
