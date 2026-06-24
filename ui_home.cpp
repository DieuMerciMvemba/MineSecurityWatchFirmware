/**
 * @file      ui_home.cpp
 * @brief     Dashboard principal – interface LVGL style smartwatch pro
 * @author    Mine Security Watch Team
 * @date      2026-06-24
 *
 * Design inspiré Apple Watch / Garmin :
 * - Fond noir profond avec accents orange sécurité
 * - Heure large et lisible en blanc
 * - Cards arrondies pour chaque métrique
 * - Bouton SOS rouge proéminent en bas
 * - Animations subtiles sur les icônes d'état
 */

#include "ui_home.h"
#include "config.h"

// ============================================================
//  Variables internes (handles LVGL)
// ============================================================
static lv_obj_t *s_screen        = nullptr;

// En-tête
static lv_obj_t *s_lblWorker     = nullptr;
static lv_obj_t *s_lblTime       = nullptr;
static lv_obj_t *s_lblZone       = nullptr;

// Indicateurs réseau
static lv_obj_t *s_lblWifi       = nullptr;
static lv_obj_t *s_lblLora       = nullptr;

// Cards métriques
static lv_obj_t *s_lblBattery    = nullptr;
static lv_obj_t *s_barBattery    = nullptr;
static lv_obj_t *s_lblSteps      = nullptr;
static lv_obj_t *s_lblTemp       = nullptr;
static lv_obj_t *s_lblMotion     = nullptr;

// Bouton SOS
static lv_obj_t *s_btnSOS        = nullptr;

// ============================================================
//  Styles réutilisables
// ============================================================
static lv_style_t s_styleScreen;
static lv_style_t s_styleCard;
static lv_style_t s_styleCardLabel;
static lv_style_t s_styleMetricVal;
static lv_style_t s_styleSOS;
static bool       s_stylesInit = false;

// ============================================================
//  Callback SOS (déclaré dans le .ino)
// ============================================================
extern void onSOSPressed();

static void sos_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_LONG_PRESSED) {
        Serial.println("[UI] 🆘 Bouton SOS appui long détecté !");
        onSOSPressed();
    } else if (code == LV_EVENT_PRESSED) {
        // Feedback visuel immédiat
        lv_obj_set_style_bg_color(s_btnSOS, lv_color_hex(0xFF4040), 0);
    } else if (code == LV_EVENT_RELEASED) {
        lv_obj_set_style_bg_color(s_btnSOS, lv_color_hex(0xFF1A1A), 0);
    }
}

// ============================================================
//  Initialisation des styles
// ============================================================
static void initStyles() {
    if (s_stylesInit) return;
    s_stylesInit = true;

    // Style écran fond noir
    lv_style_init(&s_styleScreen);
    lv_style_set_bg_color(&s_styleScreen, lv_color_hex(0x0A0A0A));
    lv_style_set_bg_opa(&s_styleScreen, LV_OPA_COVER);
    lv_style_set_border_width(&s_styleScreen, 0);

    // Style card foncé arrondie
    lv_style_init(&s_styleCard);
    lv_style_set_bg_color(&s_styleCard, lv_color_hex(0x1A1A1A));
    lv_style_set_bg_opa(&s_styleCard, LV_OPA_COVER);
    lv_style_set_radius(&s_styleCard, 16);
    lv_style_set_border_width(&s_styleCard, 1);
    lv_style_set_border_color(&s_styleCard, lv_color_hex(0x333333));
    lv_style_set_pad_all(&s_styleCard, 10);

    // Label titre card
    lv_style_init(&s_styleCardLabel);
    lv_style_set_text_color(&s_styleCardLabel, lv_color_hex(0x888888));
    lv_style_set_text_font(&s_styleCardLabel, &lv_font_montserrat_12);

    // Valeur métrique
    lv_style_init(&s_styleMetricVal);
    lv_style_set_text_color(&s_styleMetricVal, lv_color_hex(0xFFFFFF));
    lv_style_set_text_font(&s_styleMetricVal, &lv_font_montserrat_16);

    // Bouton SOS
    lv_style_init(&s_styleSOS);
    lv_style_set_bg_color(&s_styleSOS, lv_color_hex(0xFF1A1A));
    lv_style_set_bg_opa(&s_styleSOS, LV_OPA_COVER);
    lv_style_set_radius(&s_styleSOS, 20);
    lv_style_set_border_width(&s_styleSOS, 3);
    lv_style_set_border_color(&s_styleSOS, lv_color_hex(0xFF6666));
    lv_style_set_text_color(&s_styleSOS, lv_color_hex(0xFFFFFF));
    lv_style_set_text_font(&s_styleSOS, &lv_font_montserrat_18);
    lv_style_set_shadow_width(&s_styleSOS, 20);
    lv_style_set_shadow_color(&s_styleSOS, lv_color_hex(0xFF0000));
    lv_style_set_shadow_opa(&s_styleSOS, LV_OPA_50);
}

