#ifndef SCREENS_H
#define SCREENS_H

#include "screen_manager.h"
#include "api_client.h"
#include "network.h"
#include "display.h"
#include <vector>
#include <memory>

// ============================================================
// SplashScreen
// ============================================================
class SplashScreen : public Screen {
public:
    void init() override;
    void draw() override;
    void update() override;
    void handleButtonA() override;
    void handleButtonB() override;

private:
    unsigned long startTime;
};

// ============================================================
// WiFiSetupScreen
// ============================================================
class WiFiSetupScreen : public Screen {
public:
    WiFiSetupScreen();
    void init() override;
    void draw() override;
    void update() override;
    void handleButtonA() override;
    void handleButtonB() override;
    void handleButtonALongPress() override;
    void handleButtonBLongPress() override;

private:
    enum State { SCANNING, SELECT, ENTER_PASSWORD, CONNECTING, CONNECTED };  // FIX: add CONNECTED state for retry
    State state;
    int selectedNetwork;
    String password;
    int keyboardCursorX;
    int keyboardCursorY;
    Header* header;
    Footer* footer;

    // Timestamp of when the current state was entered; used for timeouts.
    unsigned long stateEnteredAt;

    void drawKeyboard(int startY);
    void handleKeyboardInput(bool moveRight); // legacy stub, not called
    char getSelectedKeyboardChar();

    // Shared key-execution logic called by both long-press A and long-press B.
    void executeSelectedKey();
};

// ============================================================
// NetworkSetupScreen
// ============================================================
class NetworkSetupScreen : public Screen {
public:
    NetworkSetupScreen();
    void init() override;
    void draw() override;
    void update() override;
    void handleButtonA() override;
    void handleButtonB() override;
    void handleButtonALongPress() override;
    void handleButtonBLongPress() override;

private:
    // CONFIRM state is unused now; ENTER_IP covers everything.
    // Kept as a named value so existing serialised state (if any) is safe.
    enum State { ENTER_IP, CONFIRM };
    State state;
    String klipperIP;
    int keyboardCursorX;
    int keyboardCursorY;
    Header* header;
    Footer* footer;

    void drawIPKeyboard(int startY);
    char getSelectedIPChar();

    // Execute whatever key is currently highlighted in the IP keyboard.
    void executeSelectedIPKey();
};

// ============================================================
// MainMenuScreen
// ============================================================
class MainMenuScreen : public Screen {
public:
    MainMenuScreen();
    void init() override;
    void draw() override;
    void update() override;
    void handleButtonA() override;
    void handleButtonB() override;
    void handleButtonALongPress() override;
    void handleButtonBLongPress() override;

private:
    enum MenuItem {
        PRINT_STATUS,
        FILE_BROWSER,
        HEATING,
        AXIS_CONTROL,
        POWER,
        MACROS,  // FIX: add macros menu item
        PRINTER_INFO,
        SETTINGS
    };
    int selectedItem;
    Header* header;
    Footer* footer;
};

// ============================================================
// PrintStatusScreen
// ============================================================
class PrintStatusScreen : public Screen {
public:
    PrintStatusScreen();
    void init() override;
    void draw() override;
    void update() override;
    void handleButtonA() override;
    void handleButtonB() override;
    void handleButtonALongPress() override; // emergency stop
    void handleButtonBLongPress() override; // back to main menu

private:
    ProgressBar* progressBar;
    Header* header;
    Footer* footer;
    bool isLoading;  // FIX: show loading box during blocking operations
    unsigned long loadingStartTime;  // FIX: timeout after 5 seconds
};

// ============================================================
// FileBrowserScreen
// ============================================================
class FileBrowserScreen : public Screen {
public:
    FileBrowserScreen();
    void init() override;
    void draw() override;
    void update() override;
    void handleButtonA() override;
    void handleButtonB() override;
    void handleButtonALongPress() override; // scroll backward
    void handleButtonBLongPress() override; // dismiss dialog / back to menu

private:
    int selectedFile;
    Header* header;
    Footer* footer;
    bool showConfirmDialog;
    unsigned long marqueeTime;  // FIX: add marquee animation for long filenames
    int marqueeOffset;  // FIX: track scroll position
    bool isLoading;  // FIX: show loading box when starting print
    unsigned long loadingStartTime;  // FIX: timeout after 5 seconds
};

