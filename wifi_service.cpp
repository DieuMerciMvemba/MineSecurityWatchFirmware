/**
 * @file      wifi_service.cpp
 * @brief     Implémentation du service WiFi avec mode hybride STA+AP, portail captif sécurisé et API locale
 * @author    Mine Security Watch Team
 * @date      2026-06-25
 */

#include "wifi_service.h"
#include "config.h"
#include "config_storage.h"
#include "sensor_service.h"
#include "battery_service.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// ============================================================
//  Variables internes
// ============================================================
static NetworkState s_wifiState = NET_DISCONNECTED;
static WebServer    s_server(80);
static DNSServer    s_dnsServer;
static bool         s_apActive = false;

// ============================================================
//  Pages du portail captif (HTML/CSS)
// ============================================================

static const char* HTML_HEADER = 
"<!DOCTYPE html><html lang='fr'><head>"
"<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>"
"<title>MSW - Configuration</title>"
"<style>"
"body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background-color: #0A0A0A; color: #FFFFFF; margin: 0; padding: 20px; display: flex; justify-content: center; }"
".container { width: 100%; max-width: 500px; background: #1A1A1A; border: 1px solid #333; border-radius: 16px; padding: 24px; box-shadow: 0 10px 25px rgba(0,0,0,0.5); }"
"h2 { color: #FF6B00; margin-top: 0; text-align: center; font-size: 24px; letter-spacing: 1px; }"
".logo { display: block; margin: 0 auto 16px; width: 60px; height: 60px; background: #FF6B00; border-radius: 50%; text-align: center; line-height: 60px; font-size: 32px; font-weight: bold; color: #FFF; }"
".section-title { font-size: 16px; color: #FF6B00; border-bottom: 2px solid #333; padding-bottom: 6px; margin: 24px 0 12px; font-weight: bold; }"
".form-group { margin-bottom: 16px; }"
"label { display: block; margin-bottom: 6px; font-size: 14px; color: #888; }"
"input[type='text'], input[type='password'], input[type='number'] { width: 100%; padding: 12px; background: #262626; border: 1px solid #444; border-radius: 8px; color: #FFF; font-size: 16px; box-sizing: border-box; transition: border-color 0.3s; }"
"input:focus { border-color: #FF6B00; outline: none; }"
"button { width: 100%; padding: 14px; background: #FF6B00; color: #FFF; border: none; border-radius: 8px; font-size: 16px; font-weight: bold; cursor: pointer; transition: background 0.3s; margin-top: 16px; }"
"button:hover { background: #E05E00; }"
".error-msg { color: #FF3333; font-size: 14px; text-align: center; margin-bottom: 12px; font-weight: bold; }"
".footer { text-align: center; margin-top: 24px; font-size: 11px; color: #666; }"
"</style></head><body><div class='container'>";

static const char* HTML_FOOTER = 
"<div class='footer'>Mine Security Watch &copy; 2026</div></div></body></html>";

static bool isAuthorized() {
    if (s_server.hasHeader("Cookie")) {
        String cookie = s_server.header("Cookie");
        if (cookie.indexOf("msw_auth=authorized") != -1) {
            return true;
        }
    }
    return false;
}

static void showLogin(const char *errorMsg = nullptr) {
    String html = String(HTML_HEADER);
    html += "<div class='logo'>🔒</div>";
    html += "<h2>Accès Sécurisé</h2>";
    
    if (errorMsg) {
        html += "<div class='error-msg'>";
        html += errorMsg;
        html += "</div>";
    }
    
    html += "<form action='/login' method='POST'>";
    html += "<div class='form-group'><label>Entrez le code PIN de configuration</label>";
    html += "<input type='password' name='pin' placeholder='••••••••' required autofocus></div>";
    html += "<button type='submit'>Valider</button>";
    html += "</form>";
    html += HTML_FOOTER;
    s_server.send(200, "text/html", html);
}

static void handleLogin() {
    if (s_server.method() != HTTP_POST) {
        s_server.send(405, "text/plain", "Method Not Allowed");
        return;
    }

    String pin = s_server.arg("pin");
    if (pin == "00000000") {
        // Authentifié avec succès - Définir cookie (expire après 1h)
        s_server.sendHeader("Set-Cookie", "msw_auth=authorized; Path=/; Max-Age=3600");
        s_server.sendHeader("Location", "/", true);
        s_server.send(302, "text/plain", "");
        Serial.println("[WIFI] Utilisateur Web authentifié par PIN");
    } else {
        showLogin("Code PIN incorrect ! Réessayez.");
    }
}

