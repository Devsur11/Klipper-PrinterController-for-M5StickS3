#include "loading_indicator.h"

// Static member initialization
int LoadingIndicator::spinnerFrame = 0;
const char* LoadingIndicator::spinners[4] = {"|", "/", "-", "\\"};

LoadingIndicator& LoadingIndicator::getInstance() {
    static LoadingIndicator instance;
    return instance;
}

LoadingIndicator::LoadingIndicator() 
    : loading(false), message("Loading...") {}

void LoadingIndicator::setLoading(bool isLoading, const String& msg) {
    std::lock_guard<std::mutex> lock(stateMutex);
    loading = isLoading;
    message = msg;
}

bool LoadingIndicator::isLoading() const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return loading;
}

String LoadingIndicator::getMessage() const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return message;
}

void LoadingIndicator::clear() {
    std::lock_guard<std::mutex> lock(stateMutex);
    loading = false;
    message = "Loading...";
}

const char* LoadingIndicator::getSpinnerFrame() {
    const char* frame = spinners[spinnerFrame % 4];
    spinnerFrame++;
    return frame;
}
