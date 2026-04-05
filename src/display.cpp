#include "display.h"
#include <cmath>

// Button Implementation
Button::Button(int x, int y, int w, int h, const String& label)
    : x(x), y(y), w(w), h(h), label(label), pressed(false) {}

void Button::draw() {
    uint16_t color = pressed ? Theme::PRIMARY : Theme::SECONDARY;
    M5.Display.fillRect(x, y, w, h, color);
    M5.Display.drawRect(x, y, w, h, Theme::FG);
    
    M5.Display.setTextColor(Theme::FG);
    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.drawString(label, x + w/2, y + h/2);
}

bool Button::handlePress(int px, int py) {
    if (px >= x && px < x + w && py >= y && py < y + h) {
        pressed = true;
        if (callback) callback();
        return true;
    }
    pressed = false;
    return false;
}

// ProgressBar Implementation
ProgressBar::ProgressBar(int x, int y, int w, int h)
    : x(x), y(y), w(w), h(h), value(0) {}

void ProgressBar::draw() {
    M5.Display.drawRect(x, y, w, h, Theme::FG);
    
    int fillWidth = (w - 2) * value / 100;
    if (fillWidth > 0) {
        M5.Display.fillRect(x + 1, y + 1, fillWidth, h - 2, Theme::SUCCESS);
    }
    
    M5.Display.setTextColor(Theme::FG);
    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.drawString(String(value) + "%", x + w/2, y + h/2);
}

// Header Implementation
Header::Header(const String& title) : title(title) {}

void Header::draw() {
    M5.Display.fillRect(0, 0, Display::WIDTH, Display::HEADER_HEIGHT, Theme::PRIMARY);
    M5.Display.setTextColor(Theme::FG);
    M5.Display.setTextDatum(ML_DATUM);
    M5.Display.drawString(title, 2, Display::HEADER_HEIGHT / 2);
    
    // Update power system to refresh battery reading
    M5.update();
    // M5.Power.update();
    
    int batteryLevel = M5.Power.getBatteryLevel();
    // Clamp battery level to valid range (0-100%)
    if (batteryLevel < 0) {
        batteryLevel = 0;
    }
    if (batteryLevel > 100) {
        batteryLevel = 100;
    }
    String batteryText = String(batteryLevel) + "%";
    M5.Display.setTextDatum(MR_DATUM);
    M5.Display.drawString(batteryText, Display::WIDTH - 2, Display::HEADER_HEIGHT / 2);
}

// Footer Implementation
Footer::Footer(const String& leftText, const String& rightText)
    : leftText(leftText), rightText(rightText) {}

void Footer::draw() {
    int footerY = Display::HEIGHT - Display::FOOTER_HEIGHT;
    M5.Display.fillRect(0, footerY, Display::WIDTH, Display::FOOTER_HEIGHT, Theme::SECONDARY);
    
    M5.Display.setTextColor(Theme::FG);
    //dont ask me what I did here, just lazy me
    if (rightText.length() > 0) {
        M5.Display.setTextDatum(ML_DATUM);
        M5.Display.drawString(rightText, 2, footerY + Display::FOOTER_HEIGHT / 2);
    }
    
    if (leftText.length() > 0) {
        M5.Display.setTextDatum(MR_DATUM);
        M5.Display.drawString(leftText, Display::WIDTH - 2, footerY + Display::FOOTER_HEIGHT / 2);
    }
}

// DisplayUtils Implementation
void DisplayUtils::drawRect(int x, int y, int w, int h, uint16_t color, bool fill) {
    if (fill) {
        M5.Display.fillRect(x, y, w, h, color);
    } else {
        M5.Display.drawRect(x, y, w, h, color);
    }
}

void DisplayUtils::drawText(int x, int y, const String& text, uint16_t color) {
    M5.Display.setTextColor(color);
    M5.Display.setTextDatum(TL_DATUM);
    M5.Display.drawString(text, x, y);
}

void DisplayUtils::drawCenteredText(int y, const String& text, uint16_t color) {
    M5.Display.setTextColor(color);
    M5.Display.setTextDatum(TC_DATUM);
    M5.Display.drawString(text, Display::WIDTH / 2, y);
}

void DisplayUtils::drawSpinner(int x, int y, int size) {
    static int spinnerFrame = 0;
    const char* spinners[] = {"|", "/", "-", "\\"};
    
    M5.Display.setTextColor(Theme::PRIMARY);
    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.drawString(spinners[spinnerFrame % 4], x, y);
    
    spinnerFrame++;
}

void DisplayUtils::drawTemperature(int x, int y, float temp, float target, const String& label) {
    M5.Display.setTextColor(Theme::FG);
    M5.Display.setTextDatum(TL_DATUM);
    
    String tempStr = label + ": " + String(temp, 1) + "/" + String(target, 1) + "C";
    M5.Display.drawString(tempStr, x, y);
}

void DisplayUtils::drawProgressBar(int x, int y, int w, int h, float progress) {
    M5.Display.drawRect(x, y, w, h, Theme::FG);
    
    int fillWidth = (w - 2) * progress / 100;
    if (fillWidth > 0) {
        M5.Display.fillRect(x + 1, y + 1, fillWidth, h - 2, Theme::SUCCESS);
    }
}

void DisplayUtils::drawLoadingBox(const String& message) {
    // FIX: Draw loading box overlay for blocking operations
    int boxWidth = M5.Display.width();
    int boxHeight = 70;
    int boxX = 0;
    int boxY = (Display::HEIGHT - boxHeight) / 2;
    
    // Draw semi-transparent dark background for box
    M5.Display.setColor(Theme::SECONDARY);
    M5.Display.fillRect(boxX, boxY, boxWidth, boxHeight, Theme::BG);
    
    // Draw box border with primary color
    M5.Display.drawRect(boxX, boxY, boxWidth, boxHeight, Theme::PRIMARY);
    
    // Draw message text
    M5.Display.setTextColor(Theme::FG);
    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.setTextSize(1);
    M5.Display.drawString(message, Display::WIDTH / 2, boxY + 25);
    
    // Draw spinner animation
    drawSpinner(Display::WIDTH / 2, boxY + 50, 10);
    
    // Reset text datum for consistency
    M5.Display.setTextDatum(TL_DATUM);
}
