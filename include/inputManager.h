#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <Arduino.h>

namespace inputManager {
    
    enum ButtonState {
        BUTTON_IDLE,
        BUTTON_PRESSED,
        BUTTON_LONG_PRESSED
    };
    
    void init();
    void update();
    
    // Query button states
    bool isButtonAPressed();
    bool isButtonBPressed();
    bool isButtonALongPressed();
    bool isButtonBLongPressed();
    
    void resetButtonStates();
    String getHelpText();
    
}

#endif
