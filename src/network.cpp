#include "network.h"
#include "esp_log.h"

static const char* TAG = "WIFI";

const unsigned long NetworkManager::SCAN_INTERVAL = 5000;

NetworkManager::NetworkManager() : scanning(false), lastScanTime(0) {
}

NetworkManager::~NetworkManager() {
    disconnect();
}

void NetworkManager::scanNetworks() {
    if (scanning) {
        return;
    }
    
    scanning = true;
    networks.clear();
    
    int n = WiFi.scanNetworks();
    
    for (int i = 0; i < n; i++) {
        WiFiNetwork net;
        net.ssid = WiFi.SSID(i);
        net.rssi = WiFi.RSSI(i);
        net.channel = WiFi.channel(i);
        
        // Avoid duplicates
        bool found = false;
        for (const auto& existing : networks) {
            if (existing.ssid == net.ssid) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            networks.push_back(net);
        }
    }
    
    WiFi.scanDelete();
    scanning = false;
    lastScanTime = millis();
    
    ESP_LOGI(TAG, "WiFi scan complete. Found %d networks", networks.size());
}

bool NetworkManager::connectToWiFi(const String& ssid, const String& password) {
    ESP_LOGI(TAG, "Attempting WiFi connection to: %s", ssid.c_str());
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    int attempts = 0;
    const int maxAttempts = 20;
    
    ESP_LOGI(TAG, "Waiting for WiFi connection...");
    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
        delay(500);  // Use delay() for blocking wait in setup context
        Serial.print(".");
        ESP_LOGD(TAG, "Attempt %d/%d, Status: %d", attempts + 1, maxAttempts, WiFi.status());
        attempts++;
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        ESP_LOGI(TAG, "WiFi connected! IP: %s, RSSI: %d dBm", 
                 WiFi.localIP().toString().c_str(), WiFi.RSSI());
        return true;
    } else {
        ESP_LOGE(TAG, "Failed to connect to WiFi. Final status: %d", WiFi.status());
        return false;
    }
}

void NetworkManager::disconnect() {
    WiFi.disconnect(true);
    ESP_LOGI(TAG, "WiFi disconnected");
}

void NetworkManager::update() {
    unsigned long now = millis();
    
    // Auto-rescan every SCAN_INTERVAL
    if (now - lastScanTime >= SCAN_INTERVAL && !scanning) {
        scanNetworks();
    }
}

String NetworkManager::getSignalStrength() const {
    if (!isConnected()) {
        return "Disconnected";
    }
    
    int32_t rssi = WiFi.RSSI();
    if (rssi > -50) return "Excellent";
    if (rssi > -60) return "Good";
    if (rssi > -70) return "Fair";
    if (rssi > -80) return "Weak";
    return "Very Weak";
}
