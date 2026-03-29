#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>
#include <WiFi.h>
#include <vector>

struct WiFiNetwork {
    String ssid;
    int32_t rssi;
    uint8_t channel;
};

class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();
    
    void scanNetworks();
    bool connectToWiFi(const String& ssid, const String& password);
    void disconnect();
    void update();
    
    bool isConnected() const { return WiFi.status() == WL_CONNECTED; }
    String getLocalIP() const { return WiFi.localIP().toString(); }
    String getMACAddress() const { return WiFi.macAddress(); }
    String getSignalStrength() const;
    
    const std::vector<WiFiNetwork>& getNetworks() const { return networks; }
    bool isScanning() const { return scanning; }
    
private:
    std::vector<WiFiNetwork> networks;
    bool scanning;
    unsigned long lastScanTime;
    
    static const unsigned long SCAN_INTERVAL;
};

#endif
