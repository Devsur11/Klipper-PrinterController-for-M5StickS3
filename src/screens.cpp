#include "screens.h"
#include "config.h"
#include <algorithm>

extern ScreenManager screenManager;
extern APIClient apiClient;
extern NetworkManager networkManager;

// Simplified keyboard layout - just characters we need
const char KEYBOARD_ROWS[4][9] = {
    {'0', '1', '2', '3', '4', '5', '6', '7', '8'},
    {'9', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'},
    {'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q'},
    {'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'}
};

// Special keys: 0=BACK(delete char), 1=SHIFT, 2=SPACE, 3=ENTER
// Col 4 on the special row is EXIT (back to SELECT state)
const char SPECIAL_KEYS[5] = {'<', '^', ' ', 'E', 'X'};
bool shiftActive = false;

// ========== SPLASH SCREEN ==========
void SplashScreen::init() {
    startTime = millis();
}

void SplashScreen::draw() {
    M5.Display.fillScreen(Theme::BG);

    DisplayUtils::drawCenteredText(50, "Klipper", Theme::PRIMARY);
    DisplayUtils::drawCenteredText(70, "Controller", Theme::PRIMARY);
    DisplayUtils::drawCenteredText(100, "v1.0", Theme::MUTED);

    unsigned long elapsed = millis() - startTime;
    if (elapsed > 2000) {
        if (!Config::hasWiFiCredentials()) {
            screenManager.switchScreen(ScreenType::WIFI_SETUP);
        } else if (!Config::hasKlipperIP()) {
            screenManager.switchScreen(ScreenType::NETWORK_SETUP);
        } else {
            screenManager.switchScreen(ScreenType::MAIN_MENU);
        }
    }
}

void SplashScreen::update() {}
void SplashScreen::handleButtonA() {}
void SplashScreen::handleButtonB() {}

// ========== WIFI SETUP SCREEN ==========

// Navigation map:
//   SCANNING  -> auto-advance to SELECT when scan finishes
//             -> timeout after 10 s goes back to SPLASH (FIX: was infinite trap)
//   SELECT    -> A: cycle networks  B: advance to ENTER_PASSWORD
//             -> long-press B: back to SPLASH (FIX: was no exit)
//   ENTER_PASSWORD
//             -> A: move cursor right (wraps per row)
//             -> long-press A: execute selected key
//             -> B: move cursor down (wraps)
//             -> long-press B: execute selected key (same as long-press A)
//             -> Special row, col 4 (EXIT key): back to SELECT  (FIX: was no exit)
//   CONNECTING -> auto-advance to NETWORK_SETUP on success
//             -> timeout after 15 s falls back to SELECT (FIX: was infinite trap)

WiFiSetupScreen::WiFiSetupScreen()
    : state(SCANNING), selectedNetwork(0),
      keyboardCursorX(0), keyboardCursorY(0),
      header(nullptr), footer(nullptr),
      stateEnteredAt(0) {}

void WiFiSetupScreen::init() {
    state = SCANNING;
    stateEnteredAt = millis();
    selectedNetwork = 0;
    password = "";
    keyboardCursorX = 0;
    keyboardCursorY = 0;

    if (header) delete header;
    if (footer) delete footer;

    header = new Header("WiFi Setup");
    footer = new Footer("Next", "Select");

    networkManager.scanNetworks();
}

void WiFiSetupScreen::draw() {
    M5.Display.fillScreen(Theme::BG);
    if (header) header->draw();

    int contentY = Display::HEADER_HEIGHT + 5;

    if (state == SCANNING) {
        DisplayUtils::drawCenteredText(contentY + 40, "Scanning WiFi...", Theme::FG);
        DisplayUtils::drawSpinner(Display::WIDTH / 2, contentY + 70, 5);
        // FIX: show the user they can bail out while scanning
        DisplayUtils::drawCenteredText(contentY + 95, "Hold B: cancel", Theme::MUTED);
        if (footer) footer->draw();
        return;
    }

    if (state == SELECT) {
        DisplayUtils::drawText(5, contentY, "Select Network:", Theme::FG);

        const auto& networks = networkManager.getNetworks();
        if (networks.empty()) {
            DisplayUtils::drawCenteredText(contentY + 40, "No networks found", Theme::MUTED);
            DisplayUtils::drawCenteredText(contentY + 55, "Hold B: back", Theme::MUTED);
        } else {
            for (size_t i = 0; i < networks.size() && i < 8; i++) {
                int y = contentY + 20 + (i * 15);
                uint16_t color = (i == (size_t)selectedNetwork) ? Theme::PRIMARY : Theme::MUTED;
                String signal = networks[i].rssi > -60 ? "[***]" : "[*]";
                String prefix = (i == (size_t)selectedNetwork) ? "> " : "  ";
                DisplayUtils::drawText(5, y, prefix + networks[i].ssid.substring(0, 12) + " " + signal, color);
            }
        }

        if (footer) {
            footer->setLeftText("Next");
            footer->setRightText("Enter");
            footer->draw();
        }
        // FIX: hint that long-press B exits
        DisplayUtils::drawText(5, Display::HEIGHT - Display::FOOTER_HEIGHT - 15 - 10, "Hold B: back", Theme::MUTED);
        return;
    }

    if (state == ENTER_PASSWORD) {
        DisplayUtils::drawText(5, contentY, "Password:", Theme::FG);

        String masked = "";
        for (size_t i = 0; i < password.length(); i++) masked += '*';
        if (masked.length() == 0) masked = "(empty)";
        DisplayUtils::drawText(5, contentY + 15, masked, Theme::PRIMARY);

        // FIX: hint about the EXIT key on the keyboard
        DisplayUtils::drawText(5, contentY + 28, "EXIT key = back to networks", Theme::MUTED);

        drawKeyboard(contentY + 40);

        if (footer) {
            footer->setLeftText("Move");
            footer->setRightText("Type");
            footer->draw();
        }
        return;
    }

    if (state == CONNECTING) {
        DisplayUtils::drawCenteredText(contentY + 40, "Connecting...", Theme::FG);
        DisplayUtils::drawSpinner(Display::WIDTH / 2, contentY + 65, 5);
        DisplayUtils::drawCenteredText(contentY + 90, "Hold B: cancel", Theme::MUTED);
    }

    // FIX: add CONNECTED state showing success and options to continue/retry
    if (state == CONNECTED) {
        DisplayUtils::drawCenteredText(contentY + 40, "WiFi Connected!", Theme::SUCCESS);
        DisplayUtils::drawCenteredText(contentY + 60, networkManager.getLocalIP(), Theme::PRIMARY);
        DisplayUtils::drawCenteredText(contentY + 80, "A: Retry", Theme::MUTED);
        DisplayUtils::drawCenteredText(contentY + 95, "B: Continue", Theme::MUTED);
    }

    if (footer) footer->draw();
}

void WiFiSetupScreen::drawKeyboard(int startY) {
    int keyW = 14;
    int keyH = 18;
    int spacing = 1;

    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 9; col++) {
            int x = 2 + (col * (keyW + spacing));
            int y = startY + (row * (keyH + spacing));

            uint16_t color = (row == keyboardCursorY && col == keyboardCursorX)
                             ? Theme::PRIMARY : Theme::MUTED;
            M5.Display.drawRect(x, y, keyW, keyH, color);

            char c = KEYBOARD_ROWS[row][col];
            if (shiftActive && c >= 'a' && c <= 'z') c = c - 'a' + 'A';

            M5.Display.setTextColor(color);
            M5.Display.setTextSize(1);
            M5.Display.setCursor(x + 4, y + 2);
            M5.Display.print(c);
        }
    }

    // Special key row -- now 5 keys: BACK, SHIFT, SPACE, ENTER, EXIT
    // EXIT (col 4) navigates back to the SELECT state.
    int specialY = startY + (4 * (keyH + spacing));

    // FIX: 5 keys now; widths adjusted to fit the same horizontal space
    const int NUM_SPECIAL = 5;
    int keyWidths[] = {22, 22, 22, 22, 22};
    const char* labels[] = {
        "BACK",
        shiftActive ? "SHFT*" : "shift",
        "SPC",
        "ENTR",
        "EXIT"   // <-- new: returns to SELECT without connecting
    };

    int xPos = 2;
    for (int i = 0; i < NUM_SPECIAL; i++) {
        uint16_t color = (keyboardCursorY == 4 && keyboardCursorX == i)
                         ? Theme::PRIMARY : Theme::MUTED;
        // FIX: EXIT key always shown in WARNING color so it stands out
        if (i == 4) color = (keyboardCursorY == 4 && keyboardCursorX == 4)
                            ? Theme::WARNING : Theme::MUTED;

        M5.Display.drawRect(xPos, specialY, keyWidths[i], keyH, color);
        M5.Display.setTextColor(color);
        M5.Display.setTextSize(1);

        String label = labels[i];
        int textLen = label.length() * 6;
        int textX = xPos + (keyWidths[i] - textLen) / 2;
        M5.Display.setCursor(textX, specialY + 3);
        M5.Display.print(label.c_str());

        xPos += keyWidths[i] + spacing;
    }
}

