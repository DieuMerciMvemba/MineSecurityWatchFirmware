/**
 * @file      ui_health.cpp
 * @brief     Écran Santé – style Garmin avec arcs de progression
 * @author    Mine Security Watch Team
 * @date      2026-06-24
 */

#include "ui_health.h"
#include "config.h"

static lv_obj_t *s_screen     = nullptr;
static lv_obj_t *s_arcSteps   = nullptr;
static lv_obj_t *s_lblSteps   = nullptr;
static lv_obj_t *s_lblActivity = nullptr;
static lv_obj_t *s_lblTemp    = nullptr;
static lv_obj_t *s_lblMotion  = nullptr;
static lv_obj_t *s_barRisk    = nullptr;
static lv_obj_t *s_lblRisk    = nullptr;
static lv_obj_t *s_lblRiskVal = nullptr;

// Objectif journalier de pas
static const int32_t STEP_GOAL = 8000;

static const char* riskLabel(RiskLevel r) {
    switch (r) {
        case RISK_LOW:      return "Faible";
        case RISK_MEDIUM:   return "Moyen";
        case RISK_HIGH:     return "Élevé";
        case RISK_CRITICAL: return "CRITIQUE";
        default: return "?";
    }
}

static lv_color_t riskColor(RiskLevel r) {
    switch (r) {
        case RISK_LOW:      return lv_color_hex(0x00CC44);
        case RISK_MEDIUM:   return lv_color_hex(0xFF6B00);
        case RISK_HIGH:     return lv_color_hex(0xFF3300);
        case RISK_CRITICAL: return lv_color_hex(0xFF1A1A);
        default: return lv_color_hex(0x888888);
    }
}

/**
 * @brief Calcule le niveau de risque à partir des données capteurs.
 */
static RiskLevel computeRisk(const SensorData &d) {
    if (d.motion == MOTION_FALL)  return RISK_CRITICAL;
    if (d.battery < 10)          return RISK_HIGH;
    if (d.battery < 20)          return RISK_MEDIUM;
    return RISK_LOW;
}

