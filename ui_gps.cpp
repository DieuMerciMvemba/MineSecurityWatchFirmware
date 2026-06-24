#include "ui_gps.h"
#include "config.h"
#include "sd_logger.h"
#include <time.h>

static lv_obj_t *s_screen = nullptr;

static lv_obj_t *s_lblLat = nullptr;
static lv_obj_t *s_lblLng = nullptr;
static lv_obj_t *s_lblSpeed = nullptr;
static lv_obj_t *s_lblSatellites = nullptr;
static lv_obj_t *s_lblDatetime = nullptr;
static lv_obj_t *s_lblStatus = nullptr;

static lv_timer_t *s_gpsTimer = nullptr;
static uint32_t s_lastLogTime = 0;

// Timer pour logging automatique des positions GPS
static void gps_log_timer_cb(lv_timer_t *timer) {
    uint32_t now = millis();
    
    // Logger la position toutes les 30 secondes
    if (now - s_lastLogTime > 30000) {
        // Récupérer les valeurs actuelles depuis les labels
        const char *latText = lv_label_get_text(s_lblLat);
        const char *lngText = lv_label_get_text(s_lblLng);
        const char *speedText = lv_label_get_text(s_lblSpeed);
        const char *satText = lv_label_get_text(s_lblSatellites);
        
        double lat = atof(latText);
        double lng = atof(lngText);
        float speed = atof(speedText);
        uint8_t satellites = atoi(satText);
        
        LogGPSEvent event;
        event.lat = lat;
        event.lng = lng;
        event.speed = speed;
        event.satellites = satellites;
        event.timestamp = now;
        
        sdLogGPS(event);
        s_lastLogTime = now;
    }
}

