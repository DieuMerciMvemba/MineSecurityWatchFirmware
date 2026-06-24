/**
 * @file      ui_network.cpp
 * @brief     Écran Réseau – style liste de statuts avec indicateurs visuels
 * @author    Mine Security Watch Team
 * @date      2026-06-24
 */

#include "ui_network.h"
#include "config.h"

static lv_obj_t *s_screen     = nullptr;
static lv_obj_t *s_dotWifi    = nullptr;
static lv_obj_t *s_lblWifiVal = nullptr;
static lv_obj_t *s_lblIPVal   = nullptr;
static lv_obj_t *s_dotApi     = nullptr;
static lv_obj_t *s_lblApiVal  = nullptr;
static lv_obj_t *s_dotLora    = nullptr;
static lv_obj_t *s_lblLoraVal = nullptr;
static lv_obj_t *s_lblRssi    = nullptr;

// ============================================================
//  Utilitaire : crée une ligne de statut
// ============================================================
static void createStatusRow(lv_obj_t *parent, int y,
                             const char *icon, const char *label,
                             lv_obj_t **dot, lv_obj_t **val) {
    // Icône fixe
    lv_obj_t *ico = lv_label_create(parent);
    lv_label_set_text(ico, icon);
    lv_obj_set_pos(ico, 12, y + 8);
    lv_obj_set_style_text_color(ico, lv_color_hex(0xFF6B00), 0);
    lv_obj_set_style_text_font(ico, &lv_font_montserrat_18, 0);

    // Libellé
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, label);
    lv_obj_set_pos(lbl, 42, y + 6);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);

    // Indicateur LED (point coloré)
    *dot = lv_obj_create(parent);
    lv_obj_set_pos(*dot, LV_HOR_RES - 56, y + 14);
    lv_obj_set_size(*dot, 10, 10);
    lv_obj_set_style_radius(*dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(*dot, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(*dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(*dot, 0, 0);

    // Valeur
    *val = lv_label_create(parent);
    lv_label_set_text(*val, "--");
    lv_obj_set_pos(*val, 42, y + 24);
    lv_obj_set_style_text_color(*val, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(*val, &lv_font_montserrat_12, 0);

    // Séparateur horizontal
    lv_obj_t *sep = lv_obj_create(parent);
    lv_obj_set_pos(sep, 8, y + 48);
    lv_obj_set_size(sep, LV_HOR_RES - 16, 1);
    lv_obj_set_style_bg_color(sep, lv_color_hex(0x2A2A2A), 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sep, 0, 0);
}

lv_obj_t* uiNetworkCreate(lv_obj_t *parent) {
    s_screen = lv_obj_create(parent);
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(0x0A0A0A), 0);
    lv_obj_set_style_bg_opa(s_screen, LV_OPA_COVER, 0);
    lv_obj_set_size(s_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_flex_flow(s_screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(s_screen, 8, 0);
    lv_obj_set_style_pad_gap(s_screen, 4, 0);
    lv_obj_set_scroll_dir(s_screen, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(s_screen, LV_SCROLLBAR_MODE_AUTO);

    // Titre
    lv_obj_t *title = lv_label_create(s_screen);
    lv_label_set_text(title, LV_SYMBOL_WIFI " RÉSEAU");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFF6B00), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);

    lv_obj_t *sep = lv_obj_create(s_screen);
    lv_obj_set_size(sep, LV_PCT(100), 2);
    lv_obj_set_style_bg_color(sep, lv_color_hex(0xFF6B00), 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_30, 0);
    lv_obj_set_style_border_width(sep, 0, 0);

    // Container pour les status rows
    lv_obj_t *statusContainer = lv_obj_create(s_screen);
    lv_obj_set_size(statusContainer, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(statusContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(statusContainer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(statusContainer, 0, 0);
    lv_obj_set_style_pad_gap(statusContainer, 8, 0);
    lv_obj_set_style_border_width(statusContainer, 0, 0);
    lv_obj_set_style_bg_opa(statusContainer, LV_OPA_TRANSP, 0);

    // WiFi
    lv_obj_t *wifiRow = lv_obj_create(statusContainer);
    lv_obj_set_size(wifiRow, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(wifiRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(wifiRow, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(wifiRow, 0, 0);
    lv_obj_set_style_pad_gap(wifiRow, 8, 0);
    lv_obj_set_style_border_width(wifiRow, 0, 0);
    lv_obj_set_style_bg_opa(wifiRow, LV_OPA_TRANSP, 0);

    lv_obj_t *wifiIcon = lv_label_create(wifiRow);
    lv_label_set_text(wifiIcon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(wifiIcon, lv_color_hex(0xFF6B00), 0);
    lv_obj_set_style_text_font(wifiIcon, &lv_font_montserrat_16, 0);

    lv_obj_t *wifiInfo = lv_obj_create(wifiRow);
    lv_obj_set_flex_flow(wifiInfo, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(wifiInfo, 0, 0);
    lv_obj_set_style_pad_gap(wifiInfo, 2, 0);
    lv_obj_set_style_border_width(wifiInfo, 0, 0);
    lv_obj_set_style_bg_opa(wifiInfo, LV_OPA_TRANSP, 0);

    lv_obj_t *wifiLabel = lv_label_create(wifiInfo);
    lv_label_set_text(wifiLabel, "WiFi");
    lv_obj_set_style_text_color(wifiLabel, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_font(wifiLabel, &lv_font_montserrat_12, 0);

    s_lblWifiVal = lv_label_create(wifiInfo);
    lv_label_set_text(s_lblWifiVal, "--");
    lv_obj_set_style_text_color(s_lblWifiVal, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(s_lblWifiVal, &lv_font_montserrat_12, 0);

    s_lblIPVal = lv_label_create(wifiInfo);
    lv_label_set_text(s_lblIPVal, "0.0.0.0");
    lv_obj_set_style_text_color(s_lblIPVal, lv_color_hex(0x0099FF), 0);
    lv_obj_set_style_text_font(s_lblIPVal, &lv_font_montserrat_10, 0);

    s_dotWifi = lv_obj_create(wifiRow);
    lv_obj_set_size(s_dotWifi, 10, 10);
    lv_obj_set_style_radius(s_dotWifi, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_dotWifi, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(s_dotWifi, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_dotWifi, 0, 0);

    // API
    lv_obj_t *apiRow = lv_obj_create(statusContainer);
    lv_obj_set_size(apiRow, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(apiRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(apiRow, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(apiRow, 0, 0);
    lv_obj_set_style_pad_gap(apiRow, 8, 0);
    lv_obj_set_style_border_width(apiRow, 0, 0);
    lv_obj_set_style_bg_opa(apiRow, LV_OPA_TRANSP, 0);

    lv_obj_t *apiIcon = lv_label_create(apiRow);
    lv_label_set_text(apiIcon, LV_SYMBOL_UPLOAD);
    lv_obj_set_style_text_color(apiIcon, lv_color_hex(0xFF6B00), 0);
    lv_obj_set_style_text_font(apiIcon, &lv_font_montserrat_16, 0);

    lv_obj_t *apiInfo = lv_obj_create(apiRow);
    lv_obj_set_flex_flow(apiInfo, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(apiInfo, 0, 0);
    lv_obj_set_style_pad_gap(apiInfo, 2, 0);
    lv_obj_set_style_border_width(apiInfo, 0, 0);
    lv_obj_set_style_bg_opa(apiInfo, LV_OPA_TRANSP, 0);

    lv_obj_t *apiLabel = lv_label_create(apiInfo);
    lv_label_set_text(apiLabel, "API REST");
    lv_obj_set_style_text_color(apiLabel, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_font(apiLabel, &lv_font_montserrat_12, 0);

    s_lblApiVal = lv_label_create(apiInfo);
    lv_label_set_text(s_lblApiVal, "--");
    lv_obj_set_style_text_color(s_lblApiVal, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(s_lblApiVal, &lv_font_montserrat_12, 0);

    s_dotApi = lv_obj_create(apiRow);
    lv_obj_set_size(s_dotApi, 10, 10);
    lv_obj_set_style_radius(s_dotApi, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_dotApi, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(s_dotApi, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_dotApi, 0, 0);

    // LoRa
    lv_obj_t *loraRow = lv_obj_create(statusContainer);
    lv_obj_set_size(loraRow, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(loraRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(loraRow, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(loraRow, 0, 0);
    lv_obj_set_style_pad_gap(loraRow, 8, 0);
    lv_obj_set_style_border_width(loraRow, 0, 0);
    lv_obj_set_style_bg_opa(loraRow, LV_OPA_TRANSP, 0);

    lv_obj_t *loraIcon = lv_label_create(loraRow);
    lv_label_set_text(loraIcon, LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_text_color(loraIcon, lv_color_hex(0xFF6B00), 0);
    lv_obj_set_style_text_font(loraIcon, &lv_font_montserrat_16, 0);

    lv_obj_t *loraInfo = lv_obj_create(loraRow);
    lv_obj_set_flex_flow(loraInfo, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(loraInfo, 0, 0);
    lv_obj_set_style_pad_gap(loraInfo, 2, 0);
    lv_obj_set_style_border_width(loraInfo, 0, 0);
    lv_obj_set_style_bg_opa(loraInfo, LV_OPA_TRANSP, 0);

    lv_obj_t *loraLabel = lv_label_create(loraInfo);
    lv_label_set_text(loraLabel, "LoRa SX1262");
    lv_obj_set_style_text_color(loraLabel, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_font(loraLabel, &lv_font_montserrat_12, 0);

    s_lblLoraVal = lv_label_create(loraInfo);
    lv_label_set_text(s_lblLoraVal, "--");
    lv_obj_set_style_text_color(s_lblLoraVal, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(s_lblLoraVal, &lv_font_montserrat_12, 0);

    s_lblRssi = lv_label_create(loraInfo);
    lv_label_set_text(s_lblRssi, "RSSI: --");
    lv_obj_set_style_text_color(s_lblRssi, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_font(s_lblRssi, &lv_font_montserrat_10, 0);

    s_dotLora = lv_obj_create(loraRow);
    lv_obj_set_size(s_dotLora, 10, 10);
    lv_obj_set_style_radius(s_dotLora, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_dotLora, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(s_dotLora, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_dotLora, 0, 0);

    // Footer ID
    lv_obj_t *footer = lv_label_create(s_screen);
    char footerBuf[32];
    snprintf(footerBuf, sizeof(footerBuf), "ID: %s", CFG_WORKER_ID);
    lv_label_set_text(footer, footerBuf);
    lv_obj_set_style_text_color(footer, lv_color_hex(0x444444), 0);
    lv_obj_set_style_text_font(footer, &lv_font_montserrat_12, 0);

    return s_screen;
}

// ============================================================
//  Mise à jour
// ============================================================
static void setDotColor(lv_obj_t *dot, bool ok, bool blink = false) {
    lv_color_t c = ok ? lv_color_hex(0x00CC44) : lv_color_hex(0xFF1A1A);
    lv_obj_set_style_bg_color(dot, c, 0);
    // Effet glow
    lv_obj_set_style_shadow_width(dot, ok ? 8 : 0, 0);
    lv_obj_set_style_shadow_color(dot, c, 0);
    lv_obj_set_style_shadow_opa(dot, LV_OPA_50, 0);
}

void uiNetworkUpdate(const NetworkStatus &net) {
    if (!s_screen) return;

    bool wifiOk = (net.wifiState == NET_CONNECTED);
    bool apiOk  = net.apiConnected;
    bool loraOk = (net.loraState == NET_CONNECTED);

    // WiFi
    setDotColor(s_dotWifi, wifiOk);
    lv_label_set_text(s_lblWifiVal, wifiOk ? "Connecté" : "Déconnecté");
    lv_obj_set_style_text_color(s_lblWifiVal,
        wifiOk ? lv_color_hex(0x00CC44) : lv_color_hex(0xFF1A1A), 0);

    // IP
    lv_label_set_text(s_lblIPVal, net.ipAddress);

    // API
    setDotColor(s_dotApi, apiOk);
    lv_label_set_text(s_lblApiVal,
        apiOk ? "En ligne" : "Hors ligne");
    lv_obj_set_style_text_color(s_lblApiVal,
        apiOk ? lv_color_hex(0x00CC44) : lv_color_hex(0xFF6B00), 0);

    // LoRa
    setDotColor(s_dotLora, loraOk);
    lv_label_set_text(s_lblLoraVal, loraOk ? "Actif" : "Inactif");
    lv_obj_set_style_text_color(s_lblLoraVal,
        loraOk ? lv_color_hex(0x00CC44) : lv_color_hex(0xFF1A1A), 0);

    // RSSI
    char rssiStr[32];
    snprintf(rssiStr, sizeof(rssiStr), "RSSI: %d dBm", net.loraRssi);
    lv_label_set_text(s_lblRssi, rssiStr);
}

lv_obj_t* uiNetworkGetScreen() { return s_screen; }
