/**
 * @file      ui_settings.cpp
 * @brief     Implémentation de l'écran des réglages avec pavé numérique de sécurité et configuration AP
 * @author    Mine Security Watch Team
 * @date      2026-06-25
 */

#include "ui_settings.h"
#include "config.h"
#include "config_storage.h"
#include "wifi_service.h"
#include "ui_images.h"
#include <WiFi.h>


// En-tête externe de navigation
extern void uiNavigateTo(int screen);

// ============================================================
//  Variables internes
// ============================================================
static lv_obj_t *s_screen            = nullptr;
static lv_obj_t *s_lockContainer     = nullptr;
static lv_obj_t *s_settingsContainer = nullptr;

// Éléments de l'écran de verrouillage
static lv_obj_t *s_lblCodeStars      = nullptr;
static char      s_enteredCode[8]    = "";

// Éléments de l'écran des réglages
static lv_obj_t *s_swAP              = nullptr;
static lv_obj_t *s_lblStatusAP       = nullptr;
static lv_obj_t *s_lblIP             = nullptr;
static lv_obj_t *s_lblWorkerInfo     = nullptr;

static lv_obj_t *s_btnSSID           = nullptr;
static lv_obj_t *s_lblSSID           = nullptr;
static lv_obj_t *s_btnPass           = nullptr;
static lv_obj_t *s_lblPass           = nullptr;
static lv_obj_t *s_btnAPIHost        = nullptr;
static lv_obj_t *s_lblAPIHost        = nullptr;
static lv_obj_t *s_lblAPInstructions = nullptr;

struct EditContext {
    char *targetBuf;
    size_t maxLen;
    lv_obj_t *layer;
    lv_obj_t *ta;
    lv_obj_t *kb;
    const char *title;
};
static EditContext s_editCtx;

// ============================================================
//  Callbacks verrouillage
// ============================================================

static void updateCodeStars() {
    int len = strlen(s_enteredCode);
    char stars[32] = "";
    for (int i = 0; i < len; i++) {
        strcat(stars, "● ");
    }
    // Si vide, afficher un placeholder
    if (len == 0) {
        lv_label_set_text(s_lblCodeStars, "Saisir code");
        lv_obj_set_style_text_color(s_lblCodeStars, lv_color_hex(0x555555), 0);
    } else {
        lv_label_set_text(s_lblCodeStars, stars);
        lv_obj_set_style_text_color(s_lblCodeStars, lv_color_hex(0xFF6B00), 0);
    }
}

static void btn_keypad_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        lv_obj_t *btn = (lv_obj_t*)lv_event_get_target(e);
        const char *txt = lv_label_get_text(lv_obj_get_child(btn, 0));
        int len = strlen(s_enteredCode);

        if (strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
            if (len > 0) {
                s_enteredCode[len - 1] = '\0';
            }
        } else if (strcmp(txt, "X") == 0) {
            // Bouton fermer / retour au Home
            s_enteredCode[0] = '\0';
            uiNavigateTo(SCREEN_HOME);
            return;
        } else {
            // Ajouter le chiffre
            if (len < 4) {
                s_enteredCode[len] = txt[0];
                s_enteredCode[len + 1] = '\0';
                len++;
            }
        }

        updateCodeStars();

        // Vérifier si le code est complet
        if (len == 4) {
            if (strcmp(s_enteredCode, "1234") == 0) {
                Serial.println("[UI] 🔓 Paramètres déverrouillés");
                s_enteredCode[0] = '\0';
                
                // Masquer le verrou, afficher les réglages
                lv_obj_add_flag(s_lockContainer, LV_OBJ_FLAG_HIDDEN);
                lv_obj_remove_flag(s_settingsContainer, LV_OBJ_FLAG_HIDDEN);
            } else {
                Serial.println("[UI] ❌ Mauvais code d'accès");
                s_enteredCode[0] = '\0';
                // Petit effet d'alerte rouge temporaire
                lv_obj_set_style_text_color(s_lblCodeStars, lv_color_hex(0xFF0000), 0);
                lv_label_set_text(s_lblCodeStars, "Code incorrect");
            }
        }
    }
}

