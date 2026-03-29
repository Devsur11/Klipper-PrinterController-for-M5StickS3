
#include <WiFi.h>
#include <SPIFFS.h>
#include "config.h"
#include "screen_manager.h"
#include "api_client.h"
#include "network.h"
#include "tasks.h"
#include "inputManager.h"
#include <M5GFX.h>
#include <M5Unified.h>
#include "esp_log.h"

static const char* TAG = "APP";

ScreenManager screenManager;
APIClient apiClient;
NetworkManager networkManager;

void setup() {
    Serial.begin(115200);
    delay(500);  // Wait for Serial to stabilize
    
    // Configure logging to use UART output
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
    
    ESP_LOGI(TAG, "=== Klipper Controller Starting ===");
    ESP_LOGI(TAG, "Serial initialized at 115200 baud");
    
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Power.begin();
    M5.Display.begin();
    M5.Power.setExtOutput(false);
    M5.Display.init();
    M5.Display.setRotation(8);
    M5.Display.setColorDepth(16);  // RGB565
  M5.Display.fillScreen(BLACK);
  M5.Display.setTextColor(WHITE);
  M5.Display.setTextSize(2);
  M5.Display.setCursor(10, 10);
  M5.Display.println("Initializing...");
  M5.Display.println("Storage Manager:");
  M5.Display.println("LittleFS");
  M5.Display.println("Please wait...");
  M5.Display.pushState();
    // Initialize display for M5StickS3
    M5.Display.init();
    //M5.Display.setRotation(1);  // Portrait mode (135x240)
    M5.Display.setColorDepth(16);  // RGB565
    M5.Display.startWrite();
    //M5.Display.fillScreen(Theme::BG);
    M5.Display.endWrite();
    
    // Set text rendering defaults
    M5.Display.setTextSize(1);
    M5.Display.setFont(&fonts::Font2);
    
    // Initialize SPIFFS for file storage
    if (!SPIFFS.begin(true)) {
        ESP_LOGE(TAG, "SPIFFS Mount Failed");
    }
    
    // Load configuration from storage
    Config::loadFromStorage();
    
    // Initialize network if credentials are saved
    if (Config::hasWiFiCredentials()) {
        ESP_LOGI(TAG, "WiFi credentials found, attempting connection");
        ESP_LOGI(TAG, "SSID: %s", Config::ssid.c_str());
        networkManager.connectToWiFi(Config::ssid, Config::wifiPassword);
        
        // Wait for WiFi to connect (max 10 seconds)
        int waitCount = 0;
        while (!networkManager.isConnected() && waitCount < 100) {
            delay(100);  // Use delay() here since this is setup()
            Serial.print(".");
            waitCount++;
        }
        Serial.println();
        
        if (networkManager.isConnected()) {
            ESP_LOGI(TAG, "WiFi connected! IP: %s", networkManager.getLocalIP().c_str());
        } else {
            ESP_LOGW(TAG, "WiFi connection timeout or failed after 10 seconds");
        }
    } else {
        ESP_LOGI(TAG, "No WiFi credentials saved");
    }
    
    // Start with appropriate screen
    if (!Config::hasWiFiCredentials()) {
        screenManager.switchScreen(ScreenType::WIFI_SETUP);
    } else if (!Config::hasKlipperIP()) {
        screenManager.switchScreen(ScreenType::NETWORK_SETUP);
    } else {
        // Automatically connect to saved Klipper IP only if WiFi is connected
        if (networkManager.isConnected()) {
            ESP_LOGI(TAG, "Connecting to Klipper at %s:7125", Config::klipperIP.c_str());
            if (apiClient.connect(Config::klipperIP, 7125)) {
                ESP_LOGI(TAG, "Connected to Klipper API successfully");
            } else {
                ESP_LOGE(TAG, "Failed to connect to Klipper API");
            }
        } else {
            ESP_LOGW(TAG, "WiFi not connected, skipping API connection");
        }
        screenManager.switchScreen(ScreenType::MAIN_MENU);
    }
    
    // Initialize background tasks for blocking operations
    TaskManager::initTasks();
    ESP_LOGI(TAG, "Background tasks initialized");
    
    // Initialize input manager
    inputManager::init();
    ESP_LOGI(TAG, "Input manager initialized");
    
    ESP_LOGI(TAG, "=== Setup Complete ===");
}

static unsigned long lastWifiCheck = 0;
static bool lastWifiStatus = false;

void loop() {
    // Check WiFi connection every 5 seconds and log changes
    unsigned long now = millis();
    if (now - lastWifiCheck > 5000) {
        lastWifiCheck = now;
        bool currentWifiStatus = networkManager.isConnected();
        
        if (currentWifiStatus != lastWifiStatus) {
            if (currentWifiStatus) {
                ESP_LOGI(TAG, "WiFi connected! IP: %s", networkManager.getLocalIP().c_str());
            } else {
                ESP_LOGW(TAG, "WiFi disconnected!");
            }
            lastWifiStatus = currentWifiStatus;
        }
    }
    
    // Update input manager (handles all button detection)
    inputManager::update();
    
    // Update screen rendering (UI is always on core 0)
    screenManager.update();
    
    // Handle short button presses
    if (inputManager::isButtonAPressed()) {
        screenManager.handleButtonA();
    }
    if (inputManager::isButtonBPressed()) {
        screenManager.handleButtonB();
    }
    
    // Handle long button presses
    if (inputManager::isButtonALongPressed()) {
        screenManager.handleButtonALongPress();
    }
    if (inputManager::isButtonBLongPressed()) {
        screenManager.handleButtonBLongPress();
    }
    
    // Small delay to prevent CPU spinning
    vTaskDelay(100 / portTICK_PERIOD_MS);
}
