/**
 * @file      ui_alert.cpp
 * @brief     Écran alertes + Écran urgence (SOS / Chute)
 * @author    Mine Security Watch Team
 * @date      2026-06-24
 *
 * L'écran d'urgence est un fond rouge plein avec animation de pulsation.
 * L'écran alertes affiche l'historique des alertes reçues.
 */

#include "ui_alert.h"
#include "config.h"

// ============================================================
//  Écran ALERTES
// ============================================================
static lv_obj_t *s_screenAlerts = nullptr;
static lv_obj_t *s_list         = nullptr;
static lv_obj_t *s_lblCount     = nullptr;

static lv_color_t alertTypeColor(AlertType t) {
    switch (t) {
        case ALERT_SOS:          return lv_color_hex(0xFF1A1A);
        case ALERT_FALL:         return lv_color_hex(0xFF3300);
        case ALERT_LOW_BATTERY:  return lv_color_hex(0xFF6B00);
        case ALERT_NETWORK_LOST: return lv_color_hex(0xFF6B00);
        case ALERT_SUPERVISOR:
        case ALERT_EVACUATE:
        case ALERT_RALLY_POINT:  return lv_color_hex(0x0099FF);
        default:                 return lv_color_hex(0x888888);
    }
}

static const char* alertTypeIcon(AlertType t) {
    switch (t) {
        case ALERT_SOS:          return LV_SYMBOL_WARNING;
        case ALERT_FALL:         return LV_SYMBOL_DOWN;
        case ALERT_LOW_BATTERY:  return LV_SYMBOL_CHARGE;
        case ALERT_NETWORK_LOST: return LV_SYMBOL_WIFI;
        case ALERT_SUPERVISOR:   return LV_SYMBOL_BELL;
        case ALERT_EVACUATE:     return LV_SYMBOL_HOME;
        case ALERT_RALLY_POINT:  return LV_SYMBOL_GPS;
        default:                 return LV_SYMBOL_LIST;
    }
}

