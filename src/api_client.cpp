#include "api_client.h"
#include "config.h"
#include "esp_log.h"
#include <HTTPClient.h>

static const char* TAG = "API";

APIClient::APIClient()
    : connected(false), klippyConnected(false), port(80), lastUpdateTime(0), updateInterval(2000), requestId(0) {
    printerState = {false, false, false, 0, 0, 0, 0, 0, 0, 0, 0, "", "offline"};
}

APIClient::~APIClient() {
    disconnect();
}

bool APIClient::connect(const String& h, int p) {
    host = h;
    port = p;

    // FIX: Use /server/info for reachability check, not /printer/info.
    // /server/info is always available even when Klippy is not ready,
    // whereas /printer/info can return errors if Klippy is still starting.
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
        queryKlippyState();  // FIX: check klippy state first
        queryPrinterState();
        queryPowerDevices();
    }
}

// ---------------------------------------------------------------------------
// HTTP transport helpers
// ---------------------------------------------------------------------------

bool APIClient::httpGet(const String& path, DynamicJsonDocument& result) {
    String url = "http://" + host + ":" + String(port) + path;
    HTTPClient http;
    http.begin(url);
    http.setTimeout(3000);

    int code = http.GET();
    if (code != 200) {
        ESP_LOGW(TAG, "GET %s -> %d", path.c_str(), code);
        http.end();
        if (code < 0) connected = false;
        return false;
    }

    String body = http.getString();
    http.end();

    DeserializationError err = deserializeJson(result, body);
    if (err) {
        ESP_LOGW(TAG, "JSON parse error (%s): %s", path.c_str(), err.c_str());
        return false;
    }
    return true;
}

bool APIClient::httpPost(const String& path, const String& jsonBody, DynamicJsonDocument& result) {
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
        return false;
    }

    String body = http.getString();
    http.end();

    DeserializationError err = deserializeJson(result, body);
    if (err) {
        ESP_LOGW(TAG, "JSON parse error (%s): %s", path.c_str(), err.c_str());
        return false;
    }
    return true;
}

bool APIClient::postGcode(const String& script) {
    // FIX: Increased document size from 256 to 1024 to safely hold longer
    // multi-line G-code scripts (e.g. the move sequence "G91\nG1...\nG90").
    DynamicJsonDocument reqDoc(1024);
    reqDoc["script"] = script;
    String body;
    serializeJson(reqDoc, body);

    // FIX: Response doc also bumped to 512; Moonraker returns error detail
    // objects that overflow a 256-byte document on failure.
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
    // FIX: The old code used GET with a bare query string like
    //   ?print_stats&extruder&heater_bed
    // which is not a valid Moonraker objects query format.
    //
    // The correct approach is POST /printer/objects/query with a JSON body
    // specifying the objects dict. Passing null for an object requests all
    // of its attributes.
    //
    // Reference: POST /printer/objects/query
    //   Body: {"objects": {"print_stats": null, "extruder": null, "heater_bed": null}}
    DynamicJsonDocument reqDoc(256);
    JsonObject objects = reqDoc.createNestedObject("objects");
    objects["print_stats"] = nullptr;
    objects["extruder"]    = nullptr;
    objects["heater_bed"]  = nullptr;
    String reqBody;
    serializeJson(reqDoc, reqBody);

    DynamicJsonDocument doc(2048);
    if (!httpPost("/printer/objects/query", reqBody, doc)) {
        ESP_LOGW(TAG, "Failed to query printer state");
        return;
    }

    JsonVariant status = doc["result"]["status"];
    if (status.isNull()) {
        ESP_LOGW(TAG, "Unexpected shape in printer state response");
        return;
    }

    JsonVariant ps = status["print_stats"];
    if (!ps.isNull()) {
        const char* state = ps["state"] | "standby";
        printerState.printing         = (strcmp(state, "printing") == 0);
        printerState.paused           = (strcmp(state, "paused")   == 0);
        printerState.printProgress    = ps["progress"]       | 0.0f;
        printerState.printTimeElapsed =
            (unsigned long)(ps["print_duration"] | 0.0);
        printerState.printTimeTotal   = 
            (unsigned long)(ps["info"]["total_duration"] | 0.0);
        // FIX: Calculate remaining time properly using total_duration
        if (printerState.printTimeTotal > printerState.printTimeElapsed) {
            printerState.printTimeRemaining = 
                printerState.printTimeTotal - printerState.printTimeElapsed;
        } else {
            printerState.printTimeRemaining = 0;
        }
        printerState.currentFile      = ps["filename"] | "";
    }

    JsonVariant ext = status["extruder"];
    if (!ext.isNull()) {
        printerState.nozzleTemp   = ext["temperature"] | 0.0f;
        printerState.nozzleTarget = ext["target"]      | 0.0f;
    }

    JsonVariant bed = status["heater_bed"];
    if (!bed.isNull()) {
        printerState.bedTemp   = bed["temperature"] | 0.0f;
        printerState.bedTarget = bed["target"]      | 0.0f;
    }

    printerState.connected = true;
}

