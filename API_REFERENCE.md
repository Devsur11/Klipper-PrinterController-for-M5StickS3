# Moonraker API Reference

## Overview

The Klipper Controller communicates with Moonraker using JSON-RPC 2.0 protocol over TCP/IP on port 7125 (default).

## Connection Details

```
Host: Printer IP (e.g., 192.168.1.100)
Port: 7125 (default, configurable)
Protocol: JSON-RPC 2.0
Transport: Raw TCP Socket
```

## JSON-RPC Request Format

```json
{
  "jsonrpc": "2.0",
  "method": "method.name",
  "params": {
    "param1": "value1",
    "param2": 123
  },
  "id": 1
}
```

Every request ends with a newline character (`\n`).

## Implementation Details

### APIClient Class

Located in `src/api_client.cpp`, handles all Moonraker communication.

#### Connection Management
```cpp
bool connect(const String& host, int port);
void disconnect();
bool isConnected() const;
void update();
```

#### Query Methods
```cpp
void queryPrinterState();        // Get temperature, position, status
void queryPrintStats();          // Get current print job stats
void queryPowerDevices();        // List available power devices
void queryFileList();            // List G-code files on printer
```

#### Control Methods
```cpp
void startPrint(const String& filename);
void pausePrint();
void resumePrint();
void cancelPrint();
void emergencyStop();
void setHeaterTarget(const String& heater, float temp);
void moveAxis(const String& axis, float distance, float feedrate);
void sendGcode(const String& gcode);
void setPowerDevice(const String& device, bool on);
void updatePrinter();
```

## API Methods Reference

### Print Control

#### printer.print.start
Start a new print job.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "printer.print.start",
  "params": {
    "filename": "file.gcode"
  },
  "id": 1
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "result": "ok",
  "id": 1
}
```

#### printer.print.pause
Pause the current print.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "printer.print.pause",
  "params": {},
  "id": 2
}
```

#### printer.print.resume
Resume a paused print.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "printer.print.resume",
  "params": {},
  "id": 3
}
```

#### printer.print.cancel
Cancel the current print job.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "printer.print.cancel",
  "params": {},
  "id": 4
}
```

#### printer.emergency_stop
Emergency stop the printer (disables all heaters and motors).

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "printer.emergency_stop",
  "params": {},
  "id": 5
}
```

### Temperature Control

#### printer.heater_bed.set_temperature
Set the bed heater target temperature.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "printer.heater_bed.set_temperature",
  "params": {
    "heater": "heater_bed",
    "target": 60.0
  },
  "id": 6
}
```

#### printer.extruder.set_temperature
Set the nozzle heater target temperature.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "printer.extruder.set_temperature",
  "params": {
    "heater": "extruder",
    "target": 200.0
  },
  "id": 7
}
```

### G-code Execution

#### printer.gcode.script
Execute arbitrary G-code commands.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "printer.gcode.script",
  "params": {
    "script": "G91\nG1 X10 F300\nG90"
  },
  "id": 8
}
```

**Note:** Commands are case-insensitive. Multiple commands separated by newlines.

### Axis Movement

Use `printer.gcode.script` for axis movement:

**Move X axis 10mm at 30mm/min:**
```json
{
  "jsonrpc": "2.0",
  "method": "printer.gcode.script",
  "params": {
    "script": "G91\nG1 X10 F30\nG90"
  },
  "id": 9
}
```

**Move Y axis -5mm at 30mm/min:**
```json
{
  "jsonrpc": "2.0",
  "method": "printer.gcode.script",
  "params": {
    "script": "G91\nG1 Y-5 F30\nG90"
  },
  "id": 10
}
```

**Move Z axis 2mm at 10mm/min:**
```json
{
  "jsonrpc": "2.0",
  "method": "printer.gcode.script",
  "params": {
    "script": "G91\nG1 Z2 F10\nG90"
  },
  "id": 11
}
```

**Extrude 10mm at 20mm/min:**
```json
{
  "jsonrpc": "2.0",
  "method": "printer.gcode.script",
  "params": {
    "script": "G91\nG1 E10 F20\nG90"
  },
  "id": 12
}
```

### Power Management

#### machine.power.devices
Query available power devices.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "machine.power.devices",
  "params": {},
  "id": 13
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "result": {
    "devices": [
      {
        "device": "power_plug_1",
        "status": "on"
      },
      {
        "device": "power_plug_2",
        "status": "off"
      }
    ]
  },
  "id": 13
}
```

#### machine.power.on
Turn ON a power device.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "machine.power.on",
  "params": {
    "device": "power_plug_1"
  },
  "id": 14
}
```