static void handleRoot() {
    if (!isAuthorized()) {
        showLogin();
        return;
    }

    String html = String(HTML_HEADER);
    html += "<div class='logo'>🆘</div>";
    html += "<h2>Configuration de la Montre</h2>";
    html += "<form action='/save' method='POST'>";
    
    // Section Identity
    html += "<div class='section-title'>Identité du Mineur</div>";
    html += "<div class='form-group'><label>Nom complet du Travailleur</label>";
    html += "<input type='text' name='worker_name' maxlength='32' value='" + String(g_config.workerName) + "' required></div>";
    html += "<div class='form-group'><label>Identifiant unique</label>";
    html += "<input type='text' name='worker_id' maxlength='16' value='" + String(g_config.workerId) + "' required></div>";
    html += "<div class='form-group'><label>Zone de travail</label>";
    html += "<input type='text' name='worker_zone' maxlength='16' value='" + String(g_config.workerZone) + "' required></div>";
    html += "<div class='form-group'><label>Nom du Site</label>";
    html += "<input type='text' name='site_name' maxlength='32' value='" + String(g_config.siteName) + "' required></div>";

    html += "<button type='submit'>Enregistrer et Redémarrer la montre</button>";
    html += "</form>";
    html += HTML_FOOTER;
    s_server.send(200, "text/html", html);
}

static void handleSave() {
    if (!isAuthorized()) {
        s_server.send(401, "text/plain", "Non autorise");
        return;
    }

    if (s_server.method() != HTTP_POST) {
        s_server.send(405, "text/plain", "Method Not Allowed");
        return;
    }

    // Récupérer et affecter les nouvelles valeurs d'identité
    strncpy(g_config.workerName, s_server.arg("worker_name").c_str(), sizeof(g_config.workerName) - 1);
    strncpy(g_config.workerId, s_server.arg("worker_id").c_str(), sizeof(g_config.workerId) - 1);
    strncpy(g_config.workerZone, s_server.arg("worker_zone").c_str(), sizeof(g_config.workerZone) - 1);
    strncpy(g_config.siteName, s_server.arg("site_name").c_str(), sizeof(g_config.siteName) - 1);

    // Sauvegarder en NVS
    configSave();

    // Renvoyer une page HTML de confirmation
    String html = String(HTML_HEADER);
    html += "<h2>Configuration Enregistrée !</h2>";
    html += "<p style='text-align:center;'>La montre va redémarrer dans un instant pour appliquer les nouveaux réglages.</p>";
    html += "<p style='text-align:center; color:#FF6B00; font-size:24px;'>🔄</p>";
    html += HTML_FOOTER;
    s_server.send(200, "text/html", html);

    // Attendre un peu puis redémarrer l'ESP32
    vTaskDelay(pdMS_TO_TICKS(1500));
    ESP.restart();
}

static void handleData() {
    SensorData data = sensorGetLatest();
    uint16_t voltage = batteryGetVoltage();
    
    String json = "{";
    json += "\"workerId\":\"" + String(g_config.workerId) + "\",";
    json += "\"workerName\":\"" + String(g_config.workerName) + "\",";
    json += "\"workerZone\":\"" + String(g_config.workerZone) + "\",";
    json += "\"battery\":" + String(data.battery, 1) + ",";
    json += "\"voltage\":" + String(voltage) + ",";
    json += "\"temperature\":" + String(data.temperature, 1) + ",";
    json += "\"steps\":" + String(data.steps) + ",";
    json += "\"motion\":\"" + String(sensorMotionStr(data.motion)) + "\",";
    json += "\"gps\":{";
    json += "\"lat\":" + String(data.latitude, 6) + ",";
    json += "\"lng\":" + String(data.longitude, 6) + ",";
    json += "\"valid\":" + String(data.gpsValid ? "true" : "false");
    json += "}";
    json += "}";
    
    s_server.send(200, "application/json", json);
}

static void handleNotFound() {
    if (!isAuthorized()) {
        showLogin();
    } else {
        // Rediriger vers l'IP locale de l'AP en cas de Captive Portal
        s_server.sendHeader("Location", "http://192.168.4.1/", true);
        s_server.send(302, "text/plain", "");
    }
}

// ============================================================
//  Implémentation
// ============================================================

