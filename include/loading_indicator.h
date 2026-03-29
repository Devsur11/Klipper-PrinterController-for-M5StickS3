#ifndef LOADING_INDICATOR_H
#define LOADING_INDICATOR_H

#include <Arduino.h>
#include <mutex>

/**
 * Thread-safe global loading state manager
 * Tracks when API requests are in progress and displays a non-blocking loading popup
 */
class LoadingIndicator {
public:
    static LoadingIndicator& getInstance();
    
    // Thread-safe setters/getters
    void setLoading(bool loading, const String& message = "Loading...");
    bool isLoading() const;
    String getMessage() const;
    void clear();
    
    // Get the current spinner frame for animation
    static const char* getSpinnerFrame();
    
private:
    LoadingIndicator();
    
    bool loading;
    String message;
    mutable std::mutex stateMutex;
    static int spinnerFrame;
    static const char* spinners[4];
};

#endif
