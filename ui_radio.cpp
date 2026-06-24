#include "ui_radio.h"
#include "config.h"
#include <Arduino.h>

static lv_obj_t *s_screen = nullptr;
static lv_obj_t *s_lblStatus = nullptr;
static lv_obj_t *s_btnPTT = nullptr;
static lv_obj_t *s_lblPTT = nullptr;
static lv_obj_t *s_msgList = nullptr;
static bool g_pttActive = false;

// Callback bouton PTT
static void btn_ptt_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_PRESSED) {
        g_pttActive = true;
        lv_label_set_text(s_lblPTT, LV_SYMBOL_AUDIO " EN COURS...");
        lv_obj_set_style_bg_color(s_btnPTT, lv_color_hex(0xCC0000), 0);
        uiRadioStartPTT();
    } else if (code == LV_EVENT_RELEASED) {
        g_pttActive = false;
        lv_label_set_text(s_lblPTT, LV_SYMBOL_BLUETOOTH " PTT");
        lv_obj_set_style_bg_color(s_btnPTT, lv_color_hex(0x00CC44), 0);
        uiRadioStopPTT();
    }
}

// Callback bouton Retour
static void btn_back_cb(lv_event_t *e) {
    extern void uiNavigateTo(int screen);
    uiNavigateTo(0); // SCREEN_HOME
}

lv_obj_t* uiRadioCreate(lv_obj_t *parent) {
    s_screen = lv_obj_create(parent);
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(0x0A0A0A), 0);
    lv_obj_set_style_bg_opa(s_screen, LV_OPA_COVER, 0);
    lv_obj_set_size(s_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_flex_flow(s_screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(s_screen, 8, 0);
    lv_obj_set_style_pad_gap(s_screen, 6, 0);
    lv_obj_set_scroll_dir(s_screen, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(s_screen, LV_SCROLLBAR_MODE_AUTO);

    // Titre
    lv_obj_t *title = lv_label_create(s_screen);
    lv_label_set_text(title, LV_SYMBOL_BLUETOOTH " WALKIE-TALKIE");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFF6B00), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);

    // Séparateur
    lv_obj_t *sep = lv_obj_create(s_screen);
    lv_obj_set_size(sep, LV_PCT(100), 2);
    lv_obj_set_style_bg_color(sep, lv_color_hex(0xFF6B00), 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_30, 0);
    lv_obj_set_style_border_width(sep, 0, 0);

    // Statut radio
    s_lblStatus = lv_label_create(s_screen);
    lv_label_set_text(s_lblStatus, "Radio: Prêt");
    lv_obj_set_style_text_color(s_lblStatus, lv_color_hex(0x00CC44), 0);
    lv_obj_set_style_text_font(s_lblStatus, &lv_font_montserrat_12, 0);

    // Liste des messages
    s_msgList = lv_obj_create(s_screen);
    lv_obj_set_size(s_msgList, LV_PCT(100), 120);
    lv_obj_set_flex_flow(s_msgList, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_msgList, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(s_msgList, 4, 0);
    lv_obj_set_style_pad_gap(s_msgList, 4, 0);
    lv_obj_set_style_border_width(s_msgList, 1, 0);
    lv_obj_set_style_border_color(s_msgList, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(s_msgList, 8, 0);
    lv_obj_set_style_bg_opa(s_msgList, LV_OPA_TRANSP, 0);
    lv_obj_set_scrollbar_mode(s_msgList, LV_SCROLLBAR_MODE_AUTO);

    // Message vide par défaut
    lv_obj_t *emptyMsg = lv_label_create(s_msgList);
    lv_label_set_text(emptyMsg, "Aucun message");
    lv_obj_set_style_text_color(emptyMsg, lv_color_hex(0x444444), 0);
    lv_obj_set_style_text_font(emptyMsg, &lv_font_montserrat_10, 0);

    // Bouton PTT
    s_btnPTT = lv_button_create(s_screen);
    lv_obj_set_size(s_btnPTT, LV_PCT(92), 60);
    lv_obj_set_style_bg_color(s_btnPTT, lv_color_hex(0x00CC44), 0);
    lv_obj_set_style_radius(s_btnPTT, 16, 0);
    lv_obj_add_event_cb(s_btnPTT, btn_ptt_cb, LV_EVENT_ALL, nullptr);

    s_lblPTT = lv_label_create(s_btnPTT);
    lv_label_set_text(s_lblPTT, LV_SYMBOL_BLUETOOTH " PTT");
    lv_obj_center(s_lblPTT);
    lv_obj_set_style_text_color(s_lblPTT, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(s_lblPTT, &lv_font_montserrat_18, 0);

    // Bouton Retour
    lv_obj_t *btnBack = lv_button_create(s_screen);
    lv_obj_set_size(btnBack, LV_PCT(92), 40);
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

void uiRadioUpdateStatus(const char* status) {
    if (s_lblStatus) {
        lv_label_set_text(s_lblStatus, status);
    }
}

void uiRadioAddMessage(const char* message, const char* sender) {
    if (!s_msgList) return;

    // Supprimer le message "Aucun message" si c'est le premier
    uint32_t cnt = lv_obj_get_child_count(s_msgList);
    if (cnt == 1) {
        lv_obj_t *first = lv_obj_get_child(s_msgList, 0);
        lv_obj_del(first);
    }

    // Créer le message
    lv_obj_t *msgRow = lv_obj_create(s_msgList);
    lv_obj_set_size(msgRow, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(msgRow, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(msgRow, 4, 0);
    lv_obj_set_style_pad_gap(msgRow, 2, 0);
    lv_obj_set_style_border_width(msgRow, 0, 0);
    lv_obj_set_style_bg_opa(msgRow, LV_OPA_TRANSP, 0);

    // Expéditeur
    lv_obj_t *senderLabel = lv_label_create(msgRow);
    lv_label_set_text_fmt(senderLabel, "%s:", sender);
    lv_obj_set_style_text_color(senderLabel, lv_color_hex(0xFF6B00), 0);
    lv_obj_set_style_text_font(senderLabel, &lv_font_montserrat_10, 0);

    // Message
    lv_obj_t *msgLabel = lv_label_create(msgRow);
    lv_label_set_text(msgLabel, message);
    lv_obj_set_style_text_color(msgLabel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(msgLabel, &lv_font_montserrat_12, 0);
    lv_label_set_long_mode(msgLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(msgLabel, LV_PCT(100));

    // Scroller vers le bas
    lv_obj_t *lastMsg = lv_obj_get_child(s_msgList, cnt - 1);
    lv_obj_scroll_to_view(lastMsg, LV_ANIM_ON);
}

void uiRadioShow() {
    if (s_screen) {
        lv_screen_load(s_screen);
    }
}

void uiRadioHide() {
    if (s_screen) {
        lv_obj_add_flag(s_screen, LV_OBJ_FLAG_HIDDEN);
    }
}

void uiRadioStartPTT() {
    Serial.println("[RADIO] Début transmission PTT");
    // TODO: Implémenter la transmission radio réelle
}

void uiRadioStopPTT() {
    Serial.println("[RADIO] Fin transmission PTT");
    // TODO: Implémenter l'arrêt transmission radio réelle
}
