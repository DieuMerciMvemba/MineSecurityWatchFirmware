# Mine Security Watch Firmware

Firmware ESP32-S3 pour montre de sécurité minière avec détection de chute, GPS, LoRa, et interface LVGL.

## 🚀 Installation

### Prérequis

- **Arduino IDE** 2.x avec support ESP32
- **Board:** LilyGo T-Watch S3 (ESP32-S3)
- **Bibliothèques requises:**
  - `LilyGoLib.h` (LilyGo T-Watch library)
  - `LV_Helper.h` (LVGL helper)
  - `LVGL` (v8.x)
  - `HTTPClient` (ESP32 Arduino)
  - `WiFi` (ESP32 Arduino)
  - `RadioLib` (pour SX1262 LoRa)

### Configuration

1. **Cloner ou télécharger ce repository**
2. **Ouvrir `MineSecurityWatch.ino`** dans Arduino IDE
3. **Éditer `config.h`** avec vos paramètres:

```cpp
// WiFi
#define CFG_WIFI_SSID       "votre_ssid"
#define CFG_WIFI_PASSWORD   "votre_password"

// Serveur API
#define CFG_API_HOST        "192.168.137.1"
#define CFG_API_PORT        3000
#define CFG_API_ENDPOINT    "/api/watch/data"

// Identité mineur
#define CFG_WORKER_ID       "M001"
#define CFG_WORKER_NAME     "Votre Nom"
#define CFG_WORKER_ZONE     "Zone B"
```

4. **Activer les capteurs** dans `sensor_service.cpp`:
   - Décommenter les lignes `instance.getAccelerometer()` et `instance.vibrate()` si votre board supporte ces APIs

### Compilation et Upload

1. **Sélectionner la board:**
   - Tools → Board → ESP32 Arduino → LilyGo T-Watch S3

2. **Sélectionner le port:**
   - Tools → Port → COM8 (ou votre port)

3. **Compiler et uploader:**
   - Cliquez sur ✓ (Verify) puis → (Upload)

### Premier démarrage

Au premier démarrage, vous verrez dans le Serial Monitor (115200 baud):

```
[INIT] ✅ Mine Security Watch opérationnel !
[WIFI] Connexion à votre_ssid...
[WIFI] ✅ Connecté – IP: 192.168.137.19
[API] POST http://192.168.137.1:3000/api/watch/data
```

## 📡 API Backend

Le firmware envoie des données à votre serveur via HTTP POST. Voir [API_SPECIFICATION.md](../API_SPECIFICATION.md) pour les détails.

### Endpoint principal: POST `/api/watch/data`

```json
{
  "workerId": "M001",
  "battery": 85.5,
  "temperature": 36.2,
  "steps": 1234,
  "motion": "WALKING",
  "lat": -11.123456,
  "lng": 27.654321,
  "timestamp": "2026-01-01T12:34:56Z"
}
```

### Endpoint alertes: POST `/api/watch/sos`

```json
{
  "workerId": "M001",
  "type": "SOS",
  "battery": 85.5,
  "lat": -11.123456,
  "lng": 27.654321,
  "timestamp": "2026-01-01T12:34:56Z"
}
```

## 🏗️ Architecture

### Fichiers principaux

| Fichier | Rôle |
|---------|------|
| `MineSecurityWatch.ino` | Main + tâches FreeRTOS + SOS + navigation |
| `config.h` | Configuration centrale (WiFi, API, LoRa) |
| `data_model.h` | Structures de données partagées |
| `sensor_service.cpp` | Accéléromètre, détection chute, comptage pas |
| `battery_service.cpp` | Gestion batterie |
| `wifi_service.cpp` | Connexion WiFi |
| `api_service.cpp` | Client HTTP pour API REST |
| `lora_service.cpp` | Communication LoRa SX1262 |
| `ui_home.cpp` | Dashboard principal |
| `ui_health.cpp` | Écran santé |
| `ui_network.cpp` | État réseau |
| `ui_alert.cpp` | Alertes et écran SOS |
| `ui_gps.cpp` | Tracking GPS |
| `ui_compass.cpp` | Boussole |
| `ui_power.cpp` | Gestion énergie |
| `ui_radio.cpp` | Walkie-talkie |
| `ui_nfc.cpp` | Authentification NFC |

### Écrans disponibles

- **Home** (0) - Dashboard avec heure, batterie, métriques, bouton SOS
- **Health** (1) - Santé avec arc de progression style Garmin
- **Network** (2) - Status WiFi/API/LoRa avec LED indicators
- **Alerts** (3) - Historique des alertes
- **SOS** (4) - Écran rouge pulsant (alerte active)
- **GPS** (5) - Tracking GPS
- **NFC** (6) - Authentification NFC
- **Compass** (7) - Boussole
- **Power** (8) - Gestion énergie
- **Radio** (9) - Walkie-talkie

## 🔧 Dépannage

### Erreur `vfs_fat_spiflash`
Cette erreur est **normale** - le filesystem SPIFFS n'est pas formaté, elle peut être ignorée.

### Erreur HTTP -1
Le serveur n'est pas joignable. Vérifiez:
- L'IP du serveur dans `config.h`
- Le firewall autorise le port 3000
- L'ESP32 et le serveur sont sur le même réseau

### Détection chute
Le seuil de détection est configurable (défaut: 2.5g) dans `config.h`:
```cpp
#define CFG_FALL_THRESHOLD  2.5f
```

### LoRa ne fonctionne pas
Vérifiez:
- La fréquence dans `config.h` (868 MHz EU / 915 MHz US)
- L'adresse LoRa de la montre et de la station de base

## 📝 Configuration avancée

### Timings (config.h)

```cpp
#define CFG_API_INTERVAL        30000   // Envoi données toutes les 30s
#define CFG_SENSOR_INTERVAL      1000   // Lecture capteurs toutes les 1s
#define CFG_WIFI_RECONNECT       5000   // Tentative reconnexion WiFi
#define CFG_LORA_HEARTBEAT      60000   // Heartbeat LoRa toutes les 60s
#define CFG_SOS_HOLD_MS          2000   // Appui long SOS = 2s
```

### Seuils d'alerte

```cpp
#define CFG_FALL_THRESHOLD      2.5f    // g – détection chute
#define CFG_LOW_BATTERY         20      // % – alerte batterie faible
#define CFG_CRIT_BATTERY        10      // % – arrêt imminent
```

## 📄 License

Ce projet est développé pour Mine Security Watch.

## 👥 Auteurs

Développé par Mvemba Dieu Merci pour Mine Security Watch Team - 2026