void WiFiSetupScreen::handleKeyboardInput(bool moveRight) {}

char WiFiSetupScreen::getSelectedKeyboardChar() {
    if (keyboardCursorY == 4) {
        if (keyboardCursorX < 5) return SPECIAL_KEYS[keyboardCursorX];
        return '?';
    }
    if (keyboardCursorY < 4 && keyboardCursorX < 9) {
        char c = KEYBOARD_ROWS[keyboardCursorY][keyboardCursorX];
        if (shiftActive && c >= 'a' && c <= 'z') c = c - 'a' + 'A';
        return c;
    }
    return '?';
}

// FIX: shared helper so both long-press A and long-press B do identical things
void WiFiSetupScreen::executeSelectedKey() {
    if (keyboardCursorY < 4) {
        char c = KEYBOARD_ROWS[keyboardCursorY][keyboardCursorX];
        if (shiftActive && c >= 'a' && c <= 'z') c = c - 'a' + 'A';
        if (password.length() < 63) password += c;
        // auto-shift off after one capital
        if (shiftActive) shiftActive = false;
    } else {
        // special key row
        switch (keyboardCursorX) {
            case 0: // BACK -- delete last char
                if (password.length() > 0) password.remove(password.length() - 1);
                break;
            case 1: // SHIFT
                shiftActive = !shiftActive;
                break;
            case 2: // SPACE
                if (password.length() < 63) password += ' ';
                break;
            case 3: // ENTER -- connect (open networks allowed)
                state = CONNECTING;
                stateEnteredAt = millis();
                networkManager.connectToWiFi(
                    networkManager.getNetworks()[selectedNetwork].ssid,
                    password);
                break;
            case 4: // EXIT -- back to network list without connecting
                state = SELECT;
                stateEnteredAt = millis();
                password = "";
                shiftActive = false;
                keyboardCursorX = 0;
                keyboardCursorY = 0;
                break;
        }
    }
}

void WiFiSetupScreen::update() {
    unsigned long now = millis();

    if (state == SCANNING) {
        // FIX: 10-second scan timeout so the user is never permanently trapped
        if (!networkManager.isScanning()) {
            state = SELECT;
            stateEnteredAt = now;
        } else if (now - stateEnteredAt > 10000) {
            //networkManager.stopScan();
            state = SELECT;
            stateEnteredAt = now;
        }
        return;
    }

    if (state == CONNECTING) {
        if (networkManager.isConnected()) {
            Config::ssid = networkManager.getNetworks()[selectedNetwork].ssid;
            Config::wifiPassword = password;
            Config::saveToStorage();
            // FIX: go to CONNECTED state to allow user to continue or retry
            state = CONNECTED;
            stateEnteredAt = now;
            return;
        }
        // FIX: 15-second connection timeout; fall back to SELECT so the user
        // can try again or choose a different network -- not permanently trapped
        if (now - stateEnteredAt > 15000) {
            networkManager.disconnect();
            state = SELECT;
            stateEnteredAt = now;
        }
    }
}

void WiFiSetupScreen::handleButtonA() {
    switch (state) {
        case SELECT: {
            const auto& networks = networkManager.getNetworks();
            if (!networks.empty())
                selectedNetwork = (selectedNetwork + 1) % networks.size();
            break;
        }
        case ENTER_PASSWORD:
            if (keyboardCursorY < 4)
                keyboardCursorX = (keyboardCursorX + 1) % 9;
            else
                keyboardCursorX = (keyboardCursorX + 1) % 5; // 5 special keys now
            break;
        case CONNECTED:
            // FIX: A button retries the connection in CONNECTED state
            networkManager.disconnect();
            state = SELECT;
            stateEnteredAt = millis();
            break;
        default:
            break;
    }
}

void WiFiSetupScreen::handleButtonALongPress() {
    if (state == ENTER_PASSWORD) executeSelectedKey();
}

void WiFiSetupScreen::handleButtonB() {
    switch (state) {
        case SELECT:
            // Advance from SELECT into ENTER_PASSWORD
            if (!networkManager.getNetworks().empty()) {
                state = ENTER_PASSWORD;
                stateEnteredAt = millis();
                password = "";
                keyboardCursorX = 0;
                keyboardCursorY = 0;
                shiftActive = false;
            }
            break;
        case ENTER_PASSWORD:
            // Move cursor row down, wrapping
            if (keyboardCursorY < 4) {
                keyboardCursorY++;
                // When landing on the special row, cap column to 4
                if (keyboardCursorY == 4 && keyboardCursorX >= 5)
                    keyboardCursorX = 4;
            } else {
                keyboardCursorY = 0;
            }
            break;
        case CONNECTED:
            // FIX: B button continues to network setup in CONNECTED state
            screenManager.switchScreen(ScreenType::NETWORK_SETUP);
            break;
        default:
            break;
    }
}

void WiFiSetupScreen::handleButtonBLongPress() {
    switch (state) {
        case SCANNING:
            // FIX: allow bailing out of a stuck scan
            //networkManager.stopScan();
            screenManager.switchScreen(ScreenType::SPLASH);
            break;
        case SELECT:
            // FIX: allow backing out to splash from the network list
            screenManager.switchScreen(ScreenType::SPLASH);
            break;
        case ENTER_PASSWORD:
            // FIX: long-press B also executes the selected key (same as long-press A)
            executeSelectedKey();
            break;
        case CONNECTING:
            // FIX: allow cancelling a stuck connection attempt
            networkManager.disconnect();
            state = SELECT;
            stateEnteredAt = millis();
            break;
        case CONNECTED:
            // FIX: allow backing out from connected state
            networkManager.disconnect();
            state = SELECT;
            stateEnteredAt = millis();
            break;
    }
}

// ========== NETWORK SETUP SCREEN ==========

// Navigation map:
//   ENTER_IP  -> A: move cursor column
//             -> long-press A: execute selected key (digit/dot/backspace/NEXT/BACK)
//             -> B: move cursor row
//             -> long-press B: save and continue (only when IP is valid)
//             -> B when disconnected: go to WIFI_SETUP instead of moving cursor
//             -> BACK key on keyboard: go to MAIN_MENU (FIX: was no soft exit)

NetworkSetupScreen::NetworkSetupScreen()
    : state(ENTER_IP), keyboardCursorX(0), keyboardCursorY(0),
      header(nullptr), footer(nullptr) {}

void NetworkSetupScreen::init() {
    state = ENTER_IP;
    klipperIP = Config::klipperIP; // pre-fill with whatever was saved
    keyboardCursorX = 0;
    keyboardCursorY = 0;

    if (header) delete header;
    if (footer) delete footer;

    //try connecting to the wifi with the saved credentials; if it works, we can skip the wifi setup screen and go straight to this one
    if (Config::hasWiFiCredentials()) {
        WiFi.begin(Config::ssid, Config::wifiPassword);
    }

    header = new Header("Printer IP");
    footer = new Footer("Nav", "Add");
}

