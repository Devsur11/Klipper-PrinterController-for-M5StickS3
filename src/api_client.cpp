#include "api_client.h"
#include "config.h"
#include "esp_log.h"
#include <HTTPClient.h>

static const char* TAG = "API";

APIClient::APIClient()
    : connected(false), klippyConnected(false), port(80),
      lastUpdateTime(0), updateInterval(2000), requestId(0) {
    printerState = {false, false, false, 0, 0, 0, 0, 0, 0, 0, 0, "", "offline"};
}

APIClient::~APIClient() {
    disconnect();
}

bool APIClient::connect(const String& h, int p) {
    host = h;
    port = p;

    String url = "http://" + host + ":" + String(port) + "/server/info";
    HTTPClient http;
    http.begin(url);
    http.setTimeout(3000);
    int code = http.GET();
    http.end();

    if (code == 200) {
        connected = true;
        ESP_LOGI(TAG, "Connected to Moonraker at %s:%d", host.c_str(), port);
        return true;
    }
    ESP_LOGE(TAG, "Failed to reach Moonraker (HTTP %d)", code);
    connected = false;
    return false;
}

void APIClient::disconnect() {
    connected = false;
    host = "";
    port = 0;
}

void APIClient::update() {
    if (!connected) return;
    unsigned long now = millis();
    if (now - lastUpdateTime >= updateInterval) {
        lastUpdateTime = now;
        queryKlippyState();
        queryPrinterState();
        queryPowerDevices();
    }
}

// ---------------------------------------------------------------------------
// HTTP transport helpers
// ---------------------------------------------------------------------------

bool APIClient::httpGet(const String& path, DynamicJsonDocument& result) {
    LoadingIndicator::getInstance().setLoading(true, "Loading...");
    
    String url = "http://" + host + ":" + String(port) + path;
    HTTPClient http;
    http.begin(url);
    http.setTimeout(3000);

    int code = http.GET();
    if (code != 200) {
        ESP_LOGW(TAG, "GET %s -> %d", path.c_str(), code);
        http.end();
        if (code < 0) connected = false;
        LoadingIndicator::getInstance().clear();
        return false;
    }

    String body = http.getString();
    http.end();

    DeserializationError err = deserializeJson(result, body);
    if (err) {
        ESP_LOGW(TAG, "JSON parse error (%s): %s", path.c_str(), err.c_str());
        LoadingIndicator::getInstance().clear();
        return false;
    }
    LoadingIndicator::getInstance().clear();
    return true;
}

bool APIClient::httpPost(const String& path, const String& jsonBody, DynamicJsonDocument& result) {
    LoadingIndicator::getInstance().setLoading(true, "Loading...");
    
    String url = "http://" + host + ":" + String(port) + path;
    HTTPClient http;
    http.begin(url);
    http.setTimeout(3000);
    http.addHeader("Content-Type", "application/json");

    int code = http.POST(jsonBody);
    if (code != 200) {
        ESP_LOGW(TAG, "POST %s -> %d", path.c_str(), code);
        http.end();
        if (code < 0) connected = false;
        LoadingIndicator::getInstance().clear();
        return false;
    }

    String body = http.getString();
    http.end();

    DeserializationError err = deserializeJson(result, body);
    if (err) {
        ESP_LOGW(TAG, "JSON parse error (%s): %s", path.c_str(), err.c_str());
        LoadingIndicator::getInstance().clear();
        return false;
    }
    LoadingIndicator::getInstance().clear();
    return true;
}

bool APIClient::postGcode(const String& script) {
    DynamicJsonDocument reqDoc(1024);
    reqDoc["script"] = script;
    String body;
    serializeJson(reqDoc, body);

    DynamicJsonDocument resp(512);
    if (!httpPost("/printer/gcode/script", body, resp)) {
        ESP_LOGW(TAG, "GCode failed: %s", script.c_str());
        return false;
    }
    ESP_LOGI(TAG, "GCode sent: %s", script.c_str());
    return true;
}

// ---------------------------------------------------------------------------
// Query methods
// ---------------------------------------------------------------------------

