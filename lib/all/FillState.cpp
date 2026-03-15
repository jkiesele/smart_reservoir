#include "FillState.h"
#include <LoggingBase.h>
#include <ReservoirSettings.h>

// ─────────────────────────────────── ReservoirFillState ─────────────────────────
ReservoirFillState::ReservoirFillState(const std::vector<uint8_t>& touchPins,
                                          const std::vector<float>&  fractions,
                                          ReservoirSettings* settings)
: fractions_(fractions),
  settings_(settings),
    printedWarning_(false) // Initialize warning flag
  {
    if (touchPins.empty()) {
        gLogger->println("ReservoirFillState: ERROR: at least one sensor required!");
    }
    if (fractions_.size() != touchPins.size()) {
        gLogger->println("ReservoirFillState: ERROR: pin & fraction vectors differ in length!");
    }
    // last fraction should be 1.0; warn if not
    if (!fractions_.empty() && (fabs(fractions_.back() - 1.0f) > 1e-3f)) {
        gLogger->println("ReservoirFillState: WARNING: top-sensor fraction is not 1.0");
    }
    // create touch sensors
    touchSensors_.reserve(touchPins.size());
    auto& thresholds = settings_->thsTouch;

    thresholds.ensureSize(touchPins.size());
    for (size_t i = 0; i < touchPins.size(); ++i) {
        uint8_t pin = touchPins[i];
        // TouchSensor(uint8_t pin, uint16_t threshold, uint16_t hysteresis = 200, uint8_t samples = 3, uint8_t nMovingAvg = 0);
        touchSensors_.emplace_back(pin, thresholds[i], TOUCH_HYSTERESIS, TOUCH_SAMPLES, TOUCH_NMOVINGAVG);
    }
}


void ReservoirFillState::begin()
{
    //iterate over touch sensors array and call begin on each 
    for (uint8_t i = 0; i < touchSensors_.size(); ++i) {
        touchSensors_[i].begin();
    }
}

void ReservoirFillState::update()
{
    const auto& thresholds = settings_->thsTouch;

    //reset thresholds
    for (size_t i = 0; i < touchSensors_.size(); ++i) {
        if(i < thresholds.value.size()) {
            touchSensors_[i].setThreshold(thresholds[i]); // update threshold from settings
        } else {
            gLogger->println("ReservoirFillState: WARNING: thresholds array size is smaller than sensor count, using default threshold for remaining sensors");
            touchSensors_[i].setThreshold(65535); // use max uint16 as fallback
        } 
    }

    // 1) read all sensors
    for (auto & sensor : touchSensors_) {
        sensor.update();
    }

    // 2) minimum guaranteed fill  (bottom-to-top)
    level_ = 0.0f;
    for (size_t i = 0; i < touchSensors_.size(); ++i) {
        if (touchSensors_[i].isActive()) {
            level_ = fractions_[i] * 100.0f;
        }
    }

    // 3) minimum guaranteed empty (top-1 down to bottom)
    emptyLevel_ = 0.0f;               // if top sensor is dry: unknown empty → 0 %
    for (int i = static_cast<int>(touchSensors_.size()) - 2; i >= 0; --i) {
        if (!touchSensors_[i].isActive()) {
            // how much is empty: fill fractions are increasing, but which fraction is empty now
            float emptyFraction = (1 - fractions_[i]);
            emptyLevel_ = emptyFraction * 100.0f;
            break;
        }
    }

    // 4) bookkeeping
    capacity_    = settings_->totalVolume;     // litres
    // temperature_ updated externally through temp sensor

    bool allgood = true;

    // 5) sanity checks
    if (level_ < 0.0f || level_ > 100.0f) {
        gLogger->println("ReservoirFillState: Invalid level: " + String(level_) + "%");
        level_ = 0.0f;
        allgood = false;
    }
    for (size_t i = 1; i < touchSensors_.size(); ++i) {
        if (touchSensors_[i].isActive() 
            && !touchSensors_[i-1].isActive()
            && !printedWarning_) {
            gLogger->println("ReservoirFillState: Sensor " + String(i+1) +
                             " active while sensor " + String(i) + " is inactive");
            printedWarning_ = true; // only print once
            allgood = false;
        }
    }
    if(allgood) {
        printedWarning_ = false; // reset warning flag if all is good
    }
}






// ─────────────────────────────────── FillStateDisplay ───────────────────────────

FillStateDisplay::FillStateDisplay(ReservoirFillState* fillState)
: fillState_(fillState),
  fillLevelDisplay_("Fill_Level", 1, fillState->settings()->totalVolume, " l")
{
    auto N = fillState_->sensorCount();
    touchDisplays_.reserve(N);
    for (size_t i = 0; i < N; ++i) {
        touchDisplays_.emplace_back("Touch_" + String(i+1), 1, 0);
    }
}

void FillStateDisplay::begin() { /* nothing yet */ }

void FillStateDisplay::update()
{
    auto N = fillState_->sensorCount();
    for (size_t i = 0; i < N; ++i) {
        touchDisplays_[i].update(fillState_->rawRead(i));
    }
    fillLevelDisplay_.setMaxVal(fillState_->capacity());
    fillLevelDisplay_.update(fillState_->litersFullMin());
}

std::vector<std::pair<String, WebDisplayBase*>>
FillStateDisplay::getDisplays()
{
    std::vector<std::pair<String, WebDisplayBase*>> out;
    for (size_t i = 0; i < touchDisplays_.size(); ++i) {
        out.emplace_back("Touch " + String(i+1) + " ADC value",
                         &touchDisplays_[i]);
    }
    out.emplace_back("Fill Level (min)", &fillLevelDisplay_);
    return out;
}