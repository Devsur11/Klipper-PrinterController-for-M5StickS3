# Development Guide

## Project Organization

```
klipper-controller/
├── platformio.ini              # PlatformIO configuration
├── src/                        # Source files
│   ├── main.cpp               # Application entry point
│   ├── config.cpp             # Configuration management
│   ├── api_client.cpp         # Moonraker API client
│   ├── network.cpp            # WiFi network management
│   ├── screen_manager.cpp     # Screen manager
│   ├── screens.cpp            # All screen implementations
│   └── display.cpp            # UI components
├── include/                    # Header files
│   ├── config.h
│   ├── api_client.h
│   ├── network.h
│   ├── screen_manager.h
│   ├── screens.h
│   └── display.h
├── lib/                        # Local libraries (if any)
├── test/                       # Unit tests (future)
├── README.md                   # Main documentation
├── QUICKSTART.md               # Quick start guide
└── API_REFERENCE.md           # API documentation
```

## Build System

### PlatformIO Commands

```bash
# Build for M5StickS3
platformio run -e m5sticks3

# Upload and monitor
platformio run -e m5sticks3 -t upload
platformio device monitor

# Clean build
platformio run -e m5sticks3 --clean-first

# List connected devices
platformio device list

# Check for project issues
platformio check
```

### Configuration (platformio.ini)

```ini
[env:m5sticks3]
platform = espressif32@6.12.0
board = esp32-s3-devkitc-1
framework = arduino
lib_ldf_mode = chain+
build_flags = 
    -DESP32S3
    -DCORE_DEBUG_LEVEL=5        # Debug logging level
lib_deps = 
    M5Unified=https://github.com/m5stack/M5Unified
    M5GFX=https://github.com/m5stack/M5GFX
    M5PM1=https://github.com/m5stack/M5PM1
    ArduinoJson=6.21.0
```

## Architecture Overview

### Application Flow

```
main.cpp
├── M5.begin()
├── Config::loadFromStorage()
├── networkManager.connectToWiFi()
├── screenManager.switchScreen()
│
└── Loop:
    ├── M5.update()           (Handle buttons)
    ├── screenManager.update() (Update current screen)
    ├── networkManager.update()(WiFi monitoring)
    └── apiClient.update()    (Poll printer state)
```

### Class Hierarchy

```
Screen (abstract base)
├── SplashScreen
├── WiFiSetupScreen
├── NetworkSetupScreen
├── MainMenuScreen
├── PrintStatusScreen
├── FileBrowserScreen
├── HeatingControlScreen
├── AxisControlScreen
├── PowerControlScreen
├── PrinterInfoScreen
└── SettingsScreen

UIComponent (abstract base)
├── Button
├── ProgressBar
├── Header
└── Footer
```

### State Management

**Global State** (from Config):
- WiFi credentials
- Printer IP and port
- Device preferences

**Session State** (APIClient):
- Printer state (temperatures, position)
- Print job status
- Power device status
- File list

**UI State** (Screen-specific):
- Selected menu item
- Input field contents
- Dialog state

## Adding Features

### Adding a New Screen

#### 1. Define Screen Class (screens.h)

```cpp
class MyNewScreen : public Screen {
public:
    void init() override;
    void draw() override;
    void update() override;
    void handleButtonA() override;
    void handleButtonB() override;
    
private:
    // Screen-specific state
    int selectedItem;
    Header header;
    Footer footer;
};
```

#### 2. Implement Screen (screens.cpp)

```cpp
void MyNewScreen::init() {
    selectedItem = 0;
    header = Header("My Screen");
    footer = Footer("Select", "Enter");
}

void MyNewScreen::draw() {
    M5.Display.fillScreen(Theme::BG);
    header.draw();
    
    int contentY = Display::HEADER_HEIGHT + 5;
    
    // Draw content
    DisplayUtils::drawText(5, contentY, "Item 1", Theme::FG);
    DisplayUtils::drawText(5, contentY + 20, "Item 2", Theme::MUTED);
    
    footer.draw();
}

void MyNewScreen::update() {
    // Update state
}

void MyNewScreen::handleButtonA() {
    // Handle button A press
    selectedItem = (selectedItem + 1) % 2;
}

void MyNewScreen::handleButtonB() {
    // Handle button B press
    screenManager.switchScreen(ScreenType::MAIN_MENU);
}
```

#### 3. Register Screen

In `screen_manager.h`, add to enum:
```cpp
enum class ScreenType {
    // ... existing types
    MY_NEW_SCREEN,
};
```

In `screen_manager.cpp`, add to createScreen():
```cpp
case ScreenType::MY_NEW_SCREEN:
    return new MyNewScreen();
```

