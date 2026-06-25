#include "ui_compass.h"
#include "config.h"
#include <cmath>

static lv_obj_t *s_screen = nullptr;
static lv_obj_t *s_arcCompass = nullptr;
static lv_obj_t *s_lblHeading = nullptr;
static lv_obj_t *s_lblDirection = nullptr;
static lv_obj_t *s_lblRoll = nullptr;
static lv_obj_t *s_lblPitch = nullptr;
static lv_obj_t *s_lblStatus = nullptr;

// Convertir heading en direction cardinale
static const char* getCardinalDirection(float heading) {
    // Normaliser heading 0-360
    while (heading < 0) heading += 360;
    while (heading >= 360) heading -= 360;
    
    if (heading >= 337.5 || heading < 22.5) return "N";
    if (heading >= 22.5 && heading < 67.5) return "NE";
    if (heading >= 67.5 && heading < 112.5) return "E";
    if (heading >= 112.5 && heading < 157.5) return "SE";
    if (heading >= 157.5 && heading < 202.5) return "S";
    if (heading >= 202.5 && heading < 247.5) return "SO";
    if (heading >= 247.5 && heading < 292.5) return "O";
    if (heading >= 292.5 && heading < 337.5) return "NO";
    
    return "N";
}

// Obtenir la couleur selon l'orientation
static lv_color_t getOrientationColor(float heading) {
    while (heading < 0) heading += 360;
    while (heading >= 360) heading -= 360;
    
    // Nord = rouge, Est = vert, Sud = bleu, Ouest = orange
    if (heading >= 337.5 || heading < 22.5) return lv_color_hex(0xFF0000);
    if (heading >= 67.5 && heading < 112.5) return lv_color_hex(0x00CC44);
    if (heading >= 157.5 && heading < 202.5) return lv_color_hex(0x0099FF);
    if (heading >= 247.5 && heading < 292.5) return lv_color_hex(0xFF6B00);
    
    return lv_color_hex(0xFF6B00);
}