// ============================================================
// HeatingControlScreen
// ============================================================
class HeatingControlScreen : public Screen {
public:
    HeatingControlScreen();
    void init() override;
    void draw() override;
    void update() override;
    void handleButtonA() override;
    void handleButtonB() override;
    void handleButtonALongPress() override; // decrease temperature
    void handleButtonBLongPress() override; // exit without applying

private:
    enum ControlState { BED_TEMP, NOZZLE_TEMP, CONFIRM };
    ControlState state;
    int bedTarget;      // stored as int (degrees C); cast to float on send
    int nozzleTarget;
    int tempIndex;      // reserved for future preset-index use
    Header* header;
    Footer* footer;
};

// ============================================================
// AxisControlScreen
// ============================================================
class AxisControlScreen : public Screen {
public:
    AxisControlScreen();
    void init() override;
    void draw() override;
    void update() override;
    void handleButtonA() override;
    void handleButtonB() override;
    void handleButtonALongPress() override; // cycle distance preset
    void handleButtonBLongPress() override; // cancel dialog / back to menu

private:
    enum Axis { X, Y, Z, E };
    enum Mode { MOVE_MODE, HOME_MODE };  // FIX: add home mode
    Axis selectedAxis;
    Mode mode;  // FIX: select between move and home
    float distance;
    Header* header;
    Footer* footer;
    bool showConfirmDialog; // true while waiting for move confirmation
};

// ============================================================
// PowerControlScreen
// ============================================================
class PowerControlScreen : public Screen {
public:
    PowerControlScreen();
    void init() override;
    void draw() override;
    void update() override;
    void handleButtonA() override;
    void handleButtonB() override;
    void handleButtonALongPress() override; // scroll backward
    void handleButtonBLongPress() override; // back to main menu

private:
    int selectedDevice;
    Header* header;
    Footer* footer;
    bool showConfirmDialog; // reserved; not used for power (low-risk toggle)
};

// ============================================================
// PrinterInfoScreen
// ============================================================
class PrinterInfoScreen : public Screen {
public:
    PrinterInfoScreen();
    void init() override;
    void draw() override;
    void update() override;
    void handleButtonA() override;          // back to main menu
    void handleButtonB() override;          // back to main menu
    void handleButtonALongPress() override; // back to main menu
    void handleButtonBLongPress() override; // back to main menu

private:
    Header* header;
    Footer* footer;
    unsigned long lastRefreshTime;  // FIX: track last refresh time for 5-sec auto-refresh
};

// ============================================================
// SettingsScreen
// ============================================================
class SettingsScreen : public Screen {
public:
    SettingsScreen();
    void init() override;
    void draw() override;
    void update() override;
    void handleButtonA() override;
    void handleButtonB() override;
    void handleButtonALongPress() override; // scroll backward
    void handleButtonBLongPress() override; // back to main menu unconditionally

private:
    enum SettingItem { WIFI, PRINTER_IP, BRIGHTNESS, RESET, DONE };
    int selectedItem;
    Header* header;
    Footer* footer;
};

// ============================================================
// MacroScreen  // FIX: add macros menu
// ============================================================
class MacroScreen : public Screen {
public:
    MacroScreen();
    void init() override;
    void draw() override;
    void update() override;
    void handleButtonA() override;
    void handleButtonB() override;
    void handleButtonALongPress() override; // scroll backward
    void handleButtonBLongPress() override; // back to main menu

private:
    int selectedMacro;
    Header* header;
    Footer* footer;
    bool showConfirmDialog;
    unsigned long marqueeTime;  // FIX: add marquee for macro names
    int marqueeOffset;
};

// ============================================================
// KlippyStateScreen  // FIX: handle klippy state changes
// ============================================================
class KlippyStateScreen : public Screen {
public:
    KlippyStateScreen();
    void init() override;
    void draw() override;
    void update() override;
    void handleButtonA() override;
    void handleButtonB() override;
    void handleButtonALongPress() override;
    void handleButtonBLongPress() override;

private:
    enum SubmenuState { MAIN, POWER_DEVICES, ACTIONS };
    SubmenuState submenu;
    int selectedDeviceOrAction;
    Header* header;
    Footer* footer;
    String selectedDevice;  // Store selected power device for control
    bool isLoading;  // FIX: show loading box during power control operations
    unsigned long loadingStartTime;  // FIX: timeout after 5 seconds
    unsigned long marqueeTime;  // FIX: track marquee animation timing
    int marqueeOffset;  // FIX: track scroll position for text marquee
};

#endif // SCREENS_H