lv_obj_t* uiAlertCreate() {
    s_screenAlerts = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(s_screenAlerts, lv_color_hex(0x0A0A0A), 0);
    lv_obj_set_style_bg_opa(s_screenAlerts, LV_OPA_COVER, 0);
    lv_obj_set_size(s_screenAlerts, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_flex_flow(s_screenAlerts, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_screenAlerts, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(s_screenAlerts, 8, 0);
    lv_obj_set_style_pad_gap(s_screenAlerts, 6, 0);

    // Header avec titre et compteur
    lv_obj_t *header = lv_obj_create(s_screenAlerts);
    lv_obj_set_size(header, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_set_style_pad_gap(header, 8, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, 0);

    // Titre
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, LV_SYMBOL_BELL " ALERTES");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFF6B00), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);

    // Compteur
    s_lblCount = lv_label_create(header);
    lv_label_set_text(s_lblCount, "0 alerte(s)");
    lv_obj_set_style_text_color(s_lblCount, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_font(s_lblCount, &lv_font_montserrat_12, 0);

    // Séparateur
    lv_obj_t *sep = lv_obj_create(s_screenAlerts);
    lv_obj_set_size(sep, LV_PCT(100), 2);
    lv_obj_set_style_bg_color(sep, lv_color_hex(0xFF6B00), 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_30, 0);
    lv_obj_set_style_border_width(sep, 0, 0);

    // Liste des alertes (scrollable)
    s_list = lv_obj_create(s_screenAlerts);
    lv_obj_set_size(s_list, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(s_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(s_list, 4, 0);
    lv_obj_set_style_pad_gap(s_list, 4, 0);
    lv_obj_set_style_bg_color(s_list, lv_color_hex(0x0A0A0A), 0);
    lv_obj_set_style_bg_opa(s_list, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_list, 0, 0);
    lv_obj_set_scrollbar_mode(s_list, LV_SCROLLBAR_MODE_AUTO);

    // Message vide par défaut
    lv_obj_t *emptyLbl = lv_label_create(s_list);
    lv_label_set_text(emptyLbl, "Aucune alerte");
    lv_obj_set_style_text_color(emptyLbl, lv_color_hex(0x444444), 0);
    lv_obj_set_style_text_font(emptyLbl, &lv_font_montserrat_14, 0);

    return s_screenAlerts;
}

void uiAlertAdd(const AlertMessage &alert) {
    if (!s_list) return;

    // Supprimer le label "Aucune alerte" si c'est le premier item
    uint32_t cnt = lv_obj_get_child_count(s_list);
    if (cnt == 1) {
        lv_obj_t *first = lv_obj_get_child(s_list, 0);
        lv_obj_del(first);
    }

    // Container de l'alerte
    lv_obj_t *row = lv_obj_create(s_list);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(row, 10, 0);
    lv_obj_set_style_border_width(row, 2, 0);
    lv_obj_set_style_border_color(row, alertTypeColor(alert.type), 0);
    lv_obj_set_style_pad_all(row, 8, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    // Icône
    lv_obj_t *ico = lv_label_create(row);
    lv_label_set_text(ico, alertTypeIcon(alert.type));
    lv_obj_set_style_text_color(ico, alertTypeColor(alert.type), 0);
    lv_obj_set_style_text_font(ico, &lv_font_montserrat_16, 0);

    // Container pour le texte et la source
    lv_obj_t *textContainer = lv_obj_create(row);
    lv_obj_set_flex_flow(textContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(textContainer, 0, 0);
    lv_obj_set_style_pad_gap(textContainer, 2, 0);
    lv_obj_set_style_border_width(textContainer, 0, 0);
    lv_obj_set_style_bg_opa(textContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_width(textContainer, LV_PCT(100));

    // Texte
    lv_obj_t *txt = lv_label_create(textContainer);
    lv_label_set_text(txt, alert.text);
    lv_obj_set_style_text_color(txt, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(txt, &lv_font_montserrat_12, 0);
    lv_label_set_long_mode(txt, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(txt, LV_PCT(100));

    // Source
    lv_obj_t *src = lv_label_create(textContainer);
    lv_label_set_text(src, alert.fromSupervisor ? "Superviseur" : "Système");
    lv_obj_set_style_text_color(src, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_font(src, &lv_font_montserrat_10, 0);

    // Mise à jour du compteur
    uint32_t newCnt = lv_obj_get_child_count(s_list);
    char buf[24];
    snprintf(buf, sizeof(buf), "%lu alerte(s)", newCnt);
    lv_label_set_text(s_lblCount, buf);
}

void uiAlertUpdate(const AlertMessage *alerts, int count) {
    if (!s_list) return;
    // Effacer tous les items
    lv_obj_clean(s_list);
    for (int i = 0; i < count; i++) {
        uiAlertAdd(alerts[i]);
    }
    if (count == 0) {
        lv_obj_t *emptyLbl = lv_label_create(s_list);
        lv_label_set_text(emptyLbl, "Aucune alerte");
        lv_obj_set_style_text_color(emptyLbl, lv_color_hex(0x444444), 0);
        lv_obj_set_style_text_font(emptyLbl, &lv_font_montserrat_14, 0);
    }
}

lv_obj_t* uiAlertGetScreen() { return s_screenAlerts; }

// ============================================================
//  Écran URGENCE (SOS / Chute)
// ============================================================
static lv_obj_t *s_screenEmerg    = nullptr;
static lv_obj_t *s_lblEmergTitle  = nullptr;
static lv_obj_t *s_lblEmergSub    = nullptr;
static lv_obj_t *s_btnEmergDismiss = nullptr;

// Animation de pulsation du fond rouge
static lv_anim_t s_pulseAnim;
static uint32_t  s_animTimerId = 0;

static void pulse_anim_cb(void *obj, int32_t val) {
    lv_obj_set_style_bg_color((lv_obj_t*)obj,
        lv_color_mix(lv_color_hex(0xFF1A1A), lv_color_hex(0x8B0000),
                     (uint8_t)val), 0);
}

static void dismiss_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        uiEmergencyHide();
    }
}

lv_obj_t* uiEmergencyCreate() {
    s_screenEmerg = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(s_screenEmerg, lv_color_hex(0xCC0000), 0);
    lv_obj_set_style_bg_opa(s_screenEmerg, LV_OPA_COVER, 0);
    lv_obj_set_size(s_screenEmerg, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_flex_flow(s_screenEmerg, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_screenEmerg, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(s_screenEmerg, 16, 0);
    lv_obj_set_style_pad_gap(s_screenEmerg, 12, 0);
    lv_obj_clear_flag(s_screenEmerg, LV_OBJ_FLAG_SCROLLABLE);

    // Grande icône d'avertissement
    lv_obj_t *bigIcon = lv_label_create(s_screenEmerg);
    lv_label_set_text(bigIcon, LV_SYMBOL_WARNING);
    lv_obj_set_style_text_font(bigIcon, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(bigIcon, lv_color_hex(0xFFFFFF), 0);

    // Titre principal
    s_lblEmergTitle = lv_label_create(s_screenEmerg);
    lv_label_set_text(s_lblEmergTitle, "SOS");
    lv_obj_set_style_text_font(s_lblEmergTitle, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(s_lblEmergTitle, lv_color_hex(0xFFFFFF), 0);

    // Sous-titre
    s_lblEmergSub = lv_label_create(s_screenEmerg);
    lv_label_set_text(s_lblEmergSub, "Alerte envoyée");
    lv_obj_set_style_text_font(s_lblEmergSub, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_lblEmergSub, lv_color_hex(0xFFCCCC), 0);

    // ID mineur
    char idBuf[32];
    snprintf(idBuf, sizeof(idBuf), "%s – %s", CFG_WORKER_ID, CFG_WORKER_NAME);
    lv_obj_t *lblId = lv_label_create(s_screenEmerg);
    lv_label_set_text(lblId, idBuf);
    lv_obj_set_style_text_font(lblId, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lblId, lv_color_hex(0xFFAA88), 0);

    // Bouton Annuler (retour)
    s_btnEmergDismiss = lv_button_create(s_screenEmerg);
    lv_obj_set_size(s_btnEmergDismiss, LV_PCT(80), 44);
    lv_obj_set_style_bg_color(s_btnEmergDismiss, lv_color_hex(0x8B0000), 0);
    lv_obj_set_style_radius(s_btnEmergDismiss, 12, 0);
    lv_obj_set_style_border_color(s_btnEmergDismiss, lv_color_hex(0xFF6666), 0);
    lv_obj_set_style_border_width(s_btnEmergDismiss, 2, 0);
    lv_obj_add_flag(s_btnEmergDismiss, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_btnEmergDismiss, dismiss_event_cb, LV_EVENT_ALL, nullptr);

    lv_obj_t *lblDismiss = lv_label_create(s_btnEmergDismiss);
    lv_label_set_text(lblDismiss, "Annuler / Retour");
    lv_obj_center(lblDismiss);
    lv_obj_set_style_text_font(lblDismiss, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lblDismiss, lv_color_hex(0xFFFFFF), 0);

    // Animation pulsation
    lv_anim_init(&s_pulseAnim);
    lv_anim_set_var(&s_pulseAnim, s_screenEmerg);
    lv_anim_set_exec_cb(&s_pulseAnim, pulse_anim_cb);
    lv_anim_set_values(&s_pulseAnim, 0, 255);
    lv_anim_set_duration(&s_pulseAnim, 800);
    lv_anim_set_playback_duration(&s_pulseAnim, 800);
    lv_anim_set_repeat_count(&s_pulseAnim, LV_ANIM_REPEAT_INFINITE);

    return s_screenEmerg;
}

void uiEmergencyShowSOS() {
    if (!s_screenEmerg) return;
    lv_label_set_text(s_lblEmergTitle, "S O S");
    lv_label_set_text(s_lblEmergSub, "Alerte SOS envoyée !");
    lv_anim_start(&s_pulseAnim);
    lv_screen_load_anim(s_screenEmerg, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
    Serial.println("[UI] Écran SOS affiché");
}

void uiEmergencyShowFall() {
    if (!s_screenEmerg) return;
    lv_label_set_text(s_lblEmergTitle, "CHUTE !");
    lv_label_set_text(s_lblEmergSub, "Chute détectée – Alerte envoyée");
    lv_anim_start(&s_pulseAnim);
    lv_screen_load_anim(s_screenEmerg, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
    Serial.println("[UI] Écran CHUTE affiché");
}

void uiEmergencyHide() {
    lv_anim_delete(s_screenEmerg, pulse_anim_cb);
    Serial.println("[UI] Écran urgence masqué");
    // Retour au home (géré par le main)
    extern void uiNavigateTo(int screen);
    uiNavigateTo(SCREEN_HOME);
}

lv_obj_t* uiEmergencyGetScreen() { return s_screenEmerg; }
