#ifndef TASKS_H
#define TASKS_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class TaskManager {
public:
    static void initTasks();
    static void stopTasks();
    static void recordActivity();  // Called on any button press to reset screen dim timer
    
private:
    static TaskHandle_t apiTaskHandle;
    static TaskHandle_t networkTaskHandle;
    static TaskHandle_t wifiScanTaskHandle;
    static TaskHandle_t screenDimTaskHandle;
    static unsigned long lastActivityTime;
    
    // Task functions
    static void apiUpdateTask(void* pvParameters);
    static void networkUpdateTask(void* pvParameters);
    static void wifiScanTask(void* pvParameters);
    static void screenDimTask(void* pvParameters);
};

#endif