#### 4. Add Navigation

From existing screen:
```cpp
void SomeScreen::handleButtonB() {
    screenManager.switchScreen(ScreenType::MY_NEW_SCREEN);
}
```

### Adding a New API Method

#### 1. Define Method Header (api_client.h)

```cpp
class APIClient {
public:
    void doSomethingNew();
    
private:
    // Private helper if needed
};
```

#### 2. Implement Method (api_client.cpp)

```cpp
void APIClient::doSomethingNew() {
    if (!connected) {
        log_w("Not connected to Moonraker");
        return;
    }
    
    // Create parameters
    StaticJsonDocument<256> params;
    params["param1"] = "value1";
    params["param2"] = 123;
    
    // Send request
    if (!sendJsonRpc("moonraker.method.name", params)) {
        log_w("Failed to call API");
        return;
    }
    
    log_i("API call successful");
}
```

#### 3. Parse Response (if needed)

```cpp
void APIClient::doSomethingNew() {
    // ... send request ...
    
    StaticJsonDocument<512> response;
    if (!receiveJsonRpc(response)) {
        log_w("Failed to receive response");
        return;
    }
    
    // Parse response
    if (response.containsKey("result")) {
        String result = response["result"].as<String>();
        // Process result
    }
}
```

### Adding UI Components

#### 1. Create Component Class (display.h)

```cpp
class MyComponent : public UIComponent {
public:
    MyComponent(int x, int y);
    void draw() override;
    bool handlePress(int x, int y) override;
    
private:
    int x, y;
    bool state;
};
```

#### 2. Implement Component (display.cpp)

```cpp
MyComponent::MyComponent(int x, int y) : x(x), y(y), state(false) {}

void MyComponent::draw() {
    uint16_t color = state ? Theme::PRIMARY : Theme::MUTED;
    M5.Display.fillRect(x, y, 50, 30, color);
    // Draw content
}

bool MyComponent::handlePress(int px, int py) {
    if (px >= x && px < x + 50 && py >= y && py < y + 30) {
        state = !state;
        return true;
    }
    return false;
}
```

#### 3. Use in Screen

```cpp
void MyScreen::init() {
    myComponent = new MyComponent(10, 50);
}

void MyScreen::draw() {
    myComponent->draw();
}

void MyScreen::handleButtonB() {
    myComponent->handlePress(50, 50);
}
```

## Configuration Management

### Adding Configuration Options

#### 1. Add Variable to Config Class (config.h)

```cpp
class Config {
public:
    static String myOption;
    static int myNumber;
    // ... existing code
};
```

#### 2. Initialize in Source (config.cpp)

```cpp
String Config::myOption = "default_value";
int Config::myNumber = 100;
```

#### 3. Update Save/Load Methods (config.cpp)

```cpp
void Config::saveToStorage() {
    StaticJsonDocument<512> doc;
    doc["myOption"] = myOption;
    doc["myNumber"] = myNumber;
    // ... existing code
    serializeJson(doc, file);
}

void Config::loadFromStorage() {
    // ... existing code
    myOption = doc["myOption"].as<String>();
    myNumber = doc["myNumber"].as<int>();
}
```

## Testing & Debugging

### Enable Debug Logging

In platformio.ini:
```ini
build_flags = 
    -DCORE_DEBUG_LEVEL=5
    -DARDUINO_LOG_LEVEL=ARDUINO_LOG_LEVEL_VERBOSE
```

### Serial Monitor Output

```bash
platformio device monitor --baud 115200 --rts 0 --dtr 0
```

### Common Debug Messages

- `log_i()` - Information (blue)
- `log_w()` - Warning (yellow)
- `log_e()` - Error (red)

### Memory Monitoring

Check free heap:
```cpp
log_i("Free heap: %d bytes", ESP.getFreeHeap());
```

### Performance Tips

1. **Minimize JSON parsing** - Parse only needed fields
2. **Use StaticJsonDocument** - Avoids dynamic allocation
3. **Cache data** - Store results locally
4. **Batch API calls** - Combine multiple queries
5. **Optimize display updates** - Only redraw changed regions

## Code Style Guidelines

### Naming Conventions

```cpp
// Classes: PascalCase
class MyClass { };

// Methods: camelCase
void myMethod();

// Constants: UPPER_SNAKE_CASE
const int MAX_DEVICES = 16;

// Variables: camelCase
int selectedItem;

// Private members: _leadingUnderscore or m_ prefix
int _internalState;
```

### Formatting

