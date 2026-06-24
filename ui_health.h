/**
 * @file      ui_health.h
 * @brief     Écran Santé – pas, activité, température, mouvement, risque
 * @author    Mine Security Watch Team
 * @date      2026-06-24
 */

#pragma once

#include <lvgl.h>
#include "data_model.h"

lv_obj_t* uiHealthCreate();
void uiHealthUpdate(const SensorData &data);
lv_obj_t* uiHealthGetScreen();