void NetworkSetupScreen::draw() {
    M5.Display.fillScreen(Theme::BG);
    if (header) header->draw();

    int contentY = Display::HEADER_HEIGHT + 5;

    DisplayUtils::drawText(5, contentY, "WiFi:", Theme::FG);
    uint16_t wifiColor = networkManager.isConnected() ? Theme::SUCCESS : Theme::DANGER;
    DisplayUtils::drawText(40, contentY,
        networkManager.isConnected() ? "OK  " + networkManager.getLocalIP() : "Not connected",
        wifiColor);

    DisplayUtils::drawText(5, contentY + 18, "Printer IP:", Theme::FG);
    String display = klipperIP.length() > 0 ? klipperIP : "(empty)";
    DisplayUtils::drawText(5, contentY + 30, display, Theme::PRIMARY);

    drawIPKeyboard(contentY + 50);

    if (footer) {
        if (networkManager.isConnected()) {
            footer->setLeftText("Nav");
            footer->setRightText("Save");
        } else {
            footer->setLeftText("Nav");
            footer->setRightText("WiFi");
        }
        footer->draw();
    }

    // FIX: guide text -- BACK key navigates back to main menu
    DisplayUtils::drawText(5, Display::HEIGHT - Display::FOOTER_HEIGHT - 15, "BACK key = main menu", Theme::MUTED);
}

void NetworkSetupScreen::drawIPKeyboard(int startY) {
    // Row 0-1: digits. Row 2: special (dot, backspace, BACK=menu, NEXT=save)
    const char ROWS[3][5] = {
        {'1', '2', '3', '4', '5'},
        {'6', '7', '8', '9', '0'},
        {'.', '<', 'B', ' ', 'N'}   // B=Back-to-menu, N=Next/Save
    };

    int keyW = 25;
    int keyH = 20;
    int spacing = 1;

    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 5; col++) {
            int x = 2 + (col * (keyW + spacing));
            int y = startY + (row * (keyH + spacing));

            uint16_t color = (row == keyboardCursorY && col == keyboardCursorX)
                             ? Theme::PRIMARY : Theme::MUTED;

            // FIX: BACK key is highlighted in WARNING so it's easy to spot
            if (row == 2 && col == 2) {
                color = (keyboardCursorY == 2 && keyboardCursorX == 2)
                        ? Theme::WARNING : Theme::MUTED;
            }

            M5.Display.drawRect(x, y, keyW, keyH, color);
            M5.Display.setTextColor(color);
            M5.Display.setTextSize(1);

            String label;
            if (row == 2 && col == 1) label = "<";
            else if (row == 2 && col == 2) label = "BACK";
            else if (row == 2 && col == 3) label = "";
            else if (row == 2 && col == 4) label = "NEXT";
            else { label = String(ROWS[row][col]); }

            int textLen = label.length() * 6;
            M5.Display.setCursor(x + (keyW - textLen) / 2, y + 5);
            M5.Display.print(label.c_str());
        }
    }
}

char NetworkSetupScreen::getSelectedIPChar() {
    const char ROWS[3][5] = {
        {'1', '2', '3', '4', '5'},
        {'6', '7', '8', '9', '0'},
        {'.', '<', 'B', ' ', 'N'}
    };
    if (keyboardCursorY < 3 && keyboardCursorX < 5)
        return ROWS[keyboardCursorY][keyboardCursorX];
    return '?';
}

static bool isValidIPv4(const String& ip) {
    int dots = 0, octet = 0;
    bool hasDigit = false;
    for (size_t i = 0; i <= ip.length(); i++) {
        char c = (i < ip.length()) ? ip[i] : '.';
        if (c >= '0' && c <= '9') {
            octet = octet * 10 + (c - '0');
            hasDigit = true;
            if (octet > 255) return false;
        } else if (c == '.') {
            if (!hasDigit) return false;
            dots++; octet = 0; hasDigit = false;
        } else {
            return false;
        }
    }
    return dots == 4;
}

void NetworkSetupScreen::executeSelectedIPKey() {
    char c = getSelectedIPChar();
    if (c == '<') {
        if (klipperIP.length() > 0) klipperIP.remove(klipperIP.length() - 1);
    } else if (c == 'B') {
        // FIX: BACK key returns to main menu without saving
        screenManager.switchScreen(ScreenType::MAIN_MENU);
    } else if (c == 'N') {
        // NEXT -- validate and save
        if (isValidIPv4(klipperIP)) {
            Config::klipperIP = klipperIP;
            Config::saveToStorage();
            apiClient.connect(klipperIP, 7125);
            screenManager.switchScreen(ScreenType::MAIN_MENU);
        } else {
            // Draw inline error; user stays on screen
            DisplayUtils::drawCenteredText(Display::HEIGHT / 2, "Invalid IP!", Theme::DANGER);
        }
    } else if ((c >= '0' && c <= '9') || c == '.') {
        if (klipperIP.length() < 15) klipperIP += c;
    }
    // ignore space cell (col 3 of row 2)
}

void NetworkSetupScreen::update() {}

void NetworkSetupScreen::handleButtonA() {
    if (!networkManager.isConnected()) {
        // FIX: redirect straight to WiFi setup instead of doing nothing
        screenManager.switchScreen(ScreenType::WIFI_SETUP);
        return;
    }
    keyboardCursorX = (keyboardCursorX + 1) % 5;
}

void NetworkSetupScreen::handleButtonALongPress() {
    if (!networkManager.isConnected()) {
        screenManager.switchScreen(ScreenType::WIFI_SETUP);
        return;
    }
    executeSelectedIPKey();
}

void NetworkSetupScreen::handleButtonB() {
    // FIX: B now only moves cursor row (no redirect here; handleButtonA handles WiFi)
    keyboardCursorY = (keyboardCursorY + 1) % 3;
}

void NetworkSetupScreen::handleButtonBLongPress() {
    // FIX: long-press B = quick save shortcut (same as pressing NEXT key)
    if (isValidIPv4(klipperIP)) {
        Config::klipperIP = klipperIP;
        Config::saveToStorage();
        apiClient.connect(klipperIP, 7125);
        screenManager.switchScreen(ScreenType::MAIN_MENU);
    } else {
        DisplayUtils::drawCenteredText(Display::HEIGHT / 2, "Invalid IP!", Theme::DANGER);
    }
}

// ========== MAIN MENU SCREEN ==========

// Navigation map:
//   A: next item   long-press A: previous item
//   B / long-press B: enter selected item
//   (no "back" needed -- this IS the root screen)

MainMenuScreen::MainMenuScreen()
    : selectedItem(PRINT_STATUS), header(nullptr), footer(nullptr) {}

void MainMenuScreen::init() {
    selectedItem = PRINT_STATUS;

    //Check if klippy is connected; if not, go to klippy state screen
    if (!apiClient.isKlippyConnected()) {
        screenManager.switchScreen(ScreenType::KLIPPY_STATE);
        return;
    }

    if (header) delete header;
    if (footer) delete footer;

    header = new Header("Main Menu");
    footer = new Footer("Next", "Select");
}

void MainMenuScreen::draw() {
    M5.Display.fillScreen(Theme::BG);
    if (header) header->draw();

    int contentY = Display::HEADER_HEIGHT + 5;
    const char* items[] = {
        "Print Status",
        "File Browser",
        "Heating",
        "Axis Control",
        "Power Devices",
        "Macros",  // FIX: add macros
        "Printer Info",
        "Settings"
    };

    for (int i = 0; i < 8; i++) {  // FIX: 8 items now
        int y = contentY + (i * 18);
        uint16_t color = (i == selectedItem) ? Theme::PRIMARY : Theme::MUTED;
        String prefix = (i == selectedItem) ? "> " : "  ";
        DisplayUtils::drawText(5, y, prefix + items[i], color);
    }

    if (footer) {
        footer->setLeftText("Next");
        footer->setRightText("Select");
        footer->draw();
    }
}

void MainMenuScreen::update() {}

void MainMenuScreen::handleButtonA() {
    selectedItem = (selectedItem + 1) % 8;  // FIX: 8 items now
}

void MainMenuScreen::handleButtonALongPress() {
    selectedItem = (selectedItem - 1 + 8) % 8;  // FIX: 8 items now
}