// ============================================================
//  Callbacks clavier virtuel & édition
// ============================================================

static void kb_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY) {
        const char *txt = lv_textarea_get_text(s_editCtx.ta);
        strncpy(s_editCtx.targetBuf, txt, s_editCtx.maxLen);
        configSave();
        
        Serial.printf("[UI] Valeur enregistrée: %s\n", txt);
        
        // Mettre à jour l'affichage
        uiSettingsUpdate(g_config.apEnabled, wifiGetAPIP().c_str(), wifiGetIP().c_str());

        // Détruire la couche d'édition
        if (s_editCtx.layer) {
            lv_obj_delete(s_editCtx.layer);
            s_editCtx.layer = nullptr;
        }
    } else if (code == LV_EVENT_CANCEL) {
        if (s_editCtx.layer) {
            lv_obj_delete(s_editCtx.layer);
            s_editCtx.layer = nullptr;
        }
    }
}

static void btn_edit_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        lv_obj_t *btn = (lv_obj_t*)lv_event_get_target(e);
        
        if (btn == s_btnSSID) {
            s_editCtx.targetBuf = g_config.wifiSsid;
            s_editCtx.maxLen = sizeof(g_config.wifiSsid) - 1;
            s_editCtx.title = "MODIFIER SSID WIFI";
        } else if (btn == s_btnPass) {
            s_editCtx.targetBuf = g_config.wifiPassword;
            s_editCtx.maxLen = sizeof(g_config.wifiPassword) - 1;
            s_editCtx.title = "MODIFIER MOT DE PASSE";
        } else if (btn == s_btnAPIHost) {
            s_editCtx.targetBuf = g_config.apiHost;
            s_editCtx.maxLen = sizeof(g_config.apiHost) - 1;
            s_editCtx.title = "MODIFIER HÔTE API";
        } else {
            return;
        }

        // Créer la couche d'édition (superposition dépolie)
        s_editCtx.layer = lv_obj_create(s_screen);
        lv_obj_set_size(s_editCtx.layer, LV_PCT(100), LV_PCT(100));
        lv_obj_set_pos(s_editCtx.layer, 0, 0);
        lv_obj_set_style_bg_color(s_editCtx.layer, lv_color_hex(0x0A0A0A), 0);
        lv_obj_set_style_bg_opa(s_editCtx.layer, LV_OPA_90, 0);
        lv_obj_set_style_border_width(s_editCtx.layer, 0, 0);
        lv_obj_set_style_pad_all(s_editCtx.layer, 12, 0);
        lv_obj_clear_flag(s_editCtx.layer, LV_OBJ_FLAG_SCROLLABLE);

        // Titre
        lv_obj_t *lblTitle = lv_label_create(s_editCtx.layer);
        lv_label_set_text(lblTitle, s_editCtx.title);
        lv_obj_align(lblTitle, LV_ALIGN_TOP_MID, 0, 10);
        lv_obj_set_style_text_color(lblTitle, lv_color_hex(0xFF6B00), 0);
        lv_obj_set_style_text_font(lblTitle, &lv_font_montserrat_14, 0);

        // Text Area
        s_editCtx.ta = lv_textarea_create(s_editCtx.layer);
        lv_obj_set_size(s_editCtx.ta, LV_PCT(90), 45);
        lv_obj_align(s_editCtx.ta, LV_ALIGN_TOP_MID, 0, 40);
        lv_textarea_set_one_line(s_editCtx.ta, true);
        lv_textarea_set_text(s_editCtx.ta, s_editCtx.targetBuf);
        lv_textarea_set_max_length(s_editCtx.ta, s_editCtx.maxLen);
        lv_obj_set_style_bg_color(s_editCtx.ta, lv_color_hex(0x222222), 0);
        lv_obj_set_style_border_color(s_editCtx.ta, lv_color_hex(0xFF6B00), 0);
        lv_obj_set_style_text_color(s_editCtx.ta, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(s_editCtx.ta, &lv_font_montserrat_12, 0);

        // Clavier
        s_editCtx.kb = lv_keyboard_create(s_editCtx.layer);
        lv_obj_set_size(s_editCtx.kb, LV_PCT(100), 180);
        lv_obj_align(s_editCtx.kb, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_keyboard_set_textarea(s_editCtx.kb, s_editCtx.ta);
        
        lv_obj_add_event_cb(s_editCtx.kb, kb_event_cb, LV_EVENT_ALL, nullptr);
    }
}