void APIClient::queryKlippyState() {
    // FIX: Query klippy state (ready, startup, error, shutdown)
    // Use /printer/info endpoint to check state
    DynamicJsonDocument doc(512);
    if (!httpGet("/printer/info", doc)) {
        // If we get a 503 error, klippy is not connected
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
        }
    }
}

void APIClient::queryPrintStats() {
    // FIX: Same as queryPrinterState -- use POST with a JSON body.
    DynamicJsonDocument reqDoc(128);
    JsonObject objects = reqDoc.createNestedObject("objects");
    objects["print_stats"] = nullptr;
    String reqBody;
    serializeJson(reqDoc, reqBody);

    DynamicJsonDocument doc(1024);
    if (!httpPost("/printer/objects/query", reqBody, doc)) {
        ESP_LOGW(TAG, "Failed to query print stats");
        return;
    }

    JsonVariant ps = doc["result"]["status"]["print_stats"];
    if (ps.isNull()) return;

    const char* state = ps["state"] | "standby";
    printerState.printing      = (strcmp(state, "printing") == 0);
    printerState.paused        = (strcmp(state, "paused")   == 0);
    printerState.printProgress = ps["progress"] | 0.0f;
    printerState.currentFile   = ps["filename"] | "";
}

void APIClient::queryPowerDevices() {
    // FIX: Increased response doc from 2048 to 4096. Installations with many
    // power devices (smart plugs, relays, etc.) can exceed 2 KB easily.
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
    // FIX: Increased doc from 8192 to 16384. Large gcode libraries easily
    // exceed 8 KB of JSON (each entry has path, modified, size, permissions).
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
    // FIX: Query available macros from Moonraker
    // Macros are typically available via /printer/gcode/help but for execution
    // we just need to know their names from the /printer/info endpoint
    DynamicJsonDocument reqDoc(128);
    JsonObject objects = reqDoc.createNestedObject("objects");
    objects["gcode_macro"] = nullptr;  // Query all gcode_macro objects
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
    // Iterate through all objects to find gcode_macro ones
    for (JsonPair p : status) {
        String key = p.key().c_str();
        if (key.startsWith("gcode_macro ")) {
            // Extract macro name from "gcode_macro MACRO_NAME"
            String macroName = key.substring(12);  // Skip "gcode_macro "
            KlipperMacro m;
            m.name = macroName;
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
    // FIX: The previous code passed "?filename=..." inside the path string
    // to httpPost(), which then prepended the host/port and posted an empty
    // "{}" body. That works for the URL construction but the Moonraker docs
    // specify the filename must be a query parameter on the POST request.
    // Keeping it as a query param (not in the body) is the documented form.
    //
    // Reference: POST /printer/print/start?filename=<filename>
    //   No request body required.
    DynamicJsonDocument doc(256);
    String path = "/printer/print/start?filename=" + filename;
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
    // FIX: add homing support for individual axes
    // G28 is the home command; can optionally specify which axes
    String gcode = "G28 " + axis;
    postGcode(gcode);
    ESP_LOGI(TAG, "Homing axis %s", axis.c_str());
}

void APIClient::executeMacro(const String& name) {
    // FIX: Execute a custom macro by sending its name as a gcode command
    postGcode(name);
    ESP_LOGI(TAG, "Executed macro: %s", name.c_str());
}

void APIClient::sendGcode(const String& gcode) {
    postGcode(gcode);
}

void APIClient::setPowerDevice(const String& device, bool on) {
    // Correct endpoint per Moonraker docs:
    //   POST /machine/device_power/device
    //   Body: { "device": "<name>", "action": "on" | "off" | "toggle" }
    //
    // The original code had the right idea but was missing the leading slash.
    String body = "{\"device\": \"" + device + "\", \"action\": \""
                  + String(on ? "on" : "off") + "\"}";
    DynamicJsonDocument doc(256);
    if (!httpPost("/machine/device_power/device", body, doc))
        ESP_LOGW(TAG, "Failed to set power device %s", device.c_str());
    else {
        ESP_LOGI(TAG, "Power device %s -> %s", device.c_str(), on ? "ON" : "OFF");
        // FIX: immediately reparse power device status after toggle
        queryPowerDevices();
    }
}

void APIClient::updatePrinter() {
    // FIX: /machine/update/update does not exist.
    // The correct endpoint to trigger a refresh of update status is:
    //   POST /machine/update/refresh
    // If the intent is to actually apply an update for a specific component,
    // that would be POST /machine/update/update?name=<component>, but a
    // bare refresh (checking for updates) is the safer default here.
    DynamicJsonDocument doc(512);
    if (!httpPost("/machine/update/refresh", "", doc))
        ESP_LOGW(TAG, "Failed to trigger update refresh");
    else
        ESP_LOGI(TAG, "Update refresh initiated");
}

// ---------------------------------------------------------------------------
// Legacy stubs -- kept so the header compiles without changes
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