void MainMenuScreen::handleButtonB() {
    ScreenType screenType;
    switch (selectedItem) {
        case PRINT_STATUS: screenType = ScreenType::PRINT_STATUS;     break;
        case FILE_BROWSER: screenType = ScreenType::PRINT_FILES;      break;
        case HEATING:      screenType = ScreenType::HEATING_CONTROL;  break;
        case AXIS_CONTROL: screenType = ScreenType::AXIS_CONTROL;     break;
        case POWER:        screenType = ScreenType::POWER_CONTROL;    break;
        case MACROS:       screenType = ScreenType::MACROS;          break;  // FIX: add macros
        case PRINTER_INFO: screenType = ScreenType::PRINTER_INFO;     break;
        case SETTINGS:     screenType = ScreenType::SETTINGS;         break;
        default:           screenType = ScreenType::MAIN_MENU;
    }
    screenManager.switchScreen(screenType);
}

void MainMenuScreen::handleButtonBLongPress() {
    handleButtonB();
}

// ========== PRINT STATUS SCREEN ==========

// Navigation map:
//   A: context action (pause / resume / cancel)
//   long-press A: emergency stop  (FIX: was not accessible anywhere)
//   B / long-press B: back to main menu

PrintStatusScreen::PrintStatusScreen()
    : progressBar(nullptr), header(nullptr), footer(nullptr), isLoading(false), loadingStartTime(0) {}

void PrintStatusScreen::init() {
    // FIX: Check if klippy is connected; if not, go to klippy state screen
    if (!apiClient.isKlippyConnected()) {
        screenManager.switchScreen(ScreenType::KLIPPY_STATE);
        return;
    }

    if (progressBar) delete progressBar;
    if (header) delete header;
    if (footer) delete footer;

    progressBar = new ProgressBar(10, 80, Display::WIDTH - 20, 30);
    header = new Header("Print Status");
    footer = new Footer("Action", "Back");
}

void PrintStatusScreen::draw() {
    M5.Display.fillScreen(Theme::BG);
    if (header) header->draw();

    // FIX: Check for loading timeout (5 seconds)
    if (isLoading && millis() - loadingStartTime > 5000) {
        isLoading = false;
    }

    // FIX: Show loading box during blocking operations
    if (isLoading) {
        DisplayUtils::drawLoadingBox("Processing...");
        if (footer) footer->draw();
        return;
    }

    int contentY = Display::HEADER_HEIGHT + 10;
    PrinterState state = apiClient.getPrinterState();

    String statusStr = state.printing ? "PRINTING"
                     : state.paused   ? "PAUSED"
                                      : "IDLE";
    uint16_t statusColor = state.printing ? Theme::SUCCESS
                         : state.paused   ? Theme::WARNING
                                          : Theme::MUTED;
    DisplayUtils::drawCenteredText(contentY, statusStr, statusColor);

    if (state.printing || state.paused) {
        DisplayUtils::drawText(5, contentY + 18, state.currentFile, Theme::FG);

        if (progressBar) {
            // FIX: Ensure progress is 0-100 (convert from 0-1 range if needed)
            float progress = state.printProgress;
            if (progress > 1.0f) progress = progress;  // Already in percentage
            else progress = progress * 100;  // Convert from 0-1 to 0-100
            progressBar->setValue(progress);
            progressBar->draw();
        }

        // Display temperatures
        String bedStr = String((int)state.bedTemp) + "C";
        if (state.bedTarget > 0) bedStr += "/" + String((int)state.bedTarget);
        DisplayUtils::drawText(5, contentY + 115, "Bed:    " + bedStr, Theme::FG);

        String nozzleStr = String((int)state.nozzleTemp) + "C";
        if (state.nozzleTarget > 0) nozzleStr += "/" + String((int)state.nozzleTarget);
        DisplayUtils::drawText(5, contentY + 130, "Nozzle: " + nozzleStr, Theme::FG);

        // FIX: Display time properly - ensure proper unit handling
        unsigned long elapsedMin = state.printTimeElapsed / 60;
        unsigned long remainingMin = state.printTimeRemaining / 60;
        DisplayUtils::drawText(5, contentY + 145,
            "Elapsed:   " + String(elapsedMin) + " min", Theme::FG);
        DisplayUtils::drawText(5, contentY + 160,
            "Remaining: " + String(remainingMin) + " min", Theme::FG);
    }

    if (footer) {
        if (state.printing) {
            footer->setLeftText("Pause");
        } else if (state.paused) {
            footer->setLeftText("Resume");
        } else {
            footer->setLeftText("Cancel");
        }
        footer->setRightText("Back");
        footer->draw();
    }

    // FIX: surface emergency-stop so user knows it exists
    DisplayUtils::drawText(5, Display::HEIGHT - Display::FOOTER_HEIGHT - 15, "Hold A: E-STOP", Theme::DANGER);
}

void PrintStatusScreen::update() {
    // Actively refresh printer state when on print status screen
    apiClient.queryPrinterState();
    apiClient.queryKlippyState();
    
    // FIX: Check if printer went offline during printing
    if (!apiClient.isKlippyConnected()) {
        screenManager.switchScreen(ScreenType::KLIPPY_STATE);
    }
}

void PrintStatusScreen::handleButtonA() {
    PrinterState state = apiClient.getPrinterState();
    // FIX: Show loading box for blocking print control operations
    isLoading = true;
    loadingStartTime = millis();
    if (state.printing)     apiClient.pausePrint();
    else if (state.paused)  apiClient.resumePrint();
    else                    apiClient.cancelPrint();
}

void PrintStatusScreen::handleButtonALongPress() {
    // FIX: emergency stop was unreachable; now bound to long-press A
    apiClient.emergencyStop();
}

void PrintStatusScreen::handleButtonB() {
    screenManager.switchScreen(ScreenType::MAIN_MENU);
}

void PrintStatusScreen::handleButtonBLongPress() {
    screenManager.switchScreen(ScreenType::MAIN_MENU);
}

// ========== FILE BROWSER SCREEN ==========

// Navigation map:
//   A: next file (clears confirm dialog)
//   long-press A: previous file
//   B: first press shows confirm dialog; second press starts print
//   long-press B: dismiss confirm dialog / back to main menu
//   (FIX: was impossible to dismiss the confirm dialog or go back to menu)

FileBrowserScreen::FileBrowserScreen()
    : selectedFile(0), header(nullptr), footer(nullptr), showConfirmDialog(false),
      marqueeTime(0), marqueeOffset(0), isLoading(false), loadingStartTime(0) {}

void FileBrowserScreen::init() {
    selectedFile = 0;
    showConfirmDialog = false;   // FIX: reset on every entry
    marqueeTime = millis();      // FIX: reset marquee animation
    marqueeOffset = 0;

    if (header) delete header;
    if (footer) delete footer;

    header = new Header("File Browser");
    footer = new Footer("Next", "Start");

    apiClient.queryFileList();
}

void FileBrowserScreen::draw() {
    M5.Display.fillScreen(Theme::BG);
    if (header) header->draw();

    // FIX: Check for loading timeout (5 seconds)
    if (isLoading && millis() - loadingStartTime > 5000) {
        isLoading = false;
    }

    // FIX: Show loading box when starting print
    if (isLoading) {
        DisplayUtils::drawLoadingBox("Starting print...");
        if (footer) footer->draw();
        return;
    }

    int contentY = Display::HEADER_HEIGHT + 5;
    const auto& files = apiClient.getFileList();

    if (files.empty()) {
        DisplayUtils::drawCenteredText(contentY + 50, "No files found", Theme::MUTED);
    } else {
        int visibleRows = 9;
        int scrollOffset = 0;
        if ((int)selectedFile >= visibleRows)
            scrollOffset = (int)selectedFile - visibleRows + 1;

        for (int i = scrollOffset;
             i < (int)files.size() && i < scrollOffset + visibleRows; i++) {
            int y = contentY + ((i - scrollOffset) * 15);
            uint16_t color = (i == (int)selectedFile) ? Theme::PRIMARY : Theme::MUTED;
            String prefix = (i == (int)selectedFile) ? "> " : "  ";
            
            // FIX: add marquee for long filenames
            String displayName = files[i];
            int maxLen = 20;  // Max chars without scrolling
            
            if (displayName.length() > maxLen) {
                // Filename is too long, apply marquee effect
                // Pad with spaces and scroll
                String padded = displayName + "   ";
                int scrollPos = marqueeOffset % (padded.length() + maxLen);
                displayName = padded.substring(scrollPos);
                if (displayName.length() < maxLen) {
                    displayName = displayName + padded.substring(0, maxLen - displayName.length());
                }
                displayName = displayName.substring(0, maxLen);
            }
            
            DisplayUtils::drawText(5, y, prefix + displayName, color);
        }
    }

    if (showConfirmDialog && !files.empty()) {
        // Semi-transparent overlay box
        M5.Display.fillRect(0, 95, Display::WIDTH, 50, Theme::SECONDARY);
        M5.Display.drawRect(0, 95, Display::WIDTH, 50, Theme::FG);
        DisplayUtils::drawCenteredText(100, "Start this file?", Theme::FG);
        // FIX: also apply marquee in dialog for very long names
        String dialogName = files[selectedFile];
        if (dialogName.length() > 18) {
            dialogName = dialogName.substring(0, 15) + "...";
        }
        DisplayUtils::drawCenteredText(115, dialogName, Theme::PRIMARY);
        DisplayUtils::drawCenteredText(132, "B=Yes  Hold B=No", Theme::MUTED);
    }

    if (footer) {
        if (showConfirmDialog) {
            footer->setLeftText("Cancel");
            footer->setRightText("Confirm");
        } else {
            footer->setLeftText("Next");
            footer->setRightText("Select");
        }
        footer->draw();
    }

    // FIX: guide user back to menu
    if (!showConfirmDialog)
        DisplayUtils::drawText(5, Display::HEIGHT - Display::FOOTER_HEIGHT - 15, "Hold B: main menu", Theme::MUTED);
}

