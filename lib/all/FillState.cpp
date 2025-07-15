#include <FillState.h>
#include <LoggingBase.h>

void FillState::begin() {
    pinMode(touch5percentPin_, INPUT);
    pinMode(touch50percentPin_, INPUT);
    pinMode(touch75percentPin_, INPUT);
    pinMode(touch100percentPin_, INPUT);
}

void FillState::update() {
    //reads level
    level_ = 0.0f;
    if (touchRead(touch5percentPin_) > settings.th5percent)
        level_ = 5.0f;
    if (touchRead(touch50percentPin_) > settings.th50percent)
        level_ = 50.0f;
    if (touchRead(touch75percentPin_) > settings.th75percent)
        level_ = 75.0f;
    if (touchRead(touch100percentPin_) > settings.th100percent)
        level_ = 100.0f;
    else
        level_ = 0.0f; // no touch detected, reset to 0%
    //now sanity checks, if e.g. 50% is detected, but 5% is not, then sth is wrong and we should print an error
    if (level_ >= 50.0f && touchRead(touch5percentPin_) < settings.th5percent) {
        gLogger->println("Warning: 50% detected, but 5% not detected! Check sensor calibration.");
    }
    if (level_ >= 75.0f && touchRead(touch50percentPin_) < settings.th50percent) {
        gLogger->println("Warning: 75% detected, but 50% not detected! Check sensor calibration.");
    }
    if (level_ >= 100.0f && touchRead(touch75percentPin_) < settings.th75percent) {
        gLogger->println("Warning: 100% detected, but 75% not detected! Check sensor calibration.");
    }
    
}