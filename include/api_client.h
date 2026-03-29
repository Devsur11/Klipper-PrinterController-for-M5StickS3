#ifndef API_CLIENT_H
#define API_CLIENT_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <vector>

struct PrinterState {
    bool connected;
    bool printing;
    bool paused;
    float bedTemp;
    float bedTarget;
    float nozzleTemp;
    float nozzleTarget;
    float printProgress;
    unsigned long printTimeRemaining;
    unsigned long printTimeElapsed;
    unsigned long printTimeTotal;  // FIX: total estimated print time (from total_duration)
    String currentFile;
    String klippyState;  // FIX: track klippy state (ready, startup, error, shutdown)
};

struct PowerDevice {
    String name;
    bool on;
};

struct KlipperMacro {  // FIX: add macro structure
    String name;
    String description;
};

class APIClient {
public:
    APIClient();
    ~APIClient();

    bool connect(const String& host, int port);
    void disconnect();
    void update();

    // Query methods
    void queryPrinterState();
    void queryPrintStats();
    void queryPowerDevices();
    void queryFileList();
    void queryMacros();  // FIX: add macro query support
    void queryKlippyState();  // FIX: query klippy agent state

    // Control methods
    void startPrint(const String& filename);
    void pausePrint();
    void resumePrint();
    void cancelPrint();
    void emergencyStop();
    void setHeaterTarget(const String& heater, float temp);
    void moveAxis(const String& axis, float distance, float feedrate);
    void homeAxis(const String& axis);  // FIX: add homing support
    void executeMacro(const String& name);  // FIX: add macro execution
    void sendGcode(const String& gcode);
    void setPowerDevice(const String& device, bool on);
    void updatePrinter();

    // Getters
    PrinterState getPrinterState() const { return printerState; }
    bool isConnected() const { return connected; }
    bool isKlippyConnected() const { return klippyConnected; }  // FIX: check klippy host connection
    String getKlippyState() const { return printerState.klippyState; }
    const std::vector<PowerDevice>& getPowerDevices() const { return powerDevices; }
    const std::vector<String>& getFileList() const { return fileList; }
    const std::vector<KlipperMacro>& getMacros() const { return macros; }  // FIX: add macro getter

private:
    // HTTP transport helpers
    bool httpGet(const String& path, DynamicJsonDocument& result);
    bool httpPost(const String& path, const String& jsonBody, DynamicJsonDocument& result);
    bool postGcode(const String& script);

    // Legacy stubs (kept for source compatibility -- not called)
    bool sendJsonRpc(const String& method, JsonDocument& params);
    bool receiveJsonRpc(JsonDocument& response);
    int  getNextRequestId();

    String host;
    int port;
    bool connected;
    bool klippyConnected;  // FIX: track if klippy host is connected
    PrinterState printerState;
    std::vector<PowerDevice> powerDevices;
    std::vector<String> fileList;
    std::vector<KlipperMacro> macros;  // FIX: add macros vector
    unsigned long lastUpdateTime;
    unsigned long updateInterval;
    int requestId;
};

#endif