// ============================================================
//  Callbacks réglages
// ============================================================

static void sw_ap_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        bool active = lv_obj_has_state(s_swAP, LV_STATE_CHECKED);
        g_config.apEnabled = active;
        configSave();

        Serial.printf("[UI] Modification Point d'Accès -> %s\n", active ? "ACTIVÉ" : "DÉSACTIVÉ");

        if (active) {
            WiFi.mode(WIFI_AP);
            wifiStartAP();
            lv_label_set_text(s_lblStatusAP, "AP Actif : MSW-M001\nConnectez-vous sur 192.168.4.1");
            lv_obj_set_style_text_color(s_lblStatusAP, lv_color_hex(0x00CC44), 0);
        } else {
            wifiStopAP();
            WiFi.mode(WIFI_STA);
            lv_label_set_text(s_lblStatusAP, "Point d'Accès désactivé");
            lv_obj_set_style_text_color(s_lblStatusAP, lv_color_hex(0x888888), 0);
        }

        // Mettre à jour l'IP affichée
        uiSettingsUpdate(active, wifiGetAPIP().c_str(), wifiGetIP().c_str());
    }
}

static void btn_back_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        uiNavigateTo(SCREEN_HOME);
    }
}

static void btn_reboot_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        Serial.println("[UI] Redémarrage provoqué par l'utilisateur...");
        vTaskDelay(pdMS_TO_TICKS(500));
        ESP.restart();
    }
}

// ============================================================
//  Création de l'interface
// ============================================================