void FileBrowserScreen::update() {
    // FIX: animate marquee effect for long filenames
    unsigned long now = millis();
    if (now - marqueeTime > 300) {  // Scroll every 300ms
        marqueeTime = now;
        marqueeOffset++;
    }
}

void FileBrowserScreen::handleButtonA() {
    const auto& files = apiClient.getFileList();
    if (!files.empty()) {
        if (showConfirmDialog) {
            // FIX: A cancels the confirm dialog rather than changing selection
            // underneath the user while the dialog is open
            showConfirmDialog = false;
        } else {
            selectedFile = (selectedFile + 1) % files.size();
        }
    }
}

void FileBrowserScreen::handleButtonALongPress() {
    const auto& files = apiClient.getFileList();
    if (!files.empty() && !showConfirmDialog) {
        selectedFile = (selectedFile - 1 + files.size()) % files.size();
    }
}

void FileBrowserScreen::handleButtonB() {
    const auto& files = apiClient.getFileList();
    if (files.empty()) return;

    if (showConfirmDialog) {
        // Confirmed -- start the print
        // FIX: Show loading box during blocking print start operation
        isLoading = true;
        loadingStartTime = millis();
        apiClient.startPrint(files[selectedFile]);
        screenManager.switchScreen(ScreenType::PRINT_STATUS);
    } else {
        // First press -- show confirmation dialog
        showConfirmDialog = true;
    }
}

void FileBrowserScreen::handleButtonBLongPress() {
    if (showConfirmDialog) {
        // FIX: long-press B dismisses confirm dialog
        showConfirmDialog = false;
    } else {
        // FIX: long-press B from the list goes back to main menu
        screenManager.switchScreen(ScreenType::MAIN_MENU);
    }
}

// ========== HEATING CONTROL SCREEN ==========

// Navigation map:
//   A: increase target temp (wraps at max)
//   long-press A: decrease target temp (FIX: was no way to go down)
//   B: confirm current target and advance to next heater / exit
//   long-press B: discard and go back to main menu  (FIX: was no exit)

HeatingControlScreen::HeatingControlScreen()
    : state(BED_TEMP), bedTarget(60), nozzleTarget(200),
      tempIndex(0), header(nullptr), footer(nullptr) {}

void HeatingControlScreen::init() {
    state = BED_TEMP;

    // Pre-fill targets from current printer state so the user sees
    // what is already set rather than hardcoded defaults
    PrinterState ps = apiClient.getPrinterState();
    bedTarget    = (ps.bedTarget    > 0) ? (int)ps.bedTarget    : 60;
    nozzleTarget = (ps.nozzleTarget > 0) ? (int)ps.nozzleTarget : 200;
    tempIndex = 0;

    if (header) delete header;
    if (footer) delete footer;

    header = new Header("Heating Control");
    footer = new Footer("Adjust", "Set");
}

void HeatingControlScreen::draw() {
    M5.Display.fillScreen(Theme::BG);
    if (header) header->draw();

    int contentY = Display::HEADER_HEIGHT + 10;
    PrinterState pstate = apiClient.getPrinterState();

    uint16_t bedColor    = (state == BED_TEMP)    ? Theme::PRIMARY : Theme::MUTED;
    uint16_t nozzleColor = (state == NOZZLE_TEMP) ? Theme::PRIMARY : Theme::MUTED;

    // Bed row
    DisplayUtils::drawText(5,  contentY,      "Bed:   actual", Theme::FG);
    DisplayUtils::drawText(95, contentY,      String(pstate.bedTemp, 1) + "C", Theme::SUCCESS);
    DisplayUtils::drawText(5,  contentY + 15, "       target", Theme::FG);
    DisplayUtils::drawText(95, contentY + 15, String(bedTarget) + "C", bedColor);

    // Nozzle row
    DisplayUtils::drawText(5,  contentY + 45, "Nozzle: actual", Theme::FG);
    DisplayUtils::drawText(95, contentY + 45, String(pstate.nozzleTemp, 1) + "C", Theme::SUCCESS);
    DisplayUtils::drawText(5,  contentY + 60, "        target", Theme::FG);
    DisplayUtils::drawText(95, contentY + 60, String(nozzleTarget) + "C", nozzleColor);

    // Active indicator
    DisplayUtils::drawText(5, contentY + 90,
        state == BED_TEMP ? "Editing: Bed" : "Editing: Nozzle", Theme::WARNING);

    if (footer) {
        footer->setLeftText("+5 deg");
        footer->setRightText(state == BED_TEMP ? "Next" : "Apply");
        footer->draw();
    }

    DisplayUtils::drawText(5, Display::HEIGHT - Display::FOOTER_HEIGHT - 15, "Hold A:-5  Hold B:back", Theme::MUTED);
}

void HeatingControlScreen::update() {
    // Actively refresh printer state to display current temperatures
    apiClient.queryPrinterState();
}

void HeatingControlScreen::handleButtonA() {
    if (state == BED_TEMP) {
        bedTarget += 5;
        if (bedTarget > 120) bedTarget = 0;
    } else {
        nozzleTarget += 5;
        if (nozzleTarget > 300) nozzleTarget = 0;
    }
}

void HeatingControlScreen::handleButtonALongPress() {
    // FIX: long-press A decreases temperature so the user can go down
    // without cycling through the entire range
    if (state == BED_TEMP) {
        bedTarget -= 5;
        if (bedTarget < 0) bedTarget = 120;
    } else {
        nozzleTarget -= 5;
        if (nozzleTarget < 0) nozzleTarget = 300;
    }
}

void HeatingControlScreen::handleButtonB() {
    if (state == BED_TEMP) {
        apiClient.setHeaterTarget("heater_bed", (float)bedTarget);
        state = NOZZLE_TEMP;
    } else {
        apiClient.setHeaterTarget("extruder", (float)nozzleTarget);
        screenManager.switchScreen(ScreenType::MAIN_MENU);
    }
}

void HeatingControlScreen::handleButtonBLongPress() {
    // FIX: long-press B exits without applying any temperature changes
    screenManager.switchScreen(ScreenType::MAIN_MENU);
}

// ========== AXIS CONTROL SCREEN ==========

// Navigation map:
//   A: cycle axis (X -> Y -> Z -> E -> X)
//   long-press A: cycle distance preset
//   B: show confirmation dialog
//   long-press B: back to main menu without moving  (FIX: was no safe exit)
//   Confirm dialog: B=move  long-press B=cancel

AxisControlScreen::AxisControlScreen()
    : selectedAxis(X), distance(10), mode(MOVE_MODE), header(nullptr), footer(nullptr),
      showConfirmDialog(false) {}

void AxisControlScreen::init() {
    selectedAxis = X;
    distance = 10;
    mode = MOVE_MODE;  // FIX: start in move mode
    showConfirmDialog = false;

    if (header) delete header;
    if (footer) delete footer;

    header = new Header("Axis Control");
    footer = new Footer("Mode", "Action");
}

