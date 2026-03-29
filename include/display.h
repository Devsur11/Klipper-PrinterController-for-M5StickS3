#ifndef DISPLAY_H
#define DISPLAY_H

#include <M5Unified.h>

// Theme colors (RGB565 format)
namespace Theme {
    static constexpr uint16_t PRIMARY = 0x1E7F;        // Dodger Blue (30, 144, 255 -> 565)
    static constexpr uint16_t SECONDARY = 0x2104;      // Royal Blue (65, 105, 225 -> 565)
    static constexpr uint16_t ACCENT = 0xF923;         // Tomato Red (255, 99, 71 -> 565)
    static constexpr uint16_t SUCCESS = 0x2FA8;        // Emerald Green (46, 204, 113 -> 565)
    static constexpr uint16_t WARNING = 0xFBE0;        // Orange (243, 156, 18 -> 565)
    static constexpr uint16_t DANGER = 0xE94B;         // Alizarin Red (231, 76, 60 -> 565)
    static constexpr uint16_t BG = 0x0000;             // Black
    static constexpr uint16_t FG = 0xFFFF;             // White
    static constexpr uint16_t MUTED = 0x8410;          // Gray
}

// Display dimensions
namespace Display {
    static constexpr int WIDTH = 135;
    static constexpr int HEIGHT = 240;
    static constexpr int HEADER_HEIGHT = 20;
    static constexpr int FOOTER_HEIGHT = 20;
    static constexpr int CONTENT_HEIGHT = HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT;
}

// UI Components
class UIComponent {
public:
    virtual ~UIComponent() = default;
    virtual void draw() = 0;
    virtual bool handlePress(int x, int y) { return false; }
};

// Button
class Button : public UIComponent {
public:
    Button(int x, int y, int w, int h, const String& label);
    void draw() override;
    bool handlePress(int x, int y) override;
    bool isPressed() const { return pressed; }
    void setCallback(std::function<void()> cb) { callback = cb; }
    
private:
    int x, y, w, h;
    String label;
    bool pressed;
    std::function<void()> callback;
};

// ProgressBar
class ProgressBar : public UIComponent {
public:
    ProgressBar(int x, int y, int w, int h);
    void draw() override;
    void setValue(float val) { value = constrain(val, 0, 100); }
    
private:
    int x, y, w, h;
    float value;
};

// Header
class Header {
public:
    Header(const String& title);
    void draw();
    void setTitle(const String& title) { this->title = title; }
    
private:
    String title;
};

// Footer
class Footer {
public:
    Footer(const String& leftText = "", const String& rightText = "");
    void draw();
    void setLeftText(const String& text) { leftText = text; }
    void setRightText(const String& text) { rightText = text; }
    
private:
    String leftText;
    String rightText;
};

// Utility functions
namespace DisplayUtils {
    void drawRect(int x, int y, int w, int h, uint16_t color, bool fill = false);
    void drawText(int x, int y, const String& text, uint16_t color = Theme::FG);
    void drawCenteredText(int y, const String& text, uint16_t color = Theme::FG);
    void drawSpinner(int x, int y, int size);
    void drawTemperature(int x, int y, float temp, float target, const String& label);
    void drawProgressBar(int x, int y, int w, int h, float progress);
    void drawLoadingBox(const String& message = "Loading...");  // FIX: loading overlay for blocking operations
}

#endif