void APIClient::queryPrinterState() {
    // BUG FIX 1 & 2: print_stats has NO "progress" field and NO estimated
    // total time field. Progress lives in virtual_sdcard.progress (0.0-1.0).
    // Time remaining must be calculated: (print_duration / progress) - print_duration.
    // Both virtual_sdcard and print_stats must be queried together.
    //
    // BUG FIX 4: Request doc bumped 256->512 to fit the added virtual_sdcard key.
    // BUG FIX 5: Response doc bumped 2048->4096 for the extra object payload.
    DynamicJsonDocument reqDoc(512);
    JsonObject objects = reqDoc.createNestedObject("objects");
    objects["print_stats"]    = nullptr;
    objects["virtual_sdcard"] = nullptr;  // FIX: this is where progress lives
    objects["extruder"]       = nullptr;
    objects["heater_bed"]     = nullptr;
    String reqBody;
    serializeJson(reqDoc, reqBody);

    DynamicJsonDocument doc(4096);
    if (!httpPost("/printer/objects/query", reqBody, doc)) {
        ESP_LOGW(TAG, "Failed to query printer state");
        return;
    }

    JsonVariant status = doc["result"]["status"];
    if (status.isNull()) {
        ESP_LOGW(TAG, "Unexpected shape in printer state response");
        return;
    }

    // --- print_stats: state, elapsed time, filename ---
    JsonVariant ps = status["print_stats"];
    if (!ps.isNull()) {
        const char* state = ps["state"] | "standby";
        printerState.printing         = (strcmp(state, "printing") == 0);
        printerState.paused           = (strcmp(state, "paused")   == 0);
        printerState.printTimeElapsed = (unsigned long)(ps["print_duration"] | 0.0f);
        printerState.currentFile      = ps["filename"] | "";
    }

    // --- virtual_sdcard: progress (0.0 - 1.0) ---
    // BUG FIX 1: progress must be read from virtual_sdcard, not print_stats.
    JsonVariant vsd = status["virtual_sdcard"];
    if (!vsd.isNull()) {
        printerState.printProgress = vsd["progress"] | 0.0f;
    }

    // --- Derive remaining / total from elapsed + progress ---
    // BUG FIX 2: Use the standard formula instead of the nonexistent
    // info.total_duration field.  Guard against division by zero (progress==0
    // before the print really starts, or when the print has just completed).
    float progress = printerState.printProgress;
    unsigned long elapsed = printerState.printTimeElapsed;
    if (progress > 0.001f) {
        unsigned long totalEstimate = (unsigned long)((float)elapsed / progress);
        printerState.printTimeTotal     = totalEstimate;
        printerState.printTimeRemaining =
            (totalEstimate > elapsed) ? (totalEstimate - elapsed) : 0;
    } else {
        printerState.printTimeTotal     = 0;
        printerState.printTimeRemaining = 0;
    }

    // --- extruder ---
    JsonVariant ext = status["extruder"];
    if (!ext.isNull()) {
        printerState.nozzleTemp   = ext["temperature"] | 0.0f;
        printerState.nozzleTarget = ext["target"]      | 0.0f;
    }

    // --- heater_bed ---
    JsonVariant bed = status["heater_bed"];
    if (!bed.isNull()) {
        printerState.bedTemp   = bed["temperature"] | 0.0f;
        printerState.bedTarget = bed["target"]      | 0.0f;
    }

    printerState.connected = true;
}

void APIClient::queryKlippyState() {
    DynamicJsonDocument doc(512);
    if (!httpGet("/printer/info", doc)) {
        klippyConnected = false;
        printerState.klippyState = "offline";
        ESP_LOGW(TAG, "Klippy host not connected (offline)");
        return;
    }

    JsonVariant result = doc["result"];
    if (!result.isNull()) {
        JsonVariant stateObj = result["state"];
        if (!stateObj.isNull()) {
            const char* state = stateObj.as<const char*>();
            printerState.klippyState = String(state);
            klippyConnected = true;
            ESP_LOGI(TAG, "Klippy state: %s", state);
        } else {
            printerState.klippyState = "unknown";
            klippyConnected = false;
            ESP_LOGW(TAG, "Klippy state not found in response");
        }
    }
}