void AxisControlScreen::draw() {
    M5.Display.fillScreen(Theme::BG);
    if (header) header->draw();

    int contentY = Display::HEADER_HEIGHT + 10;
    const char* axes[] = {"X", "Y", "Z", "E"};
    // FIX: show the available distance presets so user knows what to expect
    const float presets[] = {0.1f, 1.0f, 10.0f, 50.0f};

    // FIX: show mode selector
    uint16_t modeColor = Theme::WARNING;
    String modeStr = (mode == MOVE_MODE) ? "Mode: MOVE" : "Mode: HOME";
    DisplayUtils::drawText(5, contentY, modeStr, modeColor);

    DisplayUtils::drawText(5, contentY + 20,      "Axis:     ", Theme::FG);
    DisplayUtils::drawText(60, contentY + 20,     axes[selectedAxis], Theme::PRIMARY);

    if (mode == MOVE_MODE) {
        DisplayUtils::drawText(5, contentY + 35, "Distance: ", Theme::FG);
        DisplayUtils::drawText(60, contentY + 35, String(distance, 1) + " mm", Theme::PRIMARY);
        DisplayUtils::drawText(5, contentY + 50, "Feed:     30 mm/s", Theme::MUTED);
        if (selectedAxis == Z && distance > 5.0f) {
            DisplayUtils::drawText(5, contentY + 65, "! Large Z move", Theme::WARNING);
        }
    } else {
        DisplayUtils::drawText(5, contentY + 35, "Press B to home this", Theme::MUTED);
        DisplayUtils::drawText(5, contentY + 50, "axis", Theme::MUTED);
    }

    // FIX: confirm dialog before any motion -- axis moves are not undoable
    if (showConfirmDialog) {
        M5.Display.fillRect(10, 95, Display::WIDTH - 20, 50, Theme::SECONDARY);
        M5.Display.drawRect(10, 95, Display::WIDTH - 20, 50, Theme::FG);
        const char* axisName = axes[selectedAxis];
        String msg;
        if (mode == MOVE_MODE) {
            msg = String("Move ") + axisName + " " + String(distance, 1) + "mm?";
        } else {
            msg = String("Home ") + axisName + "?";
        }
        DisplayUtils::drawCenteredText(105, msg, Theme::FG);
        DisplayUtils::drawCenteredText(125, "B=Exec  Hold B=Cancel", Theme::MUTED);
    }

    if (footer) {
        if (showConfirmDialog) {
            footer->setLeftText("Cancel");
            footer->setRightText("Exec!");
        } else {
            footer->setLeftText("Mode");
            footer->setRightText("Action");
        }
        footer->draw();
    }

    DisplayUtils::drawText(5, Display::HEIGHT - Display::FOOTER_HEIGHT - 15, "Hold A:dist  Hold B:back", Theme::MUTED);
}

void AxisControlScreen::update() {}

void AxisControlScreen::handleButtonA() {
    if (showConfirmDialog) {
        showConfirmDialog = false;
    } else {
        // FIX: toggle between move and home modes
        mode = (mode == MOVE_MODE) ? HOME_MODE : MOVE_MODE;
    }
}

void AxisControlScreen::handleButtonALongPress() {
    // FIX: cycle through distance presets (only in move mode)
    if (mode == MOVE_MODE) {
        const float presets[] = {0.1f, 1.0f, 10.0f, 50.0f};
        const int numPresets = 4;
        int current = 0;
        for (int i = 0; i < numPresets; i++) {
            if (fabsf(distance - presets[i]) < 0.01f) { current = i; break; }
        }
        distance = presets[(current + 1) % numPresets];
    } else {
        // In home mode, cycle through axes
        selectedAxis = (Axis)((selectedAxis + 1) % 4);
    }
}

void AxisControlScreen::handleButtonB() {
    if (showConfirmDialog) {
        // Confirmed -- execute the action (move or home)
        String axis;
        switch (selectedAxis) {
            case X: axis = "X"; break;
            case Y: axis = "Y"; break;
            case Z: axis = "Z"; break;
            case E: axis = "E"; break;
        }
        if (mode == MOVE_MODE) {
            apiClient.moveAxis(axis, distance, 1800); // 30 mm/s = 1800 mm/min
        } else {
            apiClient.homeAxis(axis);  // FIX: execute home command
        }
        showConfirmDialog = false;
        // Stay on screen so user can make additional moves or homes
    } else {
        // First press -- show confirmation before moving
        showConfirmDialog = true;
    }
}

void AxisControlScreen::handleButtonBLongPress() {
    if (showConfirmDialog) {
        // FIX: cancel pending action (move or home)
        showConfirmDialog = false;
    } else {
        // FIX: exit to main menu without moving anything
        screenManager.switchScreen(ScreenType::MAIN_MENU);
    }
}

// ========== POWER CONTROL SCREEN ==========

// Navigation map:
//   A: next device
//   long-press A: previous device
//   B: toggle selected device (immediate -- power devices are low-risk)
//   long-press B: back to main menu  (FIX: was no exit)

PowerControlScreen::PowerControlScreen()
    : selectedDevice(0), header(nullptr), footer(nullptr),
      showConfirmDialog(false) {}

void PowerControlScreen::init() {
    selectedDevice = 0;
    showConfirmDialog = false;

    if (header) delete header;
    if (footer) delete footer;

    header = new Header("Power Control");
    footer = new Footer("Next", "Toggle");
}

void PowerControlScreen::draw() {
    M5.Display.fillScreen(Theme::BG);
    if (header) header->draw();

    int contentY = Display::HEADER_HEIGHT + 5;
    const auto& devices = apiClient.getPowerDevices();

    if (devices.empty()) {
        DisplayUtils::drawCenteredText(contentY + 50, "No power devices", Theme::MUTED);
    } else {
        for (size_t i = 0; i < devices.size() && i < 8; i++) {
            int y = contentY + (i * 18);
            uint16_t color = (i == (size_t)selectedDevice) ? Theme::PRIMARY : Theme::MUTED;
            uint16_t stateColor = devices[i].on ? Theme::SUCCESS : Theme::DANGER;
            String prefix = (i == (size_t)selectedDevice) ? "> " : "  ";
            DisplayUtils::drawText(5,  y, prefix + devices[i].name.substring(0, 14), color);
            DisplayUtils::drawText(100, y, devices[i].on ? "ON" : "OFF", stateColor);
        }
    }

    if (footer) {
        footer->setLeftText("Next");
        footer->setRightText("Toggle");
        footer->draw();
    }

    DisplayUtils::drawText(5, Display::HEIGHT - Display::FOOTER_HEIGHT - 15, "Hold B: main menu", Theme::MUTED);
}

void PowerControlScreen::update() {}

void PowerControlScreen::handleButtonA() {
    const auto& devices = apiClient.getPowerDevices();
    if (!devices.empty())
        selectedDevice = (selectedDevice + 1) % devices.size();
}

void PowerControlScreen::handleButtonALongPress() {
    // FIX: long-press A goes backwards through the device list
    const auto& devices = apiClient.getPowerDevices();
    if (!devices.empty())
        selectedDevice = (selectedDevice - 1 + devices.size()) % devices.size();
}

void PowerControlScreen::handleButtonB() {
    const auto& devices = apiClient.getPowerDevices();
    if (!devices.empty())
        apiClient.setPowerDevice(devices[selectedDevice].name, !devices[selectedDevice].on);
}

void PowerControlScreen::handleButtonBLongPress() {
    // FIX: back to main menu
    screenManager.switchScreen(ScreenType::MAIN_MENU);
}

// ========== PRINTER INFO SCREEN ==========

// Navigation map:
//   A / long-press A / B / long-press B: all go back to main menu
//   (read-only screen -- any button exits)

PrinterInfoScreen::PrinterInfoScreen()
    : header(nullptr), footer(nullptr), lastRefreshTime(0) {}

void PrinterInfoScreen::init() {
    if (header) delete header;
    if (footer) delete footer;

    header = new Header("Printer Info");
    footer = new Footer("Back", "Back");
    lastRefreshTime = millis();  // FIX: reset refresh timer on init
}

