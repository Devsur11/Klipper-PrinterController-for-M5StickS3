#include "tasks.h"
#include "api_client.h"
#include "network.h"
#include "config.h"
#include <M5Unified.h>

extern APIClient apiClient;
extern NetworkManager networkManager;

TaskHandle_t TaskManager::apiTaskHandle = nullptr;
TaskHandle_t TaskManager::networkTaskHandle = nullptr;
TaskHandle_t TaskManager::wifiScanTaskHandle = nullptr;
TaskHandle_t TaskManager::screenDimTaskHandle = nullptr;
unsigned long TaskManager::lastActivityTime = 0;

void TaskManager::apiUpdateTask(void* pvParameters) {
    while (true) {
        // Update API every 2 seconds
        apiClient.update();
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void TaskManager::networkUpdateTask(void* pvParameters) {
    while (true) {
        // Update network connection status every 1 second
        networkManager.update();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void TaskManager::wifiScanTask(void* pvParameters) {
    while (true) {
        // WiFi scanning is handled on-demand, just keep task alive
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void TaskManager::screenDimTask(void* pvParameters) {
    static int currentBrightness = 255;  // Start at full brightness
    const int MIN_BRIGHTNESS = 0;
    const int MAX_BRIGHTNESS = 255;
    const int DIM_STEP = 5;
    const int DIM_CHECK_INTERVAL = 100;  // Check every 1 second
    
    while (true) {
        if (Config::screenDimEnabled) {
            unsigned long now = millis();
            unsigned long timeSinceActivity = (lastActivityTime > 0) ? (now - lastActivityTime) : 0;
            unsigned long dimTimeoutMs = Config::screenDimTimeout * 1000;
            
            if (timeSinceActivity > dimTimeoutMs && lastActivityTime > 0) {
                // Time to dim: gradually decrease brightness
                if (currentBrightness > MIN_BRIGHTNESS) {
                    currentBrightness = currentBrightness - DIM_STEP;
                    if (currentBrightness < MIN_BRIGHTNESS) {
                        currentBrightness = MIN_BRIGHTNESS;
                    }
                    M5.Display.setBrightness(currentBrightness);
                }
            } else if (timeSinceActivity <= dimTimeoutMs) {
                // Activity detected within timeout: restore brightness
                if (currentBrightness < MAX_BRIGHTNESS) {
                    currentBrightness = MAX_BRIGHTNESS;
                    M5.Display.setBrightness(currentBrightness);
                }
            }
        }
        
        vTaskDelay(DIM_CHECK_INTERVAL / portTICK_PERIOD_MS);
    }
}

void TaskManager::recordActivity() {
    lastActivityTime = millis();
}

void TaskManager::initTasks() {
    // Initialize activity time on startup
    lastActivityTime = millis();
    
    // Create API update task on core 1
    xTaskCreatePinnedToCore(
        apiUpdateTask,      // Task function
        "APIUpdate",        // Task name
        4096,              // Stack size (4KB)
        nullptr,           // Parameters
        1,                 // Priority
        &apiTaskHandle,    // Task handle
        1                  // Core 1 (leave core 0 for UI)
    );
    
    // Create network update task on core 1
    xTaskCreatePinnedToCore(
        networkUpdateTask,
        "NetworkUpdate",
        2048,
        nullptr,
        1,
        &networkTaskHandle,
        1
    );
    
    // Create WiFi scan task on core 1
    xTaskCreatePinnedToCore(
        wifiScanTask,
        "WiFiScan",
        2048,
        nullptr,
        2,  // Higher priority for scanning
        &wifiScanTaskHandle,
        1
    );
    
    // Create screen dim task on core 1
    xTaskCreatePinnedToCore(
        screenDimTask,
        "ScreenDim",
        2048,
        nullptr,
        1,
        &screenDimTaskHandle,
        1
    );
}

void TaskManager::stopTasks() {
    if (apiTaskHandle) {
        vTaskDelete(apiTaskHandle);
        apiTaskHandle = nullptr;
    }
    if (networkTaskHandle) {
        vTaskDelete(networkTaskHandle);
        networkTaskHandle = nullptr;
    }
    if (wifiScanTaskHandle) {
        vTaskDelete(wifiScanTaskHandle);
        wifiScanTaskHandle = nullptr;
    }
    if (screenDimTaskHandle) {
        vTaskDelete(screenDimTaskHandle);
        screenDimTaskHandle = nullptr;
    }
}
