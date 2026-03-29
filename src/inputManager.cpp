#include "inputManager.h"
#include "M5Unified.h"
#include <Arduino.h>

namespace inputManager {
    
    // Button timing
    const uint32_t LONG_PRESS_THRESHOLD = 700; // milliseconds
    
    // Button state tracking
    struct ButtonTracker {
        bool isCurrentlyPressed = false;
        bool wasPressed = false;
        uint32_t pressStartTime = 0;
        bool longPressDetected = false;
    };
    
    static ButtonTracker buttonA;
    static ButtonTracker buttonB;

    void init() {
        log_i("Input Manager initialized");
    }

    void update() {
        M5.update();
        
        // Button A handling
        bool btnA_pressed = M5.BtnA.isPressed();
        if (btnA_pressed && !buttonA.isCurrentlyPressed) {
            if(buttonB.wasPressed){
                buttonB.wasPressed = false;
            }
            // Button just pressed
            buttonA.isCurrentlyPressed = true;
            buttonA.pressStartTime = millis();
            buttonA.longPressDetected = false;
            buttonA.wasPressed = false;
            log_d("BtnA pressed");
        } else if (btnA_pressed && buttonA.isCurrentlyPressed) {
            // Button held - check for long press
            if (!buttonA.longPressDetected && (millis() - buttonA.pressStartTime >= LONG_PRESS_THRESHOLD)) {
                buttonA.longPressDetected = true;
                buttonA.wasPressed = true;
                log_d("BtnA long press detected");
            }
        } else if (!btnA_pressed && buttonA.isCurrentlyPressed) {
            // Button just released
            buttonA.isCurrentlyPressed = false;
            if (!buttonA.longPressDetected) {
                // It was a short press
                buttonA.wasPressed = true;
            }
            log_d("BtnA released");
        }
        
        // Button B handling
        bool btnB_pressed = M5.BtnB.isPressed();
        if (btnB_pressed && !buttonB.isCurrentlyPressed) {
            if(buttonA.wasPressed){
                buttonA.wasPressed = false;
            }
            // Button just pressed
            buttonB.isCurrentlyPressed = true;
            buttonB.pressStartTime = millis();
            buttonB.longPressDetected = false;
            buttonB.wasPressed = false;
            log_d("BtnB pressed");
        } else if (btnB_pressed && buttonB.isCurrentlyPressed) {
            // Button held - check for long press
            if (!buttonB.longPressDetected && (millis() - buttonB.pressStartTime >= LONG_PRESS_THRESHOLD)) {
                buttonB.longPressDetected = true;
                buttonB.wasPressed = true;
                log_d("BtnB long press detected");
            }
        } else if (!btnB_pressed && buttonB.isCurrentlyPressed) {
            // Button just released
            buttonB.isCurrentlyPressed = false;
            if (!buttonB.longPressDetected) {
                // It was a short press
                buttonB.wasPressed = true;
            }
            log_d("BtnB released");
        }
    }

ButtonState getButtonA() {
    // Deprecated - not used
    return BUTTON_IDLE;
}

ButtonState getButtonB() {
    // Deprecated - not used
    return BUTTON_IDLE;
}

    bool isButtonAPressed() {
        if (buttonA.wasPressed && !buttonA.longPressDetected) {
            buttonA.wasPressed = false;
            log_d("isButtonAPressed() = true");
            return true;
        }
        return false;
    }

    bool isButtonBPressed() {
        if (buttonB.wasPressed && !buttonB.longPressDetected) {
            buttonB.wasPressed = false;
            log_d("isButtonBPressed() = true");
            return true;
        }
        return false;
    }

    bool isButtonALongPressed() {
        if (buttonA.wasPressed && buttonA.longPressDetected) {
            buttonA.wasPressed = false;
            log_d("isButtonALongPressed() = true");
            return true;
        }
        return false;
    }

    bool isButtonBLongPressed() {
        if (buttonB.wasPressed && buttonB.longPressDetected) {
            buttonB.wasPressed = false;
            log_d("isButtonBLongPressed() = true");
            return true;
        }
        return false;
    }

    void resetButtonStates() {
        // States are automatically managed
    }

    String getHelpText() {
        return "BtnA: Select/OK | BtnB: Back/Cancel | Long Press: Enter/Submit";
    }
}