lv_obj_t* uiHealthCreate() {
    s_screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(0x0A0A0A), 0);
    lv_obj_set_style_bg_opa(s_screen, LV_OPA_COVER, 0);
    lv_obj_set_size(s_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_flex_flow(s_screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(s_screen, 8, 0);
    lv_obj_set_style_pad_gap(s_screen, 6, 0);
    lv_obj_set_scroll_dir(s_screen, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(s_screen, LV_SCROLLBAR_MODE_AUTO);

    // --------------------------------------------------------
    //  TITRE
    // --------------------------------------------------------
    lv_obj_t *lblTitle = lv_label_create(s_screen);
    lv_label_set_text(lblTitle, LV_SYMBOL_PLUS " SANTÉ");
    lv_obj_set_style_text_color(lblTitle, lv_color_hex(0xFF6B00), 0);
    lv_obj_set_style_text_font(lblTitle, &lv_font_montserrat_16, 0);

    // Ligne séparatrice
    lv_obj_t *sep = lv_obj_create(s_screen);
    lv_obj_set_size(sep, LV_PCT(100), 2);
    lv_obj_set_style_bg_color(sep, lv_color_hex(0xFF6B00), 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_30, 0);
    lv_obj_set_style_border_width(sep, 0, 0);

    // --------------------------------------------------------
    //  ARC DE PROGRESSION – PAS
    // --------------------------------------------------------
    lv_obj_t *arcContainer = lv_obj_create(s_screen);
    lv_obj_set_size(arcContainer, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_border_width(arcContainer, 0, 0);
    lv_obj_set_style_bg_opa(arcContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(arcContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(arcContainer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    s_arcSteps = lv_arc_create(arcContainer);
    lv_arc_set_rotation(s_arcSteps, 135);
    lv_arc_set_bg_angles(s_arcSteps, 0, 270);
    lv_arc_set_value(s_arcSteps, 0);
    lv_arc_set_range(s_arcSteps, 0, 100);
    lv_obj_set_size(s_arcSteps, 100, 100);
    lv_obj_set_style_arc_color(s_arcSteps, lv_color_hex(0x00CC44),
                                LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(s_arcSteps, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_arcSteps, lv_color_hex(0x1E1E1E),
                                LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_arcSteps, 10, LV_PART_MAIN);
    lv_obj_remove_style(s_arcSteps, nullptr, LV_PART_KNOB);

    // Valeur pas au centre de l'arc
    s_lblSteps = lv_label_create(arcContainer);
    lv_label_set_text(s_lblSteps, "0");
    lv_obj_align_to(s_lblSteps, s_arcSteps, LV_ALIGN_CENTER, 0, -6);
    lv_obj_set_style_text_color(s_lblSteps, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(s_lblSteps, &lv_font_montserrat_18, 0);

    lv_obj_t *lblStepsUnit = lv_label_create(arcContainer);
    lv_label_set_text(lblStepsUnit, "pas");
    lv_obj_align_to(lblStepsUnit, s_arcSteps, LV_ALIGN_CENTER, 0, 12);
    lv_obj_set_style_text_color(lblStepsUnit, lv_color_hex(0x00CC44), 0);
    lv_obj_set_style_text_font(lblStepsUnit, &lv_font_montserrat_10, 0);

    // --------------------------------------------------------
    //  CARDS INFÉRIEURES
    // --------------------------------------------------------
    lv_obj_t *cardsContainer = lv_obj_create(s_screen);
    lv_obj_set_size(cardsContainer, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(cardsContainer, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(cardsContainer, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(cardsContainer, 0, 0);
    lv_obj_set_style_pad_gap(cardsContainer, 6, 6);
    lv_obj_set_style_border_width(cardsContainer, 0, 0);
    lv_obj_set_style_bg_opa(cardsContainer, LV_OPA_TRANSP, 0);

    int cardW = LV_PCT(48);
    int cardH = 52;

    // Card Activité
    lv_obj_t *cAct = lv_obj_create(cardsContainer);
    lv_obj_set_size(cAct, cardW, cardH);
    lv_obj_set_style_bg_color(cAct, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_radius(cAct, 12, 0);
    lv_obj_set_style_border_color(cAct, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(cAct, 1, 0);
    lv_obj_set_style_pad_all(cAct, 8, 0);
    lv_obj_clear_flag(cAct, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lActT = lv_label_create(cAct);
    lv_label_set_text(lActT, "ACTIVITÉ");
    lv_obj_set_style_text_color(lActT, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(lActT, &lv_font_montserrat_10, 0);

    s_lblActivity = lv_label_create(cAct);
    lv_label_set_text(s_lblActivity, "--");
    lv_obj_set_style_text_color(s_lblActivity, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(s_lblActivity, &lv_font_montserrat_14, 0);

    // Card Température
    lv_obj_t *cTemp = lv_obj_create(cardsContainer);
    lv_obj_set_size(cTemp, cardW, cardH);
    lv_obj_set_style_bg_color(cTemp, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_radius(cTemp, 12, 0);
    lv_obj_set_style_border_color(cTemp, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(cTemp, 1, 0);
    lv_obj_set_style_pad_all(cTemp, 8, 0);
    lv_obj_clear_flag(cTemp, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lTmpT = lv_label_create(cTemp);
    lv_label_set_text(lTmpT, "TEMPÉRATURE");
    lv_obj_set_style_text_color(lTmpT, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(lTmpT, &lv_font_montserrat_10, 0);

    s_lblTemp = lv_label_create(cTemp);
    lv_label_set_text(s_lblTemp, "--°C");
    lv_obj_set_style_text_color(s_lblTemp, lv_color_hex(0x0099FF), 0);
    lv_obj_set_style_text_font(s_lblTemp, &lv_font_montserrat_14, 0);

    // --------------------------------------------------------
    //  CARD NIVEAU DE RISQUE (pleine largeur)
    // --------------------------------------------------------
    lv_obj_t *cRisk = lv_obj_create(s_screen);
    lv_obj_set_size(cRisk, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(cRisk, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_radius(cRisk, 12, 0);
    lv_obj_set_style_border_color(cRisk, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(cRisk, 1, 0);
    lv_obj_set_style_pad_all(cRisk, 10, 0);
    lv_obj_clear_flag(cRisk, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(cRisk, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *lRiskT = lv_label_create(cRisk);
    lv_label_set_text(lRiskT, LV_SYMBOL_WARNING " NIVEAU DE RISQUE");
    lv_obj_set_style_text_color(lRiskT, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(lRiskT, &lv_font_montserrat_12, 0);

    s_lblRiskVal = lv_label_create(cRisk);
    lv_label_set_text(s_lblRiskVal, "Faible");
    lv_obj_set_style_text_color(s_lblRiskVal, lv_color_hex(0x00CC44), 0);
    lv_obj_set_style_text_font(s_lblRiskVal, &lv_font_montserrat_16, 0);

    s_barRisk = lv_bar_create(cRisk);
    lv_obj_set_size(s_barRisk, LV_PCT(100), 8);
    lv_bar_set_range(s_barRisk, 0, 3);
    lv_bar_set_value(s_barRisk, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_barRisk, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_color(s_barRisk, lv_color_hex(0x00CC44),
                               LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_barRisk, 4, 0);
    lv_obj_set_style_radius(s_barRisk, 4, LV_PART_INDICATOR);

    return s_screen;
}

void uiHealthUpdate(const SensorData &data) {
    if (!s_screen) return;

    // Pas – arc de progression
    int pct = (int)((data.steps * 100L) / STEP_GOAL);
    if (pct > 100) pct = 100;
    lv_arc_set_value(s_arcSteps, pct);
    char buf[32];
    snprintf(buf, sizeof(buf), "%ld", data.steps);
    lv_label_set_text(s_lblSteps, buf);

    // Activité
    const char *acts[] = {"Immobile", "Marche", "Course", "CHUTE !"};
    int mi = (int)data.motion;
    if (mi < 0 || mi > 3) mi = 0;
    lv_label_set_text(s_lblActivity, acts[mi]);

    // Température
    snprintf(buf, sizeof(buf), "%.1f\xC2\xB0""C", data.temperature);
    lv_label_set_text(s_lblTemp, buf);

    // Risque
    RiskLevel risk = computeRisk(data);
    lv_bar_set_value(s_barRisk, (int)risk, LV_ANIM_ON);
    lv_label_set_text(s_lblRiskVal, riskLabel(risk));
    lv_color_t rc = riskColor(risk);
    lv_obj_set_style_text_color(s_lblRiskVal, rc, 0);
    lv_obj_set_style_bg_color(s_barRisk, rc, LV_PART_INDICATOR);
}

lv_obj_t* uiHealthGetScreen() { return s_screen; }
