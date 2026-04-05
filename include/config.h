#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

class Config {
public:
    static String ssid;
    static String wifiPassword;
    static String klipperIP;
    static int klipperPort;
    static String machineName;
    static bool screenDimEnabled;
    static int screenDimTimeout;  // in seconds
    
    static bool hasWiFiCredentials();
    static bool hasKlipperIP();
    static void saveToStorage();
    static void loadFromStorage();
    static void clearWiFiData();
    static void clearKlipperData();
    static void reset();
    
private:
    static const char* CONFIG_FILE;
};

#endif
