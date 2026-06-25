#include "ui_power.h"
#include "config.h"
#include "LilyGoLib.h"
#include "sensor_service.h"

static lv_obj_t *s_screen = nullptr;
static lv_obj_t *s_lblBattery = nullptr;
static lv_obj_t *s_lblVoltage = nullptr;
static lv_obj_t *s_lblMode = nullptr;
static bool g_ecoMode = false;

// Callback bouton Sleep
static void btn_sleep_cb(lv_event_t *e) {
    Serial.println("[POWER] Bouton Sleep pressé");
    uiPowerSleep();
}

// Callback bouton Éco
static void btn_eco_cb(lv_event_t *e) {
    g_ecoMode = !g_ecoMode;
    uiPowerEcoMode(g_ecoMode);
    lv_obj_t *btn = (lv_obj_t*)lv_event_get_target(e);
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    lv_label_set_text(label, g_ecoMode ? "Éco: ON" : "Éco: OFF");
}

// Callback bouton Shutdown
static void btn_shutdown_cb(lv_event_t *e) {
    Serial.println("[POWER] Bouton Shutdown pressé");
    uiPowerShutdown();
}

// Callback bouton Retour
static void btn_back_cb(lv_event_t *e) {
    extern void uiNavigateTo(int screen);
    uiNavigateTo(0); // SCREEN_HOME
}