lv_obj_t* uiCompassCreate(lv_obj_t *parent) {
    s_screen = lv_obj_create(parent);
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(0x0A0A0A), 0);
    lv_obj_set_style_bg_opa(s_screen, LV_OPA_COVER, 0);
    lv_obj_set_size(s_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_flex_flow(s_screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_screen, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(s_screen, 8, 0);
    lv_obj_set_style_pad_gap(s_screen, 6, 0);
    lv_obj_set_scroll_dir(s_screen, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(s_screen, LV_SCROLLBAR_MODE_AUTO);

    // Titre
    lv_obj_t *title = lv_label_create(s_screen);
    lv_label_set_text(title, LV_SYMBOL_GPS " BOUSSOLE");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFF6B00), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);

    // Séparateur
    lv_obj_t *sep = lv_obj_create(s_screen);
    lv_obj_set_size(sep, LV_PCT(100), 2);
    lv_obj_set_style_bg_color(sep, lv_color_hex(0xFF6B00), 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_30, 0);
    lv_obj_set_style_border_width(sep, 0, 0);

    // Arc de boussole
    s_arcCompass = lv_arc_create(s_screen);
    lv_arc_set_rotation(s_arcCompass, 135);
    lv_arc_set_bg_angles(s_arcCompass, 0, 270);
    lv_arc_set_value(s_arcCompass, 0);
    lv_arc_set_range(s_arcCompass, 0, 360);
    lv_obj_set_size(s_arcCompass, 100, 100);
    lv_obj_set_style_arc_color(s_arcCompass, lv_color_hex(0xFF6B00), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(s_arcCompass, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_arcCompass, lv_color_hex(0x1E1E1E), LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_arcCompass, 10, LV_PART_MAIN);
    lv_obj_remove_style(s_arcCompass, nullptr, LV_PART_KNOB);
    lv_obj_clear_flag(s_arcCompass, LV_OBJ_FLAG_CLICKABLE);

    // Heading au centre (parent = s_arcCompass pour éviter d'être empilé en Flexbox)
    s_lblHeading = lv_label_create(s_arcCompass);
    lv_label_set_text(s_lblHeading, "0°");
    lv_obj_set_style_text_color(s_lblHeading, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(s_lblHeading, &lv_font_montserrat_24, 0);
    lv_obj_align(s_lblHeading, LV_ALIGN_CENTER, 0, -10);

    // Direction cardinale (parent = s_arcCompass)
    s_lblDirection = lv_label_create(s_arcCompass);
    lv_label_set_text(s_lblDirection, "N");
    lv_obj_set_style_text_color(s_lblDirection, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_text_font(s_lblDirection, &lv_font_montserrat_18, 0);
    lv_obj_align(s_lblDirection, LV_ALIGN_CENTER, 0, 12);

    // Container pour Roll/Pitch
    lv_obj_t *imuContainer = lv_obj_create(s_screen);
    lv_obj_set_size(imuContainer, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(imuContainer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(imuContainer, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(imuContainer, 0, 0);
    lv_obj_set_style_pad_gap(imuContainer, 16, 0);
    lv_obj_set_style_border_width(imuContainer, 0, 0);
    lv_obj_set_style_bg_opa(imuContainer, LV_OPA_TRANSP, 0);

    // Roll
    lv_obj_t *rollContainer = lv_obj_create(imuContainer);
    lv_obj_set_flex_flow(rollContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(rollContainer, 0, 0);
    lv_obj_set_style_pad_gap(rollContainer, 2, 0);
    lv_obj_set_style_border_width(rollContainer, 0, 0);
    lv_obj_set_style_bg_opa(rollContainer, LV_OPA_TRANSP, 0);

    lv_obj_t *rollLabel = lv_label_create(rollContainer);
    lv_label_set_text(rollLabel, "ROLL");
    lv_obj_set_style_text_color(rollLabel, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(rollLabel, &lv_font_montserrat_10, 0);

    s_lblRoll = lv_label_create(rollContainer);
    lv_label_set_text(s_lblRoll, "0°");
    lv_obj_set_style_text_color(s_lblRoll, lv_color_hex(0x0099FF), 0);
    lv_obj_set_style_text_font(s_lblRoll, &lv_font_montserrat_14, 0);

    // Pitch
    lv_obj_t *pitchContainer = lv_obj_create(imuContainer);
    lv_obj_set_flex_flow(pitchContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(pitchContainer, 0, 0);
    lv_obj_set_style_pad_gap(pitchContainer, 2, 0);
    lv_obj_set_style_border_width(pitchContainer, 0, 0);
    lv_obj_set_style_bg_opa(pitchContainer, LV_OPA_TRANSP, 0);

    lv_obj_t *pitchLabel = lv_label_create(pitchContainer);
    lv_label_set_text(pitchLabel, "PITCH");
    lv_obj_set_style_text_color(pitchLabel, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(pitchLabel, &lv_font_montserrat_10, 0);

    s_lblPitch = lv_label_create(pitchContainer);
    lv_label_set_text(s_lblPitch, "0°");
    lv_obj_set_style_text_color(s_lblPitch, lv_color_hex(0x0099FF), 0);
    lv_obj_set_style_text_font(s_lblPitch, &lv_font_montserrat_14, 0);

    // Statut
    s_lblStatus = lv_label_create(s_screen);
    lv_label_set_text(s_lblStatus, "IMU Actif");
    lv_obj_set_style_text_color(s_lblStatus, lv_color_hex(0x00CC44), 0);
    lv_obj_set_style_text_font(s_lblStatus, &lv_font_montserrat_10, 0);

    return s_screen;
}

void uiCompassUpdate(float heading, float roll, float pitch) {
    if (!s_screen) return;

    // Mettre à jour l'arc
    lv_arc_set_value(s_arcCompass, (int)heading);
    
    // Mettre à jour la couleur selon la direction
    lv_color_t dirColor = getOrientationColor(heading);
    lv_obj_set_style_arc_color(s_arcCompass, dirColor, LV_PART_INDICATOR);
    
    // Mettre à jour le heading
    lv_label_set_text_fmt(s_lblHeading, "%.0f°", heading);
    
    // Mettre à jour la direction cardinale
    const char *direction = getCardinalDirection(heading);
    lv_label_set_text(s_lblDirection, direction);
    lv_obj_set_style_text_color(s_lblDirection, dirColor, 0);
    
    // Mettre à jour roll et pitch
    lv_label_set_text_fmt(s_lblRoll, "%.1f°", roll);
    lv_label_set_text_fmt(s_lblPitch, "%.1f°", pitch);
    
    // Alerte si orientation inhabituelle (roll ou pitch > 45°)
    if (fabs(roll) > 45 || fabs(pitch) > 45) {
        lv_label_set_text(s_lblStatus, "⚠ Position inhabituelle");
        lv_obj_set_style_text_color(s_lblStatus, lv_color_hex(0xFF0000), 0);
    } else {
        lv_label_set_text(s_lblStatus, "IMU Actif");
        lv_obj_set_style_text_color(s_lblStatus, lv_color_hex(0x00CC44), 0);
    }
}

void uiCompassShow() {
    if (s_screen) {
        lv_screen_load(s_screen);
    }
}

void uiCompassHide() {
    if (s_screen) {
        lv_obj_add_flag(s_screen, LV_OBJ_FLAG_HIDDEN);
    }
}
