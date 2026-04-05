#include "config.h"
#include "esp_log.h"

static const char* TAG = "CONFIG";

// Static member initialization
String Config::ssid = "";
String Config::wifiPassword = "";
String Config::klipperIP = "";
int Config::klipperPort = 7125;
String Config::machineName = "Klipper Controller";
bool Config::screenDimEnabled = true;
int Config::screenDimTimeout = 30;  // 30 seconds default
const char* Config::CONFIG_FILE = "/config.json";

bool Config::hasWiFiCredentials() {
    return ssid.length() > 0 && wifiPassword.length() > 0;
}

bool Config::hasKlipperIP() {
    return klipperIP.length() > 0;
}

void Config::saveToStorage() {
    StaticJsonDocument<512> doc;
    doc["ssid"] = ssid;
    doc["wifiPassword"] = wifiPassword;
    doc["klipperIP"] = klipperIP;
    doc["klipperPort"] = klipperPort;
    doc["machineName"] = machineName;
    doc["screenDimEnabled"] = screenDimEnabled;
    doc["screenDimTimeout"] = screenDimTimeout;
    
    File file = SPIFFS.open(CONFIG_FILE, FILE_WRITE);
    if (!file) {
        ESP_LOGE(TAG, "Failed to open config file for writing");
        return;
    }
    
    serializeJson(doc, file);
    file.close();
    ESP_LOGI(TAG, "Config saved successfully");
}

void Config::loadFromStorage() {
    if (!SPIFFS.exists(CONFIG_FILE)) {
        ESP_LOGI(TAG, "Config file does not exist");
        return;
    }
    
    File file = SPIFFS.open(CONFIG_FILE, FILE_READ);
    if (!file) {
        ESP_LOGE(TAG, "Failed to open config file for reading");
        return;
    }
    
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        ESP_LOGE(TAG, "Failed to parse config: %s", error.c_str());
        return;
    }
    
    ssid = doc["ssid"].as<String>();
    wifiPassword = doc["wifiPassword"].as<String>();
    klipperIP = doc["klipperIP"].as<String>();
    klipperPort = doc["klipperPort"].as<int>();
    machineName = doc["machineName"].as<String>();
    screenDimEnabled = doc["screenDimEnabled"].as<bool>();
    screenDimTimeout = doc["screenDimTimeout"].as<int>();
    
    ESP_LOGI(TAG, "Config loaded successfully");
}

void Config::clearWiFiData() {
    ssid = "";
    wifiPassword = "";
    saveToStorage();
}

void Config::clearKlipperData() {
    klipperIP = "";
    klipperPort = 7125;
    saveToStorage();
}

void Config::reset() {
    ssid = "";
    wifiPassword = "";
    klipperIP = "";
    klipperPort = 7125;
    machineName = "Klipper Controller";
    screenDimEnabled = true;
    screenDimTimeout = 30;
    
    if (SPIFFS.exists(CONFIG_FILE)) {
        SPIFFS.remove(CONFIG_FILE);
    }
    ESP_LOGI(TAG, "Config reset to defaults");
}