void APIClient::queryPrintStats() {
    // BUG FIX 6: Same progress fix as queryPrinterState — must include
    // virtual_sdcard and apply the same derived-time formula.
    DynamicJsonDocument reqDoc(256);
    JsonObject objects = reqDoc.createNestedObject("objects");
    objects["print_stats"]    = nullptr;
    objects["virtual_sdcard"] = nullptr;  // FIX: needed for progress
    String reqBody;
    serializeJson(reqDoc, reqBody);

    DynamicJsonDocument doc(2048);
    if (!httpPost("/printer/objects/query", reqBody, doc)) {
        ESP_LOGW(TAG, "Failed to query print stats");
        return;
    }

    JsonVariant status = doc["result"]["status"];
    if (status.isNull()) return;

    JsonVariant ps = status["print_stats"];
    if (!ps.isNull()) {
        const char* state = ps["state"] | "standby";
        printerState.printing         = (strcmp(state, "printing") == 0);
        printerState.paused           = (strcmp(state, "paused")   == 0);
        printerState.printTimeElapsed = (unsigned long)(ps["print_duration"] | 0.0f);
        printerState.currentFile      = ps["filename"] | "";
    }

    JsonVariant vsd = status["virtual_sdcard"];
    if (!vsd.isNull()) {
        printerState.printProgress = vsd["progress"] | 0.0f;
    }

    float progress = printerState.printProgress;
    unsigned long elapsed = printerState.printTimeElapsed;
    if (progress > 0.001f) {
        unsigned long totalEstimate = (unsigned long)((float)elapsed / progress);
        printerState.printTimeTotal     = totalEstimate;
        printerState.printTimeRemaining =
            (totalEstimate > elapsed) ? (totalEstimate - elapsed) : 0;
    } else {
        printerState.printTimeTotal     = 0;
        printerState.printTimeRemaining = 0;
    }
}

void APIClient::queryPowerDevices() {
    DynamicJsonDocument doc(4096);
    if (!httpGet("/machine/device_power/devices", doc)) {
        ESP_LOGW(TAG, "Failed to query power devices");
        return;
    }

    JsonArray devices = doc["result"]["devices"];
    if (devices.isNull()) return;

    powerDevices.clear();
    for (JsonObject d : devices) {
        PowerDevice dev;
        dev.name = d["device"].as<String>();
        dev.on   = (strcmp(d["status"] | "off", "on") == 0);
        powerDevices.push_back(dev);
    }
}

void APIClient::queryFileList() {
    DynamicJsonDocument doc(16384);
    if (!httpGet("/server/files/list?root=gcodes", doc)) {
        ESP_LOGW(TAG, "Failed to query file list");
        return;
    }

    JsonArray files = doc["result"];
    if (files.isNull()) return;

    fileList.clear();
    for (JsonObject f : files) {
        fileList.push_back(f["path"].as<String>());
    }
    ESP_LOGI(TAG, "Got %d file(s)", (int)fileList.size());
}

void APIClient::queryMacros() {
    DynamicJsonDocument reqDoc(128);
    JsonObject objects = reqDoc.createNestedObject("objects");
    objects["gcode_macro"] = nullptr;
    String reqBody;
    serializeJson(reqDoc, reqBody);

    DynamicJsonDocument doc(2048);
    if (!httpPost("/printer/objects/query", reqBody, doc)) {
        ESP_LOGW(TAG, "Failed to query macros");
        return;
    }

    JsonObject status = doc["result"]["status"];
    if (status.isNull()) return;

    macros.clear();
    for (JsonPair p : status) {
        String key = p.key().c_str();
        if (key.startsWith("gcode_macro ")) {
            String macroName = key.substring(12);
            KlipperMacro m;
            m.name        = macroName;
            m.description = "(Custom Macro)";
            macros.push_back(m);
        }
    }
    ESP_LOGI(TAG, "Found %d macro(s)", (int)macros.size());
}

// ---------------------------------------------------------------------------
// Print control
// ---------------------------------------------------------------------------