#### machine.power.off
Turn OFF a power device.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "machine.power.off",
  "params": {
    "device": "power_plug_1"
  },
  "id": 15
}
```

### File Management

#### server.files.list
List G-code files available on the printer.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "server.files.list",
  "params": {
    "root": "gcodes"
  },
  "id": 16
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "result": {
    "files": [
      {
        "filename": "file1.gcode",
        "size": 1024000,
        "modified": 1234567890
      },
      {
        "filename": "file2.gcode",
        "size": 2048000,
        "modified": 1234567900
      }
    ]
  },
  "id": 16
}
```

### Printer State

#### printer.objects.query
Query printer object state (temperature, position, status).

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "printer.objects.query",
  "params": {
    "objects": {
      "heater_bed": null,
      "extruder": null,
      "print_stats": null,
      "toolhead": null
    }
  },
  "id": 17
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "result": {
    "status": {
      "heater_bed": {
        "temperature": 45.2,
        "target": 60.0,
        "power": 0.8
      },
      "extruder": {
        "temperature": 200.0,
        "target": 200.0,
        "power": 1.0
      },
      "print_stats": {
        "filename": "file.gcode",
        "state": "printing",
        "message": "",
        "info": {
          "total_duration": 3600,
          "print_duration": 1800,
          "filament_used": 10.5
        }
      },
      "toolhead": {
        "position": [100.0, 100.0, 5.0],
        "homed_axes": "xyz"
      }
    }
  },
  "id": 17
}
```

## PrinterState Structure

Used throughout the application to track printer status:

```cpp
struct PrinterState {
    bool connected;              // Connected to Moonraker
    bool printing;               // Print in progress
    bool paused;                 // Print is paused
    float bedTemp;              // Current bed temperature
    float bedTarget;            // Target bed temperature
    float nozzleTemp;           // Current nozzle temperature
    float nozzleTarget;         // Target nozzle temperature
    float printProgress;        // 0-100%
    unsigned long printTimeRemaining;  // Seconds
    unsigned long printTimeElapsed;    // Seconds
    String currentFile;         // Current print filename
};
```

## Error Handling

Errors in JSON-RPC responses follow this format:

```json
{
  "jsonrpc": "2.0",
  "error": {
    "code": -32600,
    "message": "Invalid Request"
  },
  "id": 18
}
```

Common error codes:
- `-32700`: Parse error
- `-32600`: Invalid Request
- `-32601`: Method not found
- `-32602`: Invalid params
- `-32603`: Internal error
- `-32000` to `-32099`: Server error

## Response Handling

The APIClient class uses ArduinoJson for parsing responses:

```cpp
bool receiveJsonRpc(JsonDocument& response) {
    if (!client.connected()) {
        return false;
    }
    
    String json = client.readStringUntil('\n');
    if (json.length() == 0) {
        return false;
    }
    
    DeserializationError error = deserializeJson(response, json);
    if (error) {
        log_w("JSON parse error: %s", error.c_str());
        return false;
    }
    
    return true;
}
```

## Polling Strategy

At 1000ms intervals, the application polls:
1. Printer state (temperatures, position, status)
2. Print stats (progress, time remaining)
3. Power device status

This provides real-time updates while minimizing network traffic.

## Example: Complete Print Workflow

```cpp
// 1. Query available files
apiClient.queryFileList();
delay(100);

// 2. Start print
apiClient.startPrint("benchy.gcode");

// 3. Update loop polls continuously
apiClient.update();  // Every 1000ms

// 4. Get printer state anytime
PrinterState state = apiClient.getPrinterState();

// 5. Control during print
apiClient.pausePrint();
apiClient.resumePrint();
apiClient.setHeaterTarget("extruder", 200);
apiClient.moveAxis("Z", 5, 10);

// 6. Emergency stop if needed
apiClient.emergencyStop();
```

## Performance Considerations

- **Network Latency**: 100-500ms typical on WiFi
- **Update Frequency**: 1000ms polling interval
- **Data Size**: Minimal payload (typically <1KB per request)
- **Memory**: PrinterState struct ~100 bytes
- **CPU**: Minimal background CPU usage

## Debugging

Enable debug logging in platformio.ini:
```ini
build_flags =
    -DCORE_DEBUG_LEVEL=5
```

Monitor serial output:
```bash
platformio device monitor --baud 115200
```

Log output shows:
- Connection status
- Request/response times
- API errors
- State updates

## References

- [Moonraker API Docs](https://moonraker.readthedocs.io/en/latest/web_api/)
- [Klipper G-code Commands](https://www.klipper3d.org/G-Codes.html)
- [JSON-RPC 2.0 Specification](https://www.jsonrpc.org/specification)
- [ArduinoJson Library](https://arduinojson.org/)

---

**Note**: API methods and response formats may vary based on Moonraker version. Always check your Moonraker instance for available methods.