void PrinterInfoScreen::draw() {
    M5.Display.fillScreen(Theme::BG);
    if (header) header->draw();

    int contentY = Display::HEADER_HEIGHT + 10;

    DisplayUtils::drawText(5, contentY, "WiFi:", Theme::FG);
    DisplayUtils::drawText(45, contentY,
        networkManager.isConnected() ? "Connected" : "Disconnected",
        networkManager.isConnected() ? Theme::SUCCESS : Theme::DANGER);

    if (networkManager.isConnected())
        DisplayUtils::drawText(5, contentY + 15, networkManager.getLocalIP(), Theme::MUTED);

    DisplayUtils::drawText(5, contentY + 35, "Printer IP:", Theme::FG);
    DisplayUtils::drawText(5, contentY + 50, Config::klipperIP, Theme::MUTED);

    DisplayUtils::drawText(5, contentY + 70, "API:", Theme::FG);
    DisplayUtils::drawText(45, contentY + 70,
        apiClient.isConnected() ? "Connected" : "Disconnected",
        apiClient.isConnected() ? Theme::SUCCESS : Theme::DANGER);

    if (footer) {
        footer->setLeftText("Back");
        footer->setRightText("Back");
        footer->draw();
    }
}

void PrinterInfoScreen::update() {
    // FIX: Refresh every 5 seconds while on this screen
    unsigned long now = millis();
    if (now - lastRefreshTime >= 5000) {
        lastRefreshTime = now;
        // Query API to get updated connection status
        apiClient.queryPrinterState();  // This checks klippy connectivity too
    }
}

void PrinterInfoScreen::handleButtonA()          { screenManager.switchScreen(ScreenType::MAIN_MENU); }
void PrinterInfoScreen::handleButtonALongPress() { screenManager.switchScreen(ScreenType::MAIN_MENU); }
void PrinterInfoScreen::handleButtonB()          { screenManager.switchScreen(ScreenType::MAIN_MENU); }
void PrinterInfoScreen::handleButtonBLongPress() { screenManager.switchScreen(ScreenType::MAIN_MENU); }

// ========== SETTINGS SCREEN ==========

// Navigation map:
//   A: next item   long-press A: previous item
//   B: execute item action
//   long-press B: back to main menu regardless of selection (FIX: was no exit
//                 if you didn't navigate to the "Back" item first)

SettingsScreen::SettingsScreen()
    : selectedItem(WIFI), header(nullptr), footer(nullptr) {}

void SettingsScreen::init() {
    selectedItem = WIFI;

    if (header) delete header;
    if (footer) delete footer;

    header = new Header("Settings");
    footer = new Footer("Next", "Enter");
}

void SettingsScreen::draw() {
    M5.Display.fillScreen(Theme::BG);
    if (header) header->draw();

    int contentY = Display::HEADER_HEIGHT + 5;
    const char* items[] = {
        "WiFi Settings",
        "Printer IP",
        "Brightness",
        "Factory Reset",
        "Back"
    };

    for (int i = 0; i < 5; i++) {
        int y = contentY + (i * 18);
        uint16_t color = (i == selectedItem) ? Theme::PRIMARY : Theme::MUTED;
        String prefix = (i == selectedItem) ? "> " : "  ";
        DisplayUtils::drawText(5, y, prefix + items[i], color);
    }

    if (footer) {
        footer->setLeftText("Next");
        footer->setRightText("Enter");
        footer->draw();
    }

    DisplayUtils::drawText(5, Display::HEIGHT, "Hold B: back", Theme::MUTED);
}

void SettingsScreen::update() {}

void SettingsScreen::handleButtonA() {
    selectedItem = (selectedItem + 1) % 5;
}

void SettingsScreen::handleButtonALongPress() {
    selectedItem = (selectedItem - 1 + 5) % 5;
}

void SettingsScreen::handleButtonB() {
    switch (selectedItem) {
        case WIFI:
            Config::clearWiFiData();
            networkManager.disconnect();
            screenManager.switchScreen(ScreenType::WIFI_SETUP);
            break;
        case PRINTER_IP:
            screenManager.switchScreen(ScreenType::NETWORK_SETUP);
            break;
        case BRIGHTNESS:
            // TODO: inline brightness slider -- stub for now
            // When implemented, adjust M5.Display.setBrightness() here
            break;
        case RESET:
            Config::reset();
            screenManager.switchScreen(ScreenType::SPLASH);
            break;
        case DONE:
            screenManager.switchScreen(ScreenType::MAIN_MENU);
            break;
        default:
            break;
    }
}

void SettingsScreen::handleButtonBLongPress() {
    // FIX: always exit to main menu regardless of which item is highlighted
    screenManager.switchScreen(ScreenType::MAIN_MENU);
}

// ========== MACRO SCREEN ==========

// Navigation map:
//   A: next macro
//   long-press A: previous macro
//   B: first press shows confirm dialog; second press executes macro
//   long-press B: dismiss dialog / back to main menu

MacroScreen::MacroScreen()
    : selectedMacro(0), header(nullptr), footer(nullptr), 
      showConfirmDialog(false), marqueeTime(0), marqueeOffset(0) {}

void MacroScreen::init() {
    selectedMacro = 0;
    showConfirmDialog = false;
    marqueeTime = millis();
    marqueeOffset = 0;

    if (header) delete header;
    if (footer) delete footer;

    header = new Header("Macros");
    footer = new Footer("Next", "Execute");

    apiClient.queryMacros();
}

void MacroScreen::draw() {
    M5.Display.fillScreen(Theme::BG);
    if (header) header->draw();

    int contentY = Display::HEADER_HEIGHT + 5;
    const auto& macros = apiClient.getMacros();

    if (macros.empty()) {
        DisplayUtils::drawCenteredText(contentY + 50, "No macros available", Theme::MUTED);
    } else {
        int visibleRows = 9;
        int scrollOffset = 0;
        if ((int)selectedMacro >= visibleRows)
            scrollOffset = (int)selectedMacro - visibleRows + 1;

        for (int i = scrollOffset;
             i < (int)macros.size() && i < scrollOffset + visibleRows; i++) {
            int y = contentY + ((i - scrollOffset) * 15);
            uint16_t color = (i == (int)selectedMacro) ? Theme::PRIMARY : Theme::MUTED;
            String prefix = (i == (int)selectedMacro) ? "> " : "  ";
            
            // FIX: add marquee for long macro names
            String displayName = macros[i].name;
            int maxLen = 20;
            
            if (displayName.length() > maxLen) {
                String padded = displayName + "   ";
                int scrollPos = marqueeOffset % (padded.length() + maxLen);
                displayName = padded.substring(scrollPos);
                if (displayName.length() < maxLen) {
                    displayName = displayName + padded.substring(0, maxLen - displayName.length());
                }
                displayName = displayName.substring(0, maxLen);
            }
            
            DisplayUtils::drawText(5, y, prefix + displayName, color);
        }
    }

    if (showConfirmDialog && !macros.empty()) {
        M5.Display.fillRect(10, 95, Display::WIDTH - 20, 50, Theme::SECONDARY);
        M5.Display.drawRect(10, 95, Display::WIDTH - 20, 50, Theme::FG);
        DisplayUtils::drawCenteredText(100, "Execute macro?", Theme::FG);
        String name = macros[selectedMacro].name;
        if (name.length() > 16) name = name.substring(0, 13) + "...";
        DisplayUtils::drawCenteredText(115, name, Theme::PRIMARY);
        DisplayUtils::drawCenteredText(132, "B=Yes  Hold B=No", Theme::MUTED);
    }

    if (footer) {
        if (showConfirmDialog) {
            footer->setLeftText("Cancel");
            footer->setRightText("Execute");
        } else {
            footer->setLeftText("Next");
            footer->setRightText("Execute");
        }
        footer->draw();
    }

    if (!showConfirmDialog)
        DisplayUtils::drawText(5, Display::HEIGHT - Display::FOOTER_HEIGHT - 15, "Hold B: main menu", Theme::MUTED);
}

void MacroScreen::update() {
    // FIX: animate marquee effect
    unsigned long now = millis();
    if (now - marqueeTime > 300) {
        marqueeTime = now;
        marqueeOffset++;
    }
}