// ============================================================
//  Création d'une card métrique
// ============================================================
static lv_obj_t* createCard(lv_obj_t *parent, int x, int y, int w, int h,
                              const char *title, lv_obj_t **lblVal,
                              lv_color_t accentColor) {
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_size(card, w, h);
    lv_obj_add_style(card, &s_styleCard, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    // Barre accent colorée en haut
    lv_obj_t *accent = lv_obj_create(card);
    lv_obj_set_pos(accent, 0, 0);
    lv_obj_set_size(accent, w - 20, 3);
    lv_obj_set_style_bg_color(accent, accentColor, 0);
    lv_obj_set_style_bg_opa(accent, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(accent, 0, 0);
    lv_obj_set_style_radius(accent, 2, 0);

    // Label titre
    lv_obj_t *lblTitle = lv_label_create(card);
    lv_label_set_text(lblTitle, title);
    lv_obj_set_pos(lblTitle, 0, 8);
    lv_obj_add_style(lblTitle, &s_styleCardLabel, 0);

    // Label valeur
    *lblVal = lv_label_create(card);
    lv_label_set_text(*lblVal, "--");
    lv_obj_set_pos(*lblVal, 0, 26);
    lv_obj_add_style(*lblVal, &s_styleMetricVal, 0);

    return card;
}

// ============================================================
//  Construction de l'écran Home
// ============================================================
lv_obj_t* uiHomeCreate(lv_obj_t *parent) {
    initStyles();

    // --- Écran principal ---
    s_screen = lv_obj_create(parent);
    lv_obj_add_style(s_screen, &s_styleScreen, 0);
    lv_obj_set_size(s_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_flex_flow(s_screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(s_screen, 8, 0);
    lv_obj_set_style_pad_gap(s_screen, 6, 0);
    lv_obj_set_scroll_dir(s_screen, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(s_screen, LV_SCROLLBAR_MODE_AUTO);

    int SW = LV_HOR_RES;  // Largeur écran (240 ou 480px)

    // --------------------------------------------------------
    //  EN-TÊTE : Logo MSW + Nom mineur
    // --------------------------------------------------------
    lv_obj_t *header = lv_obj_create(s_screen);
    lv_obj_set_size(header, LV_PCT(100), 52);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x0D0D0D), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(header, lv_color_hex(0xFF6B00), 0);
    lv_obj_set_style_border_width(header, 1, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(header, 8, 0);

    // Indicateur orange MSW
    lv_obj_t *dot = lv_obj_create(header);
    lv_obj_set_size(dot, 8, 8);
    lv_obj_set_style_bg_color(dot, lv_color_hex(0xFF6B00), 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(dot, 0, 0);

    // Container pour le nom et zone
    lv_obj_t *nameContainer = lv_obj_create(header);
    lv_obj_set_flex_flow(nameContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(nameContainer, 0, 0);
    lv_obj_set_style_border_width(nameContainer, 0, 0);
    lv_obj_set_style_bg_opa(nameContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_size(nameContainer, LV_PCT(60), LV_SIZE_CONTENT);

    // Nom du mineur
    s_lblWorker = lv_label_create(nameContainer);
    lv_label_set_text(s_lblWorker, CFG_WORKER_NAME);
    lv_obj_set_style_text_color(s_lblWorker, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(s_lblWorker, &lv_font_montserrat_14, 0);

    // Zone
    s_lblZone = lv_label_create(nameContainer);
    lv_label_set_text(s_lblZone, CFG_WORKER_ZONE);
    lv_obj_set_style_text_color(s_lblZone, lv_color_hex(0xFF6B00), 0);
    lv_obj_set_style_text_font(s_lblZone, &lv_font_montserrat_10, 0);

    // Indicateurs réseau (droite du header)
    lv_obj_t *netContainer = lv_obj_create(header);
    lv_obj_set_flex_flow(netContainer, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(netContainer, 0, 0);
    lv_obj_set_style_border_width(netContainer, 0, 0);
    lv_obj_set_style_bg_opa(netContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_gap(netContainer, 8, 0);

    s_lblWifi = lv_label_create(netContainer);
    lv_label_set_text(s_lblWifi, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(s_lblWifi, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(s_lblWifi, &lv_font_montserrat_16, 0);

    s_lblLora = lv_label_create(netContainer);
    lv_label_set_text(s_lblLora, LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_text_color(s_lblLora, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(s_lblLora, &lv_font_montserrat_16, 0);

    // --------------------------------------------------------
    //  HEURE CENTRALE – Grande et lisible
    // --------------------------------------------------------
    s_lblTime = lv_label_create(s_screen);
    lv_label_set_text(s_lblTime, "00:00");
    lv_obj_set_style_text_color(s_lblTime, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(s_lblTime, &lv_font_montserrat_40, 0);

    // --------------------------------------------------------
    //  GRILLE 2x2 des métriques
    // --------------------------------------------------------
    lv_obj_t *metricsContainer = lv_obj_create(s_screen);
    lv_obj_set_size(metricsContainer, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(metricsContainer, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(metricsContainer, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(metricsContainer, 0, 0);
    lv_obj_set_style_pad_gap(metricsContainer, 6, 6);
    lv_obj_set_style_border_width(metricsContainer, 0, 0);
    lv_obj_set_style_bg_opa(metricsContainer, LV_OPA_TRANSP, 0);

    int cardW = LV_PCT(48);
    int cardH = 60;

    // Card Batterie – Orange
    lv_obj_t *cardBat = createCard(metricsContainer, 0, 0, cardW, cardH,
                                    LV_SYMBOL_CHARGE " BATT.", &s_lblBattery,
                                    lv_color_hex(0xFF6B00));
    (void)cardBat;
    // Barre de batterie sous la valeur
    s_barBattery = lv_bar_create(cardBat);
    lv_obj_set_pos(s_barBattery, 0, 42);
    lv_obj_set_size(s_barBattery, LV_PCT(100), 6);
    lv_bar_set_range(s_barBattery, 0, 100);
    lv_bar_set_value(s_barBattery, 85, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_barBattery, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_color(s_barBattery, lv_color_hex(0xFF6B00),
                               LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_barBattery, 3, 0);
    lv_obj_set_style_radius(s_barBattery, 3, LV_PART_INDICATOR);

    // Card Pas – Vert
    createCard(metricsContainer, 0, 0, cardW, cardH,
               LV_SYMBOL_UP " PAS", &s_lblSteps, lv_color_hex(0x00CC44));

    // Card Température – Bleu
    createCard(metricsContainer, 0, 0, cardW, cardH,
               LV_SYMBOL_WARNING " TEMP", &s_lblTemp, lv_color_hex(0x0099FF));

    // Card Mouvement – Orange dim
    createCard(metricsContainer, 0, 0, cardW, cardH,
               LV_SYMBOL_LOOP " MVMT", &s_lblMotion, lv_color_hex(0xFF6B00));

    // --------------------------------------------------------
    //  BOUTON SOS – Grande zone tactile rouge
    // --------------------------------------------------------
    s_btnSOS = lv_button_create(s_screen);
    lv_obj_set_size(s_btnSOS, LV_PCT(92), 48);
    lv_obj_add_style(s_btnSOS, &s_styleSOS, 0);
    lv_obj_add_event_cb(s_btnSOS, sos_event_cb, LV_EVENT_ALL, nullptr);
    // Activer l'appui long (2 secondes)
    lv_obj_set_style_anim_duration(s_btnSOS, CFG_SOS_HOLD_MS, 0);

    lv_obj_t *lblSOS = lv_label_create(s_btnSOS);
    lv_label_set_text(lblSOS, LV_SYMBOL_WARNING "  SOS  " LV_SYMBOL_WARNING);
    lv_obj_center(lblSOS);
    lv_obj_set_style_text_font(lblSOS, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lblSOS, lv_color_hex(0xFFFFFF), 0);

    return s_screen;
}

// ============================================================
//  Mise à jour des données
// ============================================================
void uiHomeUpdate(const SensorData &data, const NetworkStatus &net) {
    if (!s_screen) return;

    // Batterie
    char batStr[16];
    snprintf(batStr, sizeof(batStr), "%.0f%%", data.battery);
    lv_label_set_text(s_lblBattery, batStr);
    lv_bar_set_value(s_barBattery, (int)data.battery, LV_ANIM_ON);

    // Couleur batterie selon niveau
    lv_color_t batColor;
    if (data.battery > 50)       batColor = lv_color_hex(0x00CC44);
    else if (data.battery > 20)  batColor = lv_color_hex(0xFF6B00);
    else                          batColor = lv_color_hex(0xFF1A1A);
    lv_obj_set_style_bg_color(s_barBattery, batColor, LV_PART_INDICATOR);
    lv_obj_set_style_text_color(s_lblBattery, batColor, 0);

    // Pas
    char stepsStr[16];
    snprintf(stepsStr, sizeof(stepsStr), "%ld", data.steps);
    lv_label_set_text(s_lblSteps, stepsStr);

    // Température
    char tempStr[16];
    snprintf(tempStr, sizeof(tempStr), "%.1f\xC2\xB0""C", data.temperature);
    lv_label_set_text(s_lblTemp, tempStr);

    // Mouvement
    const char *motions[] = {"Immobile", "Marche", "Course", "CHUTE!"};
    int mi = (int)data.motion;
    if (mi < 0 || mi > 3) mi = 0;
    lv_label_set_text(s_lblMotion, motions[mi]);

    // Couleur mouvement
    lv_color_t motColor = (data.motion == MOTION_FALL) ?
                           lv_color_hex(0xFF1A1A) : lv_color_hex(0xFFFFFF);
    lv_obj_set_style_text_color(s_lblMotion, motColor, 0);

    // État WiFi
    bool wifiOk = (net.wifiState == NET_CONNECTED);
    lv_obj_set_style_text_color(s_lblWifi,
        wifiOk ? lv_color_hex(0x00CC44) : lv_color_hex(0x888888), 0);

    // État LoRa
    bool loraOk = (net.loraState == NET_CONNECTED);
    lv_obj_set_style_text_color(s_lblLora,
        loraOk ? lv_color_hex(0x00CC44) : lv_color_hex(0x888888), 0);
}

void uiHomeSetTime(int h, int m, int s) {
    if (!s_lblTime) return;
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d", h, m);
    lv_label_set_text(s_lblTime, buf);
    (void)s;  // Secondes pas affichées sur le dashboard
}

lv_obj_t* uiHomeGetScreen() {
    return s_screen;
}
