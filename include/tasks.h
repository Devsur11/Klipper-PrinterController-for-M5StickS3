#ifndef TASKS_H
#define TASKS_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class TaskManager {
public:
    static void initTasks();
    static void stopTasks();
    
private:
    static TaskHandle_t apiTaskHandle;
    static TaskHandle_t networkTaskHandle;
    static TaskHandle_t wifiScanTaskHandle;
    
    // Task functions
    static void apiUpdateTask(void* pvParameters);
    static void networkUpdateTask(void* pvParameters);
    static void wifiScanTask(void* pvParameters);
};

#endif