void MacroScreen::handleButtonA() {
    const auto& macros = apiClient.getMacros();
    if (!macros.empty()) {
        if (showConfirmDialog) {
            showConfirmDialog = false;
        } else {
            selectedMacro = (selectedMacro + 1) % macros.size();
        }
    }
}

void MacroScreen::handleButtonALongPress() {
    const auto& macros = apiClient.getMacros();
    if (!macros.empty() && !showConfirmDialog) {
        selectedMacro = (selectedMacro - 1 + macros.size()) % macros.size();
    }
}

void MacroScreen::handleButtonB() {
    const auto& macros = apiClient.getMacros();
    if (macros.empty()) return;

    if (showConfirmDialog) {
        apiClient.executeMacro(macros[selectedMacro].name);
        showConfirmDialog = false;
    } else {
        showConfirmDialog = true;
    }
}

void MacroScreen::handleButtonBLongPress() {
    if (showConfirmDialog) {
        showConfirmDialog = false;
    } else {
        screenManager.switchScreen(ScreenType::MAIN_MENU);
    }
}

// ========== KLIPPY STATE SCREEN ==========

// Navigation map:
//   MAIN menu: A=next option, B=select action
//   Options: Power Control, Return to Main Menu
//   When offline: Can toggle power devices to restart printer

KlippyStateScreen::KlippyStateScreen()
    : submenu(MAIN), selectedDeviceOrAction(0), header(nullptr), footer(nullptr), selectedDevice(""),
      isLoading(false), loadingStartTime(0), marqueeTime(0), marqueeOffset(0) {}

void KlippyStateScreen::init() {
    submenu = MAIN;
    selectedDeviceOrAction = 0;
    marqueeTime = millis();  // FIX: reset marquee animation
    marqueeOffset = 0;

    if (header) delete header;
    if (footer) delete footer;

    String title = "Klippy Status";
    String state = apiClient.getKlippyState();
    if (state == "offline") {
        title = "Klippy Offline";
    } else if (state == "error") {
        title = "Klippy Error";
    } else if (state == "startup") {
        title = "Klippy Starting";
    }

    header = new Header(title);
    footer = new Footer("Select", "Enter");
}

void KlippyStateScreen::draw() {
    M5.Display.fillScreen(Theme::BG);
    if (header) header->draw();

    // FIX: Check for loading timeout (5 seconds)
    if (isLoading && millis() - loadingStartTime > 5000) {
        isLoading = false;
    }

    // FIX: Show loading box during power control operations
    if (isLoading) {
        DisplayUtils::drawLoadingBox("Controlling power...");
        if (footer) footer->draw();
        return;
    }

    int contentY = Display::HEADER_HEIGHT + 10;
    String state = apiClient.getKlippyState();

    // Display current state
    DisplayUtils::drawCenteredText(contentY, "State: " + state, Theme::MUTED);

    // Display appropriate message based on state
    String message = "";
    uint16_t messageColor = Theme::WARNING;
    if (state == "offline") {
        message = "Klippy host not responding";
        messageColor = Theme::DANGER;
    } else if (state == "error") {
        message = "Klippy encountered an error";
        messageColor = Theme::DANGER;
    } else if (state == "startup") {
        message = "Klippy initializing...";
        messageColor = Theme::WARNING;
    } else if (state == "shutdown") {
        message = "Klippy shutdown";
        messageColor = Theme::WARNING;
    } else if (state == "ready") {
        screenManager.switchScreen(ScreenType::MAIN_MENU);
        return;
    }

    // FIX: Add marquee effect for long messages using M5 display dimensions
    String displayMessage = message;
    int displayWidth = M5.Display.width();  // Get width from M5 class
    int maxMessageWidth = displayWidth - 20;  // Leave 10px margin on each side
    
    // Apply marquee if message is too long
    if (displayMessage.length() > 20) {  // Rough estimate: ~3 chars per 10px
        String padded = displayMessage + "   ";
        int scrollPos = marqueeOffset % (padded.length() + 20);
        displayMessage = padded.substring(scrollPos);
        if (displayMessage.length() < 20) {
            displayMessage = displayMessage + padded.substring(0, 20 - displayMessage.length());
        }
        displayMessage = displayMessage.substring(0, 20);
    }

    DisplayUtils::drawCenteredText(contentY + 30, displayMessage, messageColor);

    // Display menu
    if (submenu == MAIN) {
        const char* options[] = {
            "Power Control",
            "Restart Printer",
            "Settings"
        };

        for (int i = 0; i <3; i++) {
            int y = contentY + 70 + (i * 20);
            uint16_t color = (i == selectedDeviceOrAction) ? Theme::PRIMARY : Theme::MUTED;
            String prefix = (i == selectedDeviceOrAction) ? "> " : "  ";
            DisplayUtils::drawText(5, y, prefix + options[i], color);
        }
    } else if (submenu == POWER_DEVICES) {
        const auto& devices = apiClient.getPowerDevices();

        if (devices.empty()) {
            DisplayUtils::drawCenteredText(contentY + 60, "No power devices", Theme::MUTED);
        } else {
            DisplayUtils::drawCenteredText(contentY + 60, "Select device:", Theme::FG);
            for (size_t i = 0; i < devices.size() && i < 4; i++) {
                int y = contentY + 80 + (i * 18);
                uint16_t color = (i == (size_t)selectedDeviceOrAction) ? Theme::PRIMARY : Theme::MUTED;
                String prefix = (i == (size_t)selectedDeviceOrAction) ? "> " : "  ";
                DisplayUtils::drawText(5, y, prefix + devices[i].name, color);
            }
        }
    }

    if (footer) {
        if (submenu == MAIN) {
            footer->setLeftText("Select");
            footer->setRightText("Action");
        } else if (submenu == POWER_DEVICES) {
            footer->setLeftText("Back");
            footer->setRightText("Toggle");
        }
        footer->draw();
    }
}

void KlippyStateScreen::update() {
    // FIX: regularly check if klippy state changes back to ready
    apiClient.queryKlippyState();
    //try to connect to api if we are offline, in case klippy just restarted
    apiClient.connect(Config::klipperIP, Config::klipperPort);
    
    // FIX: animate marquee effect for long messages
    unsigned long now = millis();
    if (now - marqueeTime > 300) {  // Scroll every 300ms
        marqueeTime = now;
        marqueeOffset++;
    }
}

void KlippyStateScreen::handleButtonA() {
    if (submenu == MAIN) {
        selectedDeviceOrAction = (selectedDeviceOrAction + 1) % 3;
    } else if (submenu == POWER_DEVICES) {
        const auto& devices = apiClient.getPowerDevices();
        if (!devices.empty()) {
            selectedDeviceOrAction = (selectedDeviceOrAction + 1) % devices.size();
        }
    }
}

void KlippyStateScreen::handleButtonALongPress() {
    // Go back to main submenu
    if (submenu == POWER_DEVICES) {
        submenu = MAIN;
        selectedDeviceOrAction = 0;
    }
}

void KlippyStateScreen::handleButtonB() {
    if (submenu == MAIN) {
        switch (selectedDeviceOrAction) {
            case 0:  // Power Control
                submenu = POWER_DEVICES;
                selectedDeviceOrAction = 0;
                break;
            case 1:  // Restart Printer
                apiClient.sendGcode("FIRMWARE_RESTART");
                break;
            case 2:  // Settings
                screenManager.switchScreen(ScreenType::SETTINGS);
                break;
        }
    } else if (submenu == POWER_DEVICES) {
        const auto& devices = apiClient.getPowerDevices();
        if (!devices.empty()) {
            // FIX: Show loading box during power device toggle operation
            isLoading = true;
            loadingStartTime = millis();
            // Toggle selected power device
            apiClient.setPowerDevice(devices[selectedDeviceOrAction].name, 
                                     !devices[selectedDeviceOrAction].on);
        }
    }
}

void KlippyStateScreen::handleButtonBLongPress() {
    if (submenu == MAIN) {
        // Go back to main menu
        screenManager.switchScreen(ScreenType::MAIN_MENU);
    } else if (submenu == POWER_DEVICES) {
        // Go back to main submenu
        submenu = MAIN;
        selectedDeviceOrAction = 0;
    }
}