---
name: klipper-controller-workspace
description: "Workspace instructions for the Klipper Controller M5StickS3 project. Embedded IoT application using PlatformIO, Arduino, and FreeRTOS."
---

# Klipper Controller Workspace Instructions

**Project**: M5StickS3 Klipper Printer Controller  
**Tech Stack**: C++ (Arduino), PlatformIO, FreeRTOS, Moonraker JSON-RPC API  
**Device**: ESP32-S3 (8MB Flash, 8MB PSRAM, 1.14" Display 135×240)

See [DEVELOPMENT.md](../DEVELOPMENT.md) for setup, [README.md](../README.md) for features, [API_REFERENCE.md](../API_REFERENCE.md) for API docs.

---

## Architecture Overview

The project follows a **modular singleton pattern** with clear separation of concerns:

```
main.cpp (entry point)
├─ ScreenManager (UI state, screen factory, navigation)
├─ APIClient (Moonraker JSON-RPC to port 7125)
├─ NetworkManager (WiFi scanning, connection state)
├─ Config (SPIFFS JSON persistence: WiFi, IP, settings)
├─ TaskManager (FreeRTOS background tasks on core 1)
└─ InputManager (button state, press detection)

UI Loop (core 0): 30ms cycle
├─ Update WiFi/API status
├─ Process button input
├─ Call screen.update() & screen.draw()

Background Tasks (core 1):
├─ apiUpdateTask: Query Moonraker every 2s
├─ networkUpdateTask: Check WiFi every 1s
└─ wifiScanTask: Scan networks every 5s
```

**Key Interaction Flow**:
1. Device boots → Load config from SPIFFS
2. If no WiFi credentials: Show WiFi Setup screen
3. Connect to WiFi → Connect to Moonraker API
4. Show Main Menu → User navigates 11 screens
5. Background tasks keep API/WiFi state fresh

---

## Build & Run Commands

```bash
# Build for M5StickS3
platformio run -e m5sticks3

# Upload firmware
platformio run -e m5sticks3 -t upload

# Monitor serial output (115200 baud)
platformio device monitor
platformio device monitor | grep "ERROR\|WARNING"

# Clean & rebuild
platformio run -e m5sticks3 --clean-first
```

Use **VS Code tasks** (`.vscode/tasks.json`): `Ctrl+Shift+B` → select "Build" or "Upload".

---

## Code Organization & Conventions

### File Structure
- `src/ main.cpp` — Entry point, global instances, event loop
- `src/ config.cpp` — SPIFFS JSON storage/retrieval
- `src/ api_client.cpp` — Moonraker TCP/HTTP requests, JSON parsing
- `src/ network.cpp` — WiFi scanning, connection management
- `src/ screen_manager.cpp` — Screen factory, navigation, state
- `src/ screens.cpp` — All 11 screen implementations (polymorphic Screen base)
- `src/ tasks.cpp` — FreeRTOS task definitions
- `src/ imu.cpp, inputManager.cpp` — Sensor & input utilities
- `src/ display.cpp, ui.cpp` — UI components & drawing
- `include/ *.h` — Headers with include guards, forward declarations

### Naming Conventions
- **Classes**: `PascalCase` — `ScreenManager`, `APIClient`, `WiFiSetupScreen`
- **Functions**: `camelCase` — `queryPrinterState()`, `switchScreen()`
- **Constants/Enums**: `PascalCase` — `Theme::PRIMARY`, `ScreenType::MAIN_MENU`
- **Namespaces**: `lowercase` — `inputManager`, `Theme`, `Display`
- **Members**: No prefix — use `private:` / `public:` access levels

### Design Patterns Used

**1. Polymorphic Screen System**
```cpp
class Screen {
    virtual void init() = 0;        // Setup (called once)
    virtual void draw() = 0;        // Render (called each cycle)
    virtual void update() = 0;      // Logic (called each cycle)
    virtual void handleButtonA() = 0; // Input handling
    virtual void handleButtonB() = 0;
};
```
All screens inherit from `Screen`. `ScreenManager` creates & transitions via `switchScreen(ScreenType)`.

**2. Singleton/Global Instances**
```cpp
// In main.cpp
extern ScreenManager screenManager;
extern APIClient apiClient;
extern NetworkManager networkManager;
extern Config config;

// Usage across files
screenManager.switchScreen(ScreenType::MAIN_MENU);
```

**3. State Machines**
Each screen manages internal state (e.g., WiFiSetupScreen: `SCANNING` → `SELECT` → `ENTER_PASSWORD` → `CONNECTING`). State enum stored as member variable.

**4. FreeRTOS Task Coordination**
```cpp
// tasks.cpp defines background tasks
// main.cpp creates them: xTaskCreatePinnedToCore(apiUpdateTask, ..., 1, xPortGetCoreID() == 0 ? 1 : 0)
// Tasks run continuously on core 1, main UI loop on core 0
```

---

## Common Development Practices

### Logging
Use `esp_log.h` with consistent TAG context:
```cpp
const char* TAG = "SCREENS";
ESP_LOGI(TAG, "Switching to menu screen");
ESP_LOGW(TAG, "WiFi disconnected");
ESP_LOGE(TAG, "Failed to call API: %d", error_code);
```

### Error Handling
- Check HTTP response codes: `if (statusCode != 200) { ESP_LOGE(...); return false; }`
- Graceful degradation: Missing IMU doesn't crash, just disables gyro features
- Gate access: Only show MAIN_MENU if WiFi + API connected (redirect to NETWORK_SETUP otherwise)

### Timing & Intervals
- API update: 2 seconds
- WiFi check: 1 second
- WiFi scan: 5 seconds
- Long-press detection: 700ms threshold
- UI cycle: ~30ms per frame

### Memory Considerations
- Stack sizes tuned per task (4KB for API, 2KB for network)
- Dynamic allocation in screen factories: `new SplashScreen()`
- Cleanup in destructors and screen transitions

### Display & Color
```cpp
// Colors are RGB565 (Theme namespace)
#define THEME_COLOR_PRIMARY   0x07E0   // Green
#define THEME_COLOR_WARNING   0xF800   // Red
M5.Display.fillRect(x, y, w, h, Theme::PRIMARY);
```

---

## Key Files & Common Edits

| File | Purpose | Common Changes |
|------|---------|-----------------|
| [src/screens.cpp](../src/screens.cpp) | All 11 screen implementations | Add UI, fix navigation, adjust layouts |
| [src/api_client.cpp](../src/api_client.cpp) | Moonraker API calls | Add new endpoints, fix JSON parsing |
| [include/screens.h](../include/screens.h) | Screen definitions & enums | Add new screen types |
| [src/config.cpp](../src/config.cpp) | Persistent storage | Add new config fields |
| [platformio.ini](../platformio.ini) | Build config | Update dependencies, add flags |

---

## Adding a New Screen

1. **Define** in `include/screens.h`:
   ```cpp
   class MyNewScreen : public Screen {
   public:
       void init() override;
       void draw() override;
       void update() override;
       void handleButtonA() override;
       void handleButtonB() override;
   private:
       int state = 0; // State machine variable
   };
   ```

2. **Add enum** to `ScreenType`:
   ```cpp
   enum class ScreenType { SPLASH, WIFI_SETUP, MY_NEW_SCREEN, ... };
   ```

3. **Implement** in `src/screens.cpp` with logging, button logic, drawing

4. **Register** in `ScreenManager::createScreen()` factory method

5. **Navigate** via `screenManager.switchScreen(ScreenType::MY_NEW_SCREEN)`

---

## Troubleshooting

| Issue | Common Cause | Fix |
|-------|-------------|-----|
| Upload fails | Serial connection issue | Check USB, run `platformio device list` |
| Freezes on startup | SPIFFS corruption | Set `build_flags -= -DCORE_DEBUG_LEVEL=5` in platformio.ini, rebuild |
| WiFi won't connect | DNS/DHCP issue | Try static IP in settings, check network logs |
| API always fails with `{"code":-32000}` | Moonraker down or wrong IP | Verify IP + port 7125 reachable from device |
| Screen flickers | Too many updates per cycle | Reduce refresh rate or batch draw calls |

---

## Related Documentation

- [README.md](../README.md) — Full feature overview & hardware specs
- [QUICKSTART.md](../QUICKSTART.md) — 5-minute setup guide
- [DEVELOPMENT.md](../DEVELOPMENT.md) — Build system deep dive
- [API_REFERENCE.md](../API_REFERENCE.md) — Moonraker JSON-RPC endpoints & error codes

---

## Quick Snippets

**Query printer state**:
```cpp
bool success = apiClient.queryPrinterState();
if (success) {
    ESP_LOGI(TAG, "Nozzle: %.1f°C", apiClient.printerState.nozzleTemp);
}
```

**Switch screens**:
```cpp
screenManager.switchScreen(ScreenType::WIFI_SETUP);
```

**Save config**:
```cpp
config.klipperIP = "192.168.1.100";
config.saveToStorage();
```

**Long-press detection**:
```cpp
if (M5.BtnA.wasPressed()) { /* short press */ }
if (M5.BtnA.wasPressedFor(700)) { /* long press */ }
```