```cpp
// Braces on same line
if (condition) {
    doSomething();
}

// Spacing around operators
int result = a + b;

// Comments for complex logic
// Convert temperature from Celsius to Fahrenheit
float fahrenheit = (celsius * 9.0 / 5.0) + 32.0;
```

### Headers

```cpp
#ifndef MY_CLASS_H
#define MY_CLASS_H

#include <Arduino.h>
#include "other_header.h"

class MyClass { };

#endif
```

## Performance Targets

- **Display refresh**: <50ms
- **Button response**: <100ms
- **API response**: 100-500ms (network dependent)
- **Memory usage**: <2MB
- **Power consumption**: <500mA typical

## Common Pitfalls

### 1. Blocking Operations

**Bad:**
```cpp
while (!condition) {
    delay(1000);  // Blocks everything
}
```

**Good:**
```cpp
void update() {
    if (millis() - lastTime > 1000) {
        // Non-blocking check
    }
}
```

### 2. Memory Leaks

**Bad:**
```cpp
screenManager.switchScreen(type);
// Old screen pointer never deleted
```

**Good:**
```cpp
void ScreenManager::switchScreen(ScreenType type) {
    if (currentScreen) delete currentScreen;
    currentScreen = createScreen(type);
}
```

### 3. Race Conditions

**Bad:**
```cpp
if (apiClient.isConnected()) {
    apiClient.doSomething();  // Might disconnect between check and call
}
```

**Good:**
```cpp
apiClient.doSomething();  // Handle disconnect inside method
```

## Version Control

### Git Workflow

```bash
# Create feature branch
git checkout -b feature/my-feature

# Make changes and commit
git add .
git commit -m "Add new feature"

# Push and create PR
git push origin feature/my-feature
```

### Commit Messages

- Be descriptive: "Add heating control screen"
- Reference issues: "Fix #42: WiFi connection timeout"
- Keep commits logical and focused

## Documentation

### In-Code Comments

```cpp
// Complex algorithm needs explanation
// This iterates through files and sorts by date,
// skipping hidden files (starting with .)
for (const auto& file : files) {
    if (file[0] != '.') {
        // Process file...
    }
}
```

### Function Documentation

```cpp
/// Set the target temperature for a heater
/// @param heater The heater name (e.g., "heater_bed", "extruder")
/// @param temp Target temperature in Celsius
void setHeaterTarget(const String& heater, float temp);
```

## Troubleshooting Common Issues

### Build Errors

**Error**: `undefined reference to function`
- **Solution**: Check library is included in platformio.ini

**Error**: `out of memory`
- **Solution**: Use StaticJsonDocument with smaller capacity

### Runtime Errors

**Crash on startup**
- Check Config::loadFromStorage() - ensure SPIFFS is mounted
- Verify all required libraries are installed

**WiFi won't connect**
- Check SSID/password are correct
- Verify WiFi credentials saved properly
- Monitor serial output for error messages

### API Errors

**No printer connection**
- Verify printer IP address is correct
- Ensure Moonraker is running
- Check firewall allows port 7125
- Monitor network traffic for timeouts

## Tools & Resources

### Recommended Tools

- **VS Code** - Code editor with PlatformIO extension
- **Serial Monitor** - Monitor serial output
- **Moonraker Dashboard** - Verify printer setup
- **Network Monitor** - Check WiFi connectivity

### Useful Links

- [PlatformIO Docs](https://docs.platformio.org/)
- [M5StickS3 API](https://github.com/m5stack/M5Unified)
- [ArduinoJson Docs](https://arduinojson.org/)
- [Klipper Docs](https://www.klipper3d.org/)
- [Moonraker API](https://moonraker.readthedocs.io/)

## Contributing

### Code Review Checklist

- [ ] Code compiles without warnings
- [ ] Follows project coding style
- [ ] Includes appropriate comments
- [ ] Handles error cases
- [ ] Tested on hardware
- [ ] Documentation updated
- [ ] No memory leaks

### Pull Request Process

1. Fork repository
2. Create feature branch
3. Make changes and test
4. Submit PR with detailed description
5. Address review feedback
6. Merge after approval

## Future Development

### Planned Improvements

- [ ] WebSocket support for persistent connection
- [ ] Temperature graphs and logging
- [ ] Camera integration
- [ ] Macro execution UI
- [ ] OTA firmware updates
- [ ] Multi-printer support
- [ ] Custom themes
- [ ] Gesture controls

### Performance Optimizations

- [ ] Reduce network traffic
- [ ] Cache frequently accessed data
- [ ] Optimize display updates
- [ ] Minimize memory footprint
- [ ] Improve battery life

---

Happy coding! Feel free to contribute improvements and new features!
