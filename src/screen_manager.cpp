#include "screen_manager.h"
#include "screens.h"
#include "network.h"
#include "esp_log.h"

static const char* TAG = "ScreenManager";

extern NetworkManager networkManager;

ScreenManager::ScreenManager() : currentScreen(nullptr), currentType(ScreenType::SPLASH) {
    currentScreen = createScreen(ScreenType::SPLASH);
    if (currentScreen) {
        currentScreen->init();
    }
}

ScreenManager::~ScreenManager() {
    if (currentScreen) {
        delete currentScreen;
    }
}

void ScreenManager::switchScreen(ScreenType type) {
    // Gate access to MAIN_MENU - require WiFi connection
    if (type == ScreenType::MAIN_MENU && !networkManager.isConnected()) {
        ESP_LOGW(TAG, "Cannot access MAIN_MENU - WiFi not connected. Redirecting to NETWORK_SETUP.");
        type = ScreenType::NETWORK_SETUP;
    }
    
    if (currentScreen) {
        delete currentScreen;
    }
    
    currentScreen = createScreen(type);
    currentType = type;
    
    if (currentScreen) {
        currentScreen->init();
    }
}

void ScreenManager::update() {
    if (currentScreen) {
        currentScreen->update();
        currentScreen->draw();
    }
}

void ScreenManager::handleButtonA() {
    if (currentScreen) {
        currentScreen->handleButtonA();
    }
}

void ScreenManager::handleButtonB() {
    if (currentScreen) {
        currentScreen->handleButtonB();
    }
}

void ScreenManager::handleButtonALongPress() {
    if (currentScreen) {
        currentScreen->handleButtonALongPress();
    }
}

void ScreenManager::handleButtonBLongPress() {
    if (currentScreen) {
        currentScreen->handleButtonBLongPress();
    }
}

Screen* ScreenManager::createScreen(ScreenType type) {
    switch (type) {
        case ScreenType::SPLASH:
            return new SplashScreen();
        case ScreenType::WIFI_SETUP:
            return new WiFiSetupScreen();
        case ScreenType::NETWORK_SETUP:
            return new NetworkSetupScreen();
        case ScreenType::MAIN_MENU:
            return new MainMenuScreen();
        case ScreenType::PRINT_STATUS:
            return new PrintStatusScreen();
        case ScreenType::PRINT_FILES:
            return new FileBrowserScreen();
        case ScreenType::SETTINGS:
            return new SettingsScreen();
        case ScreenType::HEATING_CONTROL:
            return new HeatingControlScreen();
        case ScreenType::AXIS_CONTROL:
            return new AxisControlScreen();
        case ScreenType::POWER_CONTROL:
            return new PowerControlScreen();
        case ScreenType::PRINTER_INFO:
            return new PrinterInfoScreen();
        case ScreenType::MACROS:  // FIX: add macros screen
            return new MacroScreen();
        case ScreenType::KLIPPY_STATE:  // FIX: add klippy state screen
            return new KlippyStateScreen();
        case ScreenType::ABOUT:
            return new AboutScreen();
        default:
            return new SplashScreen();
    }
}