lv_obj_t* uiSettingsCreate(lv_obj_t *parent) {
    // --- Écran de base ---
    s_screen = lv_obj_create(parent);
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(0x0A0A0A), 0);
    lv_obj_set_style_bg_opa(s_screen, LV_OPA_COVER, 0);
    lv_obj_set_size(s_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);

    // Background Wallpaper Image
    lv_obj_t *bg = lv_image_create(s_screen);
    lv_image_set_src(bg, &img_background2);
    lv_obj_set_size(bg, 480, 222);
    lv_obj_align(bg, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(bg, LV_OBJ_FLAG_FLOATING);
    lv_obj_move_background(bg);

    // ============================================================
    //  1. CONTAINER VERROUILLAGE (Keypad numérique)
    // ============================================================
    s_lockContainer = lv_obj_create(s_screen);
    lv_obj_set_size(s_lockContainer, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(s_lockContainer, 0, 0);
    lv_obj_set_style_bg_opa(s_lockContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_lockContainer, 0, 0);
    lv_obj_set_style_pad_all(s_lockContainer, 8, 0);
    lv_obj_clear_flag(s_lockContainer, LV_OBJ_FLAG_SCROLLABLE);

    // Titre code
    lv_obj_t *lblLockTitle = lv_label_create(s_lockContainer);
    lv_label_set_text(lblLockTitle, "SÉCURITÉ PARAMÈTRES");
    lv_obj_align(lblLockTitle, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_set_style_text_color(lblLockTitle, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(lblLockTitle, &lv_font_montserrat_12, 0);

    // Zone des étoiles
    s_lblCodeStars = lv_label_create(s_lockContainer);
    lv_obj_align(s_lblCodeStars, LV_ALIGN_TOP_MID, 0, 24);
    lv_obj_set_style_text_font(s_lblCodeStars, &lv_font_montserrat_16, 0);
    updateCodeStars();

    // Pavé numérique 3x4 stylisé
    lv_obj_t *keypad = lv_obj_create(s_lockContainer);
    lv_obj_set_size(keypad, LV_PCT(94), 160);
    lv_obj_align(keypad, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_bg_opa(keypad, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(keypad, 0, 0);
    lv_obj_set_style_pad_all(keypad, 0, 0);
    lv_obj_set_style_pad_gap(keypad, 6, 0);
    lv_obj_set_flex_flow(keypad, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(keypad, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    const char* keys[] = {
        "1", "2", "3",
        "4", "5", "6",
        "7", "8", "9",
        "X", "0", LV_SYMBOL_BACKSPACE
    };

    for (int i = 0; i < 12; i++) {
        lv_obj_t *btn = lv_button_create(keypad);
        lv_obj_set_size(btn, LV_PCT(29), 32);
        lv_obj_add_event_cb(btn, btn_keypad_cb, LV_EVENT_CLICKED, nullptr);
        
        // Style bouton clavier (frosted glass)
        if (strcmp(keys[i], "X") == 0) {
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x552222), 0);
            lv_obj_set_style_bg_opa(btn, LV_OPA_70, 0);
        } else if (strcmp(keys[i], LV_SYMBOL_BACKSPACE) == 0) {
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x222222), 0);
            lv_obj_set_style_bg_opa(btn, LV_OPA_70, 0);
        } else {
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x121212), 0);
            lv_obj_set_style_bg_opa(btn, LV_OPA_70, 0);
        }
        lv_obj_set_style_radius(btn, 8, 0);
        lv_obj_set_style_border_width(btn, 1, 0);
        lv_obj_set_style_border_color(btn, lv_color_hex(0x555555), 0);
        lv_obj_set_style_border_opa(btn, LV_OPA_40, 0);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, keys[i]);
        lv_obj_center(lbl);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
    }

    // ============================================================
    //  2. CONTAINER RÉGLAGES (Affiché uniquement après déverrouillage)
    // ============================================================
    s_settingsContainer = lv_obj_create(s_screen);
    lv_obj_set_size(s_settingsContainer, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(s_settingsContainer, 0, 0);
    lv_obj_set_style_bg_opa(s_settingsContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_settingsContainer, 0, 0);
    lv_obj_set_style_pad_all(s_settingsContainer, 10, 0);
    lv_obj_set_style_pad_gap(s_settingsContainer, 8, 0);
    lv_obj_set_flex_flow(s_settingsContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_settingsContainer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(s_settingsContainer, LV_OBJ_FLAG_HIDDEN); // Masqué au départ

    // Titre
    lv_obj_t *title = lv_label_create(s_settingsContainer);
    lv_label_set_text(title, "⚙️ RÉGLAGES SYSTEME");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFF6B00), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);

    // Ligne Switch AP (frosted glass)
    lv_obj_t *rowAP = lv_obj_create(s_settingsContainer);
    lv_obj_set_size(rowAP, LV_PCT(94), 38);
    lv_obj_set_style_bg_color(rowAP, lv_color_hex(0x121212), 0);
    lv_obj_set_style_bg_opa(rowAP, LV_OPA_70, 0);
    lv_obj_set_style_radius(rowAP, 8, 0);
    lv_obj_set_style_border_width(rowAP, 1, 0);
    lv_obj_set_style_border_color(rowAP, lv_color_hex(0x555555), 0);
    lv_obj_set_style_border_opa(rowAP, LV_OPA_40, 0);
    lv_obj_set_style_pad_all(rowAP, 6, 0);
    lv_obj_clear_flag(rowAP, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(rowAP, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(rowAP, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *lblAP = lv_label_create(rowAP);
    lv_label_set_text(lblAP, "Mode AP (Point Accès)");
    lv_obj_set_style_text_font(lblAP, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lblAP, lv_color_hex(0xFFFFFF), 0);

    s_swAP = lv_switch_create(rowAP);
    lv_obj_set_size(s_swAP, 40, 20);
    lv_obj_add_event_cb(s_swAP, sw_ap_cb, LV_EVENT_VALUE_CHANGED, nullptr);
    if (g_config.apEnabled) {
        lv_obj_add_state(s_swAP, LV_STATE_CHECKED);
    }

    // Label Status AP
    s_lblStatusAP = lv_label_create(s_settingsContainer);
    lv_label_set_long_mode(s_lblStatusAP, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_lblStatusAP, LV_PCT(90));
    lv_obj_set_style_text_font(s_lblStatusAP, &lv_font_montserrat_10, 0);
    if (g_config.apEnabled) {
        lv_label_set_text(s_lblStatusAP, "AP Actif : Connectez-vous sur 192.168.4.1\npour configurer.");
        lv_obj_set_style_text_color(s_lblStatusAP, lv_color_hex(0x00CC44), 0);
    } else {
        lv_label_set_text(s_lblStatusAP, "Point d'Accès désactivé");
        lv_obj_set_style_text_color(s_lblStatusAP, lv_color_hex(0x888888), 0);
    }

    // Label IP et infos réseau
    s_lblIP = lv_label_create(s_settingsContainer);
    lv_label_set_long_mode(s_lblIP, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_lblIP, LV_PCT(90));
    lv_obj_set_style_text_font(s_lblIP, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(s_lblIP, lv_color_hex(0xAAAAAA), 0);

    // Label infos mineur
    s_lblWorkerInfo = lv_label_create(s_settingsContainer);
    lv_label_set_long_mode(s_lblWorkerInfo, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_lblWorkerInfo, LV_PCT(90));
    lv_obj_set_style_text_font(s_lblWorkerInfo, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(s_lblWorkerInfo, lv_color_hex(0x888888), 0);

    // Bouton SSID
    s_btnSSID = lv_button_create(s_settingsContainer);
    lv_obj_set_size(s_btnSSID, LV_PCT(94), 38);
    lv_obj_set_style_bg_color(s_btnSSID, lv_color_hex(0x121212), 0);
    lv_obj_set_style_bg_opa(s_btnSSID, LV_OPA_70, 0);
    lv_obj_set_style_radius(s_btnSSID, 8, 0);
    lv_obj_set_style_border_width(s_btnSSID, 1, 0);
    lv_obj_set_style_border_color(s_btnSSID, lv_color_hex(0x555555), 0);
    lv_obj_set_style_border_opa(s_btnSSID, LV_OPA_40, 0);
    lv_obj_add_event_cb(s_btnSSID, btn_edit_cb, LV_EVENT_CLICKED, nullptr);
    s_lblSSID = lv_label_create(s_btnSSID);
    lv_obj_center(s_lblSSID);
    lv_obj_set_style_text_font(s_lblSSID, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(s_lblSSID, lv_color_hex(0xFFFFFF), 0);

    // Bouton Password
    s_btnPass = lv_button_create(s_settingsContainer);
    lv_obj_set_size(s_btnPass, LV_PCT(94), 38);
    lv_obj_set_style_bg_color(s_btnPass, lv_color_hex(0x121212), 0);
    lv_obj_set_style_bg_opa(s_btnPass, LV_OPA_70, 0);
    lv_obj_set_style_radius(s_btnPass, 8, 0);
    lv_obj_set_style_border_width(s_btnPass, 1, 0);
    lv_obj_set_style_border_color(s_btnPass, lv_color_hex(0x555555), 0);
    lv_obj_set_style_border_opa(s_btnPass, LV_OPA_40, 0);
    lv_obj_add_event_cb(s_btnPass, btn_edit_cb, LV_EVENT_CLICKED, nullptr);
    s_lblPass = lv_label_create(s_btnPass);
    lv_obj_center(s_lblPass);
    lv_obj_set_style_text_font(s_lblPass, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(s_lblPass, lv_color_hex(0xFFFFFF), 0);

    // Bouton API Host
    s_btnAPIHost = lv_button_create(s_settingsContainer);
    lv_obj_set_size(s_btnAPIHost, LV_PCT(94), 38);
    lv_obj_set_style_bg_color(s_btnAPIHost, lv_color_hex(0x121212), 0);
    lv_obj_set_style_bg_opa(s_btnAPIHost, LV_OPA_70, 0);
    lv_obj_set_style_radius(s_btnAPIHost, 8, 0);
    lv_obj_set_style_border_width(s_btnAPIHost, 1, 0);
    lv_obj_set_style_border_color(s_btnAPIHost, lv_color_hex(0x555555), 0);
    lv_obj_set_style_border_opa(s_btnAPIHost, LV_OPA_40, 0);
    lv_obj_add_event_cb(s_btnAPIHost, btn_edit_cb, LV_EVENT_CLICKED, nullptr);
    s_lblAPIHost = lv_label_create(s_btnAPIHost);
    lv_obj_center(s_lblAPIHost);
    lv_obj_set_style_text_font(s_lblAPIHost, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(s_lblAPIHost, lv_color_hex(0xFFFFFF), 0);

    // Instructions AP Label
    s_lblAPInstructions = lv_label_create(s_settingsContainer);
    lv_label_set_long_mode(s_lblAPInstructions, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_lblAPInstructions, LV_PCT(90));
    lv_obj_set_style_text_font(s_lblAPInstructions, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(s_lblAPInstructions, lv_color_hex(0xFF6B00), 0);
    lv_label_set_text(s_lblAPInstructions, "Mode AP activé. Modifiez le profil mineur\nvia le portail Wi-Fi sécurisé :\n🔗 http://192.168.4.1/");

    // Mettre à jour les labels dynamiques et visibilités
    uiSettingsUpdate(g_config.apEnabled, wifiGetAPIP().c_str(), wifiGetIP().c_str());

    // Row Boutons actions
    lv_obj_t *rowActions = lv_obj_create(s_settingsContainer);
    lv_obj_set_size(rowActions, LV_PCT(94), 40);
    lv_obj_set_style_bg_opa(rowActions, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(rowActions, 0, 0);
    lv_obj_set_style_pad_all(rowActions, 0, 0);
    lv_obj_set_style_pad_gap(rowActions, 6, 0);
    lv_obj_set_flex_flow(rowActions, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(rowActions, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(rowActions, LV_OBJ_FLAG_SCROLLABLE);

    // Bouton Redémarrer (frosted glass)
    lv_obj_t *btnReboot = lv_button_create(rowActions);
    lv_obj_set_size(btnReboot, LV_PCT(46), 34);
    lv_obj_set_style_bg_color(btnReboot, lv_color_hex(0x553311), 0);
    lv_obj_set_style_bg_opa(btnReboot, LV_OPA_70, 0);
    lv_obj_set_style_radius(btnReboot, 8, 0);
    lv_obj_set_style_border_width(btnReboot, 1, 0);
    lv_obj_set_style_border_color(btnReboot, lv_color_hex(0xFF9900), 0);
    lv_obj_set_style_border_opa(btnReboot, LV_OPA_40, 0);
    lv_obj_add_event_cb(btnReboot, btn_reboot_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *lblReboot = lv_label_create(btnReboot);
    lv_label_set_text(lblReboot, "Redémarrer");
    lv_obj_center(lblReboot);
    lv_obj_set_style_text_font(lblReboot, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lblReboot, lv_color_hex(0xFFFFFF), 0);

    // Bouton Retour (frosted glass)
    lv_obj_t *btnBack = lv_button_create(rowActions);
    lv_obj_set_size(btnBack, LV_PCT(46), 34);
    lv_obj_set_style_bg_color(btnBack, lv_color_hex(0x222222), 0);
    lv_obj_set_style_bg_opa(btnBack, LV_OPA_70, 0);
    lv_obj_set_style_radius(btnBack, 8, 0);
    lv_obj_set_style_border_width(btnBack, 1, 0);
    lv_obj_set_style_border_color(btnBack, lv_color_hex(0x888888), 0);
    lv_obj_set_style_border_opa(btnBack, LV_OPA_40, 0);
    lv_obj_add_event_cb(btnBack, btn_back_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *lblBack = lv_label_create(btnBack);
    lv_label_set_text(lblBack, "Retour");
    lv_obj_center(lblBack);
    lv_obj_set_style_text_font(lblBack, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lblBack, lv_color_hex(0xFFFFFF), 0);

    return s_screen;
}

void uiSettingsUpdate(bool apActive, const char* apIP, const char* staIP) {
    if (!s_screen) return;

    // IP Info
    char ipBuf[128];
    snprintf(ipBuf, sizeof(ipBuf), "IP Station : %s\nIP AP Point : %s",
             staIP && strlen(staIP) > 0 ? staIP : "Déconnecté",
             apActive && apIP && strlen(apIP) > 0 ? apIP : "Désactivé");
    lv_label_set_text(s_lblIP, ipBuf);

    // Worker Info
    char workerBuf[128];
    snprintf(workerBuf, sizeof(workerBuf), "Mineur: %s\nID: %s | Zone: %s\nSite: %s",
             g_config.workerName, g_config.workerId, g_config.workerZone, g_config.siteName);
    lv_label_set_text(s_lblWorkerInfo, workerBuf);

    // Mettre à jour les labels des boutons d'édition
    if (s_lblSSID) {
        char buf[64];
        snprintf(buf, sizeof(buf), "WiFi SSID : %s", g_config.wifiSsid);
        lv_label_set_text(s_lblSSID, buf);
    }
    if (s_lblPass) {
        char buf[64];
        snprintf(buf, sizeof(buf), "WiFi Pass : %s", strlen(g_config.wifiPassword) > 0 ? g_config.wifiPassword : "Aucun");
        lv_label_set_text(s_lblPass, buf);
    }
    if (s_lblAPIHost) {
        char buf[64];
        snprintf(buf, sizeof(buf), "IP Serveur : %s", g_config.apiHost);
        lv_label_set_text(s_lblAPIHost, buf);
    }

    // Visibilité conditionnelle selon le mode AP actif ou inactif
    if (apActive) {
        if (s_btnSSID) lv_obj_add_flag(s_btnSSID, LV_OBJ_FLAG_HIDDEN);
        if (s_btnPass) lv_obj_add_flag(s_btnPass, LV_OBJ_FLAG_HIDDEN);
        if (s_btnAPIHost) lv_obj_add_flag(s_btnAPIHost, LV_OBJ_FLAG_HIDDEN);
        if (s_lblAPInstructions) lv_obj_remove_flag(s_lblAPInstructions, LV_OBJ_FLAG_HIDDEN);
    } else {
        if (s_btnSSID) lv_obj_remove_flag(s_btnSSID, LV_OBJ_FLAG_HIDDEN);
        if (s_btnPass) lv_obj_remove_flag(s_btnPass, LV_OBJ_FLAG_HIDDEN);
        if (s_btnAPIHost) lv_obj_remove_flag(s_btnAPIHost, LV_OBJ_FLAG_HIDDEN);
        if (s_lblAPInstructions) lv_obj_add_flag(s_lblAPInstructions, LV_OBJ_FLAG_HIDDEN);
    }
}

void uiSettingsResetLock() {
    s_enteredCode[0] = '\0';
    updateCodeStars();
    if (s_lockContainer && s_settingsContainer) {
        lv_obj_remove_flag(s_lockContainer, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_settingsContainer, LV_OBJ_FLAG_HIDDEN);
    }
}
