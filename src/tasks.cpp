#include "tasks.h"
#include "api_client.h"
#include "network.h"

extern APIClient apiClient;
extern NetworkManager networkManager;

TaskHandle_t TaskManager::apiTaskHandle = nullptr;
TaskHandle_t TaskManager::networkTaskHandle = nullptr;
TaskHandle_t TaskManager::wifiScanTaskHandle = nullptr;

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

void TaskManager::initTasks() {
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
}