lv_obj_t* uiGpsCreate() {
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

    // Titre
    lv_obj_t *title = lv_label_create(s_screen);
    lv_label_set_text(title, LV_SYMBOL_GPS " GPS TRACKING");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFF6B00), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);

    // Séparateur
    lv_obj_t *sep = lv_obj_create(s_screen);
    lv_obj_set_size(sep, LV_PCT(100), 2);
    lv_obj_set_style_bg_color(sep, lv_color_hex(0xFF6B00), 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_30, 0);
    lv_obj_set_style_border_width(sep, 0, 0);

    // Container pour les données GPS
    lv_obj_t *dataContainer = lv_obj_create(s_screen);
    lv_obj_set_size(dataContainer, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(dataContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(dataContainer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(dataContainer, 0, 0);
    lv_obj_set_style_pad_gap(dataContainer, 8, 0);
    lv_obj_set_style_border_width(dataContainer, 0, 0);
    lv_obj_set_style_bg_opa(dataContainer, LV_OPA_TRANSP, 0);

    // Statut GPS
    s_lblStatus = lv_label_create(dataContainer);
    lv_label_set_text(s_lblStatus, "Recherche satellites...");
    lv_obj_set_style_text_color(s_lblStatus, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(s_lblStatus, &lv_font_montserrat_12, 0);

    // Latitude
    lv_obj_t *latRow = lv_obj_create(dataContainer);
    lv_obj_set_size(latRow, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(latRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(latRow, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(latRow, 0, 0);
    lv_obj_set_style_pad_gap(latRow, 8, 0);
    lv_obj_set_style_border_width(latRow, 0, 0);
    lv_obj_set_style_bg_opa(latRow, LV_OPA_TRANSP, 0);

    lv_obj_t *latLabel = lv_label_create(latRow);
    lv_label_set_text(latLabel, "LAT:");
    lv_obj_set_style_text_color(latLabel, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_font(latLabel, &lv_font_montserrat_12, 0);

    s_lblLat = lv_label_create(latRow);
    lv_label_set_text(s_lblLat, "0.000000");
    lv_obj_set_style_text_color(s_lblLat, lv_color_hex(0x0099FF), 0);
    lv_obj_set_style_text_font(s_lblLat, &lv_font_montserrat_14, 0);

    // Longitude
    lv_obj_t *lngRow = lv_obj_create(dataContainer);
    lv_obj_set_size(lngRow, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(lngRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(lngRow, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(lngRow, 0, 0);
    lv_obj_set_style_pad_gap(lngRow, 8, 0);
    lv_obj_set_style_border_width(lngRow, 0, 0);
    lv_obj_set_style_bg_opa(lngRow, LV_OPA_TRANSP, 0);

    lv_obj_t *lngLabel = lv_label_create(lngRow);
    lv_label_set_text(lngLabel, "LNG:");
    lv_obj_set_style_text_color(lngLabel, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_font(lngLabel, &lv_font_montserrat_12, 0);

    s_lblLng = lv_label_create(lngRow);
    lv_label_set_text(s_lblLng, "0.000000");
    lv_obj_set_style_text_color(s_lblLng, lv_color_hex(0x0099FF), 0);
    lv_obj_set_style_text_font(s_lblLng, &lv_font_montserrat_14, 0);

    // Vitesse
    lv_obj_t *speedRow = lv_obj_create(dataContainer);
    lv_obj_set_size(speedRow, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(speedRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(speedRow, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(speedRow, 0, 0);
    lv_obj_set_style_pad_gap(speedRow, 8, 0);
    lv_obj_set_style_border_width(speedRow, 0, 0);
    lv_obj_set_style_bg_opa(speedRow, LV_OPA_TRANSP, 0);

    lv_obj_t *speedLabel = lv_label_create(speedRow);
    lv_label_set_text(speedLabel, LV_SYMBOL_UP " VITESSE:");
    lv_obj_set_style_text_color(speedLabel, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_font(speedLabel, &lv_font_montserrat_12, 0);

    s_lblSpeed = lv_label_create(speedRow);
    lv_label_set_text(s_lblSpeed, "0.0 km/h");
    lv_obj_set_style_text_color(s_lblSpeed, lv_color_hex(0x00CC44), 0);
    lv_obj_set_style_text_font(s_lblSpeed, &lv_font_montserrat_14, 0);

    // Satellites
    lv_obj_t *satRow = lv_obj_create(dataContainer);
    lv_obj_set_size(satRow, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(satRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(satRow, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(satRow, 0, 0);
    lv_obj_set_style_pad_gap(satRow, 8, 0);
    lv_obj_set_style_border_width(satRow, 0, 0);
    lv_obj_set_style_bg_opa(satRow, LV_OPA_TRANSP, 0);

    lv_obj_t *satLabel = lv_label_create(satRow);
    lv_label_set_text(satLabel, LV_SYMBOL_GPS " SATELLITES:");
    lv_obj_set_style_text_color(satLabel, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_font(satLabel, &lv_font_montserrat_12, 0);

    s_lblSatellites = lv_label_create(satRow);
    lv_label_set_text(s_lblSatellites, "0");
    lv_obj_set_style_text_color(s_lblSatellites, lv_color_hex(0xFF6B00), 0);
    lv_obj_set_style_text_font(s_lblSatellites, &lv_font_montserrat_14, 0);

    // Date/Heure GPS
    s_lblDatetime = lv_label_create(dataContainer);
    lv_label_set_text(s_lblDatetime, "--/--/-- --:--:--");
    lv_obj_set_style_text_color(s_lblDatetime, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_font(s_lblDatetime, &lv_font_montserrat_10, 0);

    // Timer pour logging automatique
    s_gpsTimer = lv_timer_create(gps_log_timer_cb, 1000, nullptr);

    return s_screen;
}

void uiGpsUpdate(double lat, double lng, float speed, uint8_t satellites) {
    if (!s_screen) return;

    lv_label_set_text_fmt(s_lblLat, "%.6f", lat);
    lv_label_set_text_fmt(s_lblLng, "%.6f", lng);
    lv_label_set_text_fmt(s_lblSpeed, "%.1f km/h", speed);
    lv_label_set_text_fmt(s_lblSatellites, "%u", satellites);

    if (satellites >= 4) {
        lv_label_set_text(s_lblStatus, "GPS Fix OK");
        lv_obj_set_style_text_color(s_lblStatus, lv_color_hex(0x00CC44), 0);
    } else if (satellites > 0) {
        lv_label_set_text(s_lblStatus, "Acquisition...");
        lv_obj_set_style_text_color(s_lblStatus, lv_color_hex(0xFF6B00), 0);
    } else {
        lv_label_set_text(s_lblStatus, "Recherche satellites...");
        lv_obj_set_style_text_color(s_lblStatus, lv_color_hex(0x888888), 0);
    }

    // Mettre à jour la date/heure GPS
    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", &timeinfo);
    lv_label_set_text(s_lblDatetime, buffer);
}

void uiGpsShow() {
    if (s_screen) {
        lv_screen_load(s_screen);
    }
}

void uiGpsHide() {
    if (s_screen) {
        lv_obj_add_flag(s_screen, LV_OBJ_FLAG_HIDDEN);
    }
}