void APIClient::startPrint(const String& filename) {
    // BUG FIX 8: URL-encode the filename so that spaces, Polish diacritics,
    // and other special characters don't break the query string.
    String encoded = "";
    for (int i = 0; i < (int)filename.length(); i++) {
        char c = filename[i];
        if (isAlphaNumeric(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        } else {
            char buf[4];
            snprintf(buf, sizeof(buf), "%%%02X", (unsigned char)c);
            encoded += buf;
        }
    }

    DynamicJsonDocument doc(256);
    String path = "/printer/print/start?filename=" + encoded;
    if (!httpPost(path, "", doc)) {
        ESP_LOGW(TAG, "Failed to start print");
        return;
    }
    ESP_LOGI(TAG, "Print started: %s", filename.c_str());
}

void APIClient::pausePrint() {
    DynamicJsonDocument doc(256);
    if (!httpPost("/printer/print/pause", "", doc))
        ESP_LOGW(TAG, "Failed to pause print");
    else
        ESP_LOGI(TAG, "Print paused");
}

void APIClient::resumePrint() {
    DynamicJsonDocument doc(256);
    if (!httpPost("/printer/print/resume", "", doc))
        ESP_LOGW(TAG, "Failed to resume print");
    else
        ESP_LOGI(TAG, "Print resumed");
}

void APIClient::cancelPrint() {
    DynamicJsonDocument doc(256);
    if (!httpPost("/printer/print/cancel", "", doc))
        ESP_LOGW(TAG, "Failed to cancel print");
    else
        ESP_LOGI(TAG, "Print cancelled");
}

void APIClient::emergencyStop() {
    DynamicJsonDocument doc(256);
    if (!httpPost("/printer/emergency_stop", "", doc))
        ESP_LOGW(TAG, "Failed to emergency stop");
    else
        ESP_LOGI(TAG, "Emergency stop activated");
}

// ---------------------------------------------------------------------------
// Heater / motion / power
// ---------------------------------------------------------------------------

void APIClient::setHeaterTarget(const String& heater, float temp) {
    String gcode = "SET_HEATER_TEMPERATURE HEATER=" + heater
                   + " TARGET=" + String(temp, 1);
    postGcode(gcode);
    ESP_LOGI(TAG, "Set %s to %.1f C", heater.c_str(), temp);
}

void APIClient::moveAxis(const String& axis, float distance, float feedrate) {
    String gcode = "G91\nG1 " + axis + String(distance)
                   + " F" + String(feedrate) + "\nG90";
    postGcode(gcode);
}

void APIClient::homeAxis(const String& axis) {
    // BUG FIX 7: Sending "G28 " (with trailing space and no axis) is
    // harmless on most firmware but explicitly wrong. If axis is empty,
    // send plain "G28" to home all axes.
    String gcode = axis.length() > 0 ? "G28 " + axis : "G28";
    postGcode(gcode);
    ESP_LOGI(TAG, "Homing: %s", gcode.c_str());
}

void APIClient::executeMacro(const String& name) {
    postGcode(name);
    ESP_LOGI(TAG, "Executed macro: %s", name.c_str());
}

void APIClient::sendGcode(const String& gcode) {
    postGcode(gcode);
}

void APIClient::setPowerDevice(const String& device, bool on) {
    String body = "{\"device\": \"" + device + "\", \"action\": \""
                  + String(on ? "on" : "off") + "\"}";
    DynamicJsonDocument doc(256);
    if (!httpPost("/machine/device_power/device", body, doc))
        ESP_LOGW(TAG, "Failed to set power device %s", device.c_str());
    else {
        ESP_LOGI(TAG, "Power device %s -> %s", device.c_str(), on ? "ON" : "OFF");
        queryPowerDevices();
    }
}

void APIClient::updatePrinter() {
    DynamicJsonDocument doc(512);
    if (!httpPost("/machine/update/refresh", "", doc))
        ESP_LOGW(TAG, "Failed to trigger update refresh");
    else
        ESP_LOGI(TAG, "Update refresh initiated");
}

// ---------------------------------------------------------------------------
// Legacy stubs
// ---------------------------------------------------------------------------

bool APIClient::sendJsonRpc(const String& method, JsonDocument& params) {
    (void)method; (void)params;
    ESP_LOGW(TAG, "sendJsonRpc() is not used with the HTTP transport");
    return false;
}

bool APIClient::receiveJsonRpc(JsonDocument& response) {
    (void)response;
    return false;
}

int APIClient::getNextRequestId() {
    return requestId++;
}