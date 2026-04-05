#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include <Arduino.h>
#include <vector>

enum class ScreenType {
    SPLASH,
    WIFI_SETUP,
    NETWORK_SETUP,
    MAIN_MENU,
    PRINT_STATUS,
    PRINT_FILES,
    SETTINGS,
    HEATING_CONTROL,
    AXIS_CONTROL,
    POWER_CONTROL,
    PRINTER_INFO,
    MACROS,
    KLIPPY_STATE,  // FIX: add screen for klippy state handling
    ABOUT          // About screen with version and device info
};

class Screen {
public:
    virtual ~Screen() = default;
    virtual void init() = 0;
    virtual void draw() = 0;
    virtual void update() = 0;
    virtual void handleButtonA() = 0;
    virtual void handleButtonB() = 0;
    virtual void handleButtonALongPress() {}
    virtual void handleButtonBLongPress() {}
    virtual void handleButtonPress(uint8_t button) {}
};

class ScreenManager {
public:
    ScreenManager();
    ~ScreenManager();
    
    void switchScreen(ScreenType type);
    void update();
    void handleButtonA();
    void handleButtonB();
    void handleButtonALongPress();
    void handleButtonBLongPress();
    
private:
    Screen* currentScreen;
    ScreenType currentType;
    
    Screen* createScreen(ScreenType type);
};

#endif
