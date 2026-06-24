#include "ui_nfc.h"
#include "config.h"
#include "sd_logger.h"
#include <time.h>

static lv_obj_t *s_screen = nullptr;
static lv_obj_t *s_lblStatus = nullptr;
static lv_obj_t *s_lblLastTag = nullptr;
static lv_obj_t *s_lblLastAction = nullptr;
static lv_obj_t *s_lblLastTime = nullptr;
static lv_obj_t *s_msgbox = nullptr;

// Callback pour fermer le popup NFC
static void nfc_popup_close_cb(lv_event_t *e) {
    if (s_msgbox) {
        lv_obj_del(s_msgbox);
        s_msgbox = nullptr;
    }
}

lv_obj_t* uiNfcCreate() {
    s_screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(0x0A0A0A), 0);
    lv_obj_set_style_bg_opa(s_screen, LV_OPA_COVER, 0);
    lv_obj_set_size(s_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_flex_flow(s_screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_screen, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(s_screen, 16, 0);
    lv_obj_set_style_pad_gap(s_screen, 12, 0);
    lv_obj_set_scroll_dir(s_screen, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(s_screen, LV_SCROLLBAR_MODE_AUTO);

    // Icône NFC grande
    lv_obj_t *icon = lv_label_create(s_screen);
    lv_label_set_text(icon, LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_text_color(icon, lv_color_hex(0xFF6B00), 0);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_48, 0);

    // Titre
    lv_obj_t *title = lv_label_create(s_screen);
    lv_label_set_text(title, "AUTHENTIFICATION NFC");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);

    // Séparateur
    lv_obj_t *sep = lv_obj_create(s_screen);
    lv_obj_set_size(sep, LV_PCT(80), 2);
    lv_obj_set_style_bg_color(sep, lv_color_hex(0xFF6B00), 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_30, 0);
    lv_obj_set_style_border_width(sep, 0, 0);

    // Statut
    s_lblStatus = lv_label_create(s_screen);
    lv_label_set_text(s_lblStatus, "En attente de badge...");
    lv_obj_set_style_text_color(s_lblStatus, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(s_lblStatus, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(s_lblStatus, LV_TEXT_ALIGN_CENTER, 0);

    // Container pour dernier scan
    lv_obj_t *lastContainer = lv_obj_create(s_screen);
    lv_obj_set_size(lastContainer, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(lastContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(lastContainer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(lastContainer, 8, 0);
    lv_obj_set_style_pad_gap(lastContainer, 4, 0);
    lv_obj_set_style_border_width(lastContainer, 0, 0);
    lv_obj_set_style_bg_opa(lastContainer, LV_OPA_TRANSP, 0);

    // Label "Dernier scan"
    lv_obj_t *lastLabel = lv_label_create(lastContainer);
    lv_label_set_text(lastLabel, "DERNIER SCAN:");
    lv_obj_set_style_text_color(lastLabel, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_font(lastLabel, &lv_font_montserrat_10, 0);

    // ID du dernier tag
    s_lblLastTag = lv_label_create(lastContainer);
    lv_label_set_text(s_lblLastTag, "--");
    lv_obj_set_style_text_color(s_lblLastTag, lv_color_hex(0x0099FF), 0);
    lv_obj_set_style_text_font(s_lblLastTag, &lv_font_montserrat_12, 0);

    // Action du dernier scan
    s_lblLastAction = lv_label_create(lastContainer);
    lv_label_set_text(s_lblLastAction, "--");
    lv_obj_set_style_text_color(s_lblLastAction, lv_color_hex(0x00CC44), 0);
    lv_obj_set_style_text_font(s_lblLastAction, &lv_font_montserrat_12, 0);

    // Heure du dernier scan
    s_lblLastTime = lv_label_create(lastContainer);
    lv_label_set_text(s_lblLastTime, "--:--:--");
    lv_obj_set_style_text_color(s_lblLastTime, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_font(s_lblLastTime, &lv_font_montserrat_10, 0);

    return s_screen;
}

void uiNfcUpdateStatus(const char* status) {
    if (s_lblStatus) {
        lv_label_set_text(s_lblStatus, status);
    }
}

void uiNfcShowPopup(const char* tagId, bool checkIn) {
    if (s_msgbox) {
        lv_obj_del(s_msgbox);
    }

    const char *action = checkIn ? "CHECK-IN" : "CHECK-OUT";
    
    char msg[128];
    snprintf(msg, sizeof(msg), 
             "Badge détecté\n%s\n\nAction: %s", 
             tagId, action);

    // Créer le msgbox avec l'API LVGL v9
    s_msgbox = lv_msgbox_create(lv_scr_act());
    lv_msgbox_add_title(s_msgbox, "NFC");
    lv_msgbox_add_text(s_msgbox, msg);
    lv_msgbox_add_close_button(s_msgbox);
    lv_obj_center(s_msgbox);
    
    // Ajouter l'événement de fermeture
    lv_obj_add_event_cb(s_msgbox, nfc_popup_close_cb, LV_EVENT_VALUE_CHANGED, nullptr);
    
    // Logger l'événement NFC
    LogNFCEvent event;
    strncpy(event.tagId, tagId, sizeof(event.tagId) - 1);
    event.checkIn = checkIn;
    event.timestamp = millis();
    sdLogNFC(event);
    
    // Mettre à jour l'interface
    lv_label_set_text(s_lblLastTag, tagId);
    lv_label_set_text(s_lblLastAction, action);
    
    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    char timeBuf[32];
    strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &timeinfo);
    lv_label_set_text(s_lblLastTime, timeBuf);
    
    lv_label_set_text(s_lblStatus, "En attente de badge...");
}

void uiNfcShow() {
    if (s_screen) {
        lv_screen_load(s_screen);
    }
}

void uiNfcHide() {
    if (s_screen) {
        lv_obj_add_flag(s_screen, LV_OBJ_FLAG_HIDDEN);
    }
}