void wifiInit() {
    // Choisir le mode en fonction de l'activation AP
    if (g_config.apEnabled) {
        WiFi.mode(WIFI_AP);
        wifiStartAP();
    } else {
        WiFi.mode(WIFI_STA);
        WiFi.setAutoReconnect(true);
        connectWifi();
    }
    Serial.println("[WIFI] Service initialisé");
}

bool connectWifi() {
    if (WiFi.isConnected()) {
        s_wifiState = NET_CONNECTED;
        return true;
    }

    s_wifiState = NET_CONNECTING;
    Serial.printf("[WIFI] Connexion à %s ...\n", g_config.wifiSsid);

    // Utilisation des credentials de la config active
    WiFi.begin(g_config.wifiSsid, g_config.wifiPassword);

    return WiFi.isConnected();
}

void reconnectWifi() {
    Serial.println("[WIFI] Reconnexion...");
    WiFi.disconnect(false);
    connectWifi();
}

bool isWifiConnected() {
    return WiFi.isConnected();
}

String wifiGetIP() {
    if (WiFi.isConnected()) {
        return WiFi.localIP().toString();
    }
    return String("0.0.0.0");
}

String wifiGetAPIP() {
    if (s_apActive) {
        return WiFi.softAPIP().toString();
    }
    return String("0.0.0.0");
}

NetworkState wifiGetState() {
    if (g_config.apEnabled && s_apActive) {
        return NET_CONNECTED;
    }
    if (WiFi.isConnected()) s_wifiState = NET_CONNECTED;
    return s_wifiState;
}

void wifiStartAP() {
    char apSsid[32];
    snprintf(apSsid, sizeof(apSsid), "MSW-%s", g_config.workerId);
    
    // Configurer l'AP en WPA2 avec mot de passe "11111111"
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    WiFi.softAP(apSsid, "11111111");

    s_apActive = true;
    Serial.printf("[WIFI] AP démarré : %s (IP: 192.168.4.1, Pass: 11111111)\n", apSsid);

    // Lancer le DNS Server pour le portail captif
    s_dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));

    // Définir les en-têtes à écouter (nécessaire pour récupérer les cookies)
    const char *headerKeys[] = {"Cookie"};
    s_server.collectHeaders(headerKeys, 1);

    // Configurer les routes du serveur Web
    s_server.on("/", handleRoot);
    s_server.on("/login", handleLogin);
    s_server.on("/save", handleSave);
    s_server.on("/data", handleData);
    s_server.on("/generate_204", handleRoot);  // Redirections portail captif Android/iOS
    s_server.on("/fwlink", handleRoot);          // Redirection Windows
    s_server.onNotFound(handleNotFound);
    s_server.begin();
    
    Serial.println("[WIFI] Serveur Web HTTP sécurisé du portail captif démarré");
}

void wifiStopAP() {
    if (!s_apActive) return;
    s_dnsServer.stop();
    s_server.close();
    WiFi.softAPdisconnect(true);
    s_apActive = false;
    Serial.println("[WIFI] AP arrêté");
}

void wifiHandleWebServer() {
    if (s_apActive) {
        s_dnsServer.processNextRequest();
        s_server.handleClient();
    }
}

// ============================================================
//  Tâche FreeRTOS – reconnexion automatique + serveur web
// ============================================================
void wifiTask(void *param) {
    Serial.println("[WIFI] Tâche démarrée");
    uint32_t lastCheck = 0;
    uint32_t lastConnectAttempt = 0;

    for (;;) {
        wifiHandleWebServer();

        uint32_t now = millis();
        if (now - lastCheck > 1000) { // Vérifier l'état toutes les secondes
            lastCheck = now;
            if (g_config.apEnabled) {
                if (s_apActive) {
                    s_wifiState = NET_CONNECTED;
                } else {
                    s_wifiState = NET_DISCONNECTED;
                }
            } else {
                if (WiFi.isConnected()) {
                    s_wifiState = NET_CONNECTED;
                } else {
                    // Si l'AP est désactivé, on tente la connexion toutes les 25 secondes
                    if (lastConnectAttempt == 0 || (now - lastConnectAttempt > 25000)) {
                        lastConnectAttempt = now;
                        s_wifiState = NET_CONNECTING;
                        connectWifi();
                    } else {
                        s_wifiState = NET_CONNECTING;
                    }
                }
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(15));
    }
}

void startWifiTask() {
    xTaskCreatePinnedToCore(
        wifiTask,
        "WiFiTask",
        CFG_STACK_WIFI,
        nullptr,
        CFG_PRIO_WIFI,
        nullptr,
        0
    );
}