lv_obj_t* uiPowerCreate(lv_obj_t *parent) {
    s_screen = lv_obj_create(parent);
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(0x0A0A0A), 0);
    lv_obj_set_style_bg_opa(s_screen, LV_OPA_COVER, 0);
    lv_obj_set_size(s_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_flex_flow(s_screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_screen, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(s_screen, 16, 0);
    lv_obj_set_style_pad_gap(s_screen, 12, 0);
    lv_obj_set_scroll_dir(s_screen, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(s_screen, LV_SCROLLBAR_MODE_AUTO);

    // Titre
    lv_obj_t *title = lv_label_create(s_screen);
    lv_label_set_text(title, LV_SYMBOL_CHARGE " GESTION ÉNERGIE");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFF6B00), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);

    // Séparateur
    lv_obj_t *sep = lv_obj_create(s_screen);
    lv_obj_set_size(sep, LV_PCT(100), 2);
    lv_obj_set_style_bg_color(sep, lv_color_hex(0xFF6B00), 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_30, 0);
    lv_obj_set_style_border_width(sep, 0, 0);

    // Icône batterie
    lv_obj_t *batteryIcon = lv_label_create(s_screen);
    lv_label_set_text(batteryIcon, LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_color(batteryIcon, lv_color_hex(0x00CC44), 0);
    lv_obj_set_style_text_font(batteryIcon, &lv_font_montserrat_32, 0);

    // Niveau batterie
    s_lblBattery = lv_label_create(s_screen);
    lv_label_set_text(s_lblBattery, "--%");
    lv_obj_set_style_text_color(s_lblBattery, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(s_lblBattery, &lv_font_montserrat_24, 0);

    // Tension
    s_lblVoltage = lv_label_create(s_screen);
    lv_label_set_text(s_lblVoltage, "-- mV");
    lv_obj_set_style_text_color(s_lblVoltage, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(s_lblVoltage, &lv_font_montserrat_12, 0);

    // Mode actuel
    s_lblMode = lv_label_create(s_screen);
    lv_label_set_text(s_lblMode, "Mode: Normal");
    lv_obj_set_style_text_color(s_lblMode, lv_color_hex(0x00CC44), 0);
    lv_obj_set_style_text_font(s_lblMode, &lv_font_montserrat_14, 0);

    // Container boutons
    lv_obj_t *btnContainer = lv_obj_create(s_screen);
    lv_obj_set_size(btnContainer, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(btnContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btnContainer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(btnContainer, 0, 0);
    lv_obj_set_style_pad_gap(btnContainer, 8, 0);
    lv_obj_set_style_border_width(btnContainer, 0, 0);
    lv_obj_set_style_bg_opa(btnContainer, LV_OPA_TRANSP, 0);

    // Bouton Sleep
    lv_obj_t *btnSleep = lv_button_create(btnContainer);
    lv_obj_set_size(btnSleep, LV_PCT(80), 44);
    lv_obj_set_style_bg_color(btnSleep, lv_color_hex(0x0099FF), 0);
    lv_obj_set_style_radius(btnSleep, 12, 0);
    lv_obj_add_event_cb(btnSleep, btn_sleep_cb, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *lblSleep = lv_label_create(btnSleep);
    lv_label_set_text(lblSleep, LV_SYMBOL_EYE_OPEN " Sleep");
    lv_obj_center(lblSleep);
    lv_obj_set_style_text_color(lblSleep, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lblSleep, &lv_font_montserrat_14, 0);

    // Bouton Éco
    lv_obj_t *btnEco = lv_button_create(btnContainer);
    lv_obj_set_size(btnEco, LV_PCT(80), 44);
    lv_obj_set_style_bg_color(btnEco, lv_color_hex(0xFF6B00), 0);
    lv_obj_set_style_radius(btnEco, 12, 0);
    lv_obj_add_event_cb(btnEco, btn_eco_cb, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *lblEco = lv_label_create(btnEco);
    lv_label_set_text(lblEco, "Éco: OFF");
    lv_obj_center(lblEco);
    lv_obj_set_style_text_color(lblEco, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lblEco, &lv_font_montserrat_14, 0);

    // Bouton Shutdown
    lv_obj_t *btnShutdown = lv_button_create(btnContainer);
    lv_obj_set_size(btnShutdown, LV_PCT(80), 44);
    lv_obj_set_style_bg_color(btnShutdown, lv_color_hex(0xCC0000), 0);
    lv_obj_set_style_radius(btnShutdown, 12, 0);
    lv_obj_add_event_cb(btnShutdown, btn_shutdown_cb, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *lblShutdown = lv_label_create(btnShutdown);
    lv_label_set_text(lblShutdown, LV_SYMBOL_POWER " Shutdown");
    lv_obj_center(lblShutdown);
    lv_obj_set_style_text_color(lblShutdown, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lblShutdown, &lv_font_montserrat_14, 0);

    // Bouton Retour
    lv_obj_t *btnBack = lv_button_create(btnContainer);
    lv_obj_set_size(btnBack, LV_PCT(80), 44);
    lv_obj_set_style_bg_color(btnBack, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(btnBack, 12, 0);
    lv_obj_add_event_cb(btnBack, btn_back_cb, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *lblBack = lv_label_create(btnBack);
    lv_label_set_text(lblBack, LV_SYMBOL_LEFT " Retour");
    lv_obj_center(lblBack);
    lv_obj_set_style_text_color(lblBack, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lblBack, &lv_font_montserrat_14, 0);

    return s_screen;
}

void uiPowerShow() {
    if (s_screen) {
        // Mettre à jour les infos batterie via PMU
        uint8_t battery = instance.pmu.getBatteryPercent();
        uint16_t voltage = instance.pmu.getBattVoltage();
        
        lv_label_set_text_fmt(s_lblBattery, "%u%%", battery);
        lv_label_set_text_fmt(s_lblVoltage, "%u mV", voltage);
        
        if (battery < 20) {
            lv_obj_set_style_text_color(s_lblBattery, lv_color_hex(0xCC0000), 0);
        } else if (battery < 50) {
            lv_obj_set_style_text_color(s_lblBattery, lv_color_hex(0xFF6B00), 0);
        } else {
            lv_obj_set_style_text_color(s_lblBattery, lv_color_hex(0x00CC44), 0);
        }
        
        lv_screen_load(s_screen);
    }
}

void uiPowerHide() {
    if (s_screen) {
        lv_obj_add_flag(s_screen, LV_OBJ_FLAG_HIDDEN);
    }
}

void uiPowerSleep() {
    Serial.println("[POWER] Entrée en mode sleep...");
    
    // Sauvegarder les pas avant de dormir
    sensorSaveState();
    
    // Écran noir
    lv_obj_clean(lv_screen_active());
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), LV_PART_MAIN);
    lv_refr_now(nullptr);
    
    delay(100);
    
    // Entrer en sleep
    instance.sleep();
}

void uiPowerEcoMode(bool enable) {
    g_ecoMode = enable;
    
    if (enable) {
        Serial.println("[POWER] Mode Éco activé");
        lv_label_set_text(s_lblMode, "Mode: Éco");
        lv_obj_set_style_text_color(s_lblMode, lv_color_hex(0xFF6B00), 0);
        // Réduire la luminosité
        instance.setBrightness(DEVICE_MAX_BRIGHTNESS_LEVEL / 3);
    } else {
        Serial.println("[POWER] Mode Éco désactivé");
        lv_label_set_text(s_lblMode, "Mode: Normal");
        lv_obj_set_style_text_color(s_lblMode, lv_color_hex(0x00CC44), 0);
        // Restaurer la luminosité
        instance.setBrightness(DEVICE_MAX_BRIGHTNESS_LEVEL);
    }
}

void uiPowerShutdown() {
    Serial.println("[POWER] Shutdown...");
    
    // Sauvegarder les pas avant l'arrêt
    sensorSaveState();
    
    // Écran d'arrêt
    lv_obj_clean(lv_screen_active());
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), LV_PART_MAIN);
    
    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, LV_SYMBOL_POWER " Arrêt...");
    lv_obj_center(label);
    lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_18, LV_PART_MAIN);
    
    lv_refr_now(nullptr);
    delay(2000);
    
    // Shutdown - utiliser power off au lieu de shutdown
    esp_deep_sleep_start();
}

void uiPowerUpdate(float battery, uint16_t voltage) {
    if (!s_screen) return;
    lv_label_set_text_fmt(s_lblBattery, "%.0f%%", battery);
    lv_label_set_text_fmt(s_lblVoltage, "%u mV", voltage);
    
    if (battery < 20) {
        lv_obj_set_style_text_color(s_lblBattery, lv_color_hex(0xCC0000), 0);
    } else if (battery < 50) {
        lv_obj_set_style_text_color(s_lblBattery, lv_color_hex(0xFF6B00), 0);
    } else {
        lv_obj_set_style_text_color(s_lblBattery, lv_color_hex(0x00CC44), 0);
    }
}
