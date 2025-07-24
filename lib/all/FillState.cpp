#include "FillState.h"
#include <LoggingBase.h>
#include <Settings.h>

// ─────────────────────────────────── ReservoirFillState ─────────────────────────

ReservoirFillState::ReservoirFillState(const std::vector<uint8_t>& touchPins,
                                       const std::vector<float>&  fractions,
                                       uint32_t                   touchThreshold)
: touchSensors_(),
  fractions_(fractions)
  {
    if (touchPins.empty()) {
        gLogger->println("ReservoirFillState: ERROR: at least one sensor required!");
    }
    if (fractions_.size() != touchPins.size()) {
        gLogger->println("ReservoirFillState: ERROR: pin & fraction vectors differ in length!");
    }
    // last fraction should be 1.0; warn if not
    if (!fractions_.empty() && (abs(fractions_.back() - 1.0f) > 1e-3f)) {
        gLogger->println("ReservoirFillState: WARNING: top-sensor fraction is not 1.0");
    }
    // create touch sensors
    touchSensors_.reserve(touchPins.size());
    for (uint8_t pin : touchPins) {
        touchSensors_.emplace_back(pin, touchThreshold, TOUCH_HYSTERESIS);
    }
}

void ReservoirFillState::begin()
{
    for (auto& sensor : touchSensors_) {
        sensor.begin();
    }
}

void ReservoirFillState::update()
{
    const size_t N = touchSensors_.size();

    //reset thresholds
    for (auto& sensor : touchSensors_) {
        sensor.setThreshold(settings.thTouch); // update threshold from settings
    }

    // 1) read all sensors
    for (auto & sensor : touchSensors_) {
        sensor.update();
    }

    // 2) minimum guaranteed fill  (bottom-to-top)
    level_ = 0.0f;
    for (size_t i = 0; i < N; ++i) {
        if (touchSensors_[i].isActive()) {
            level_ = fractions_[i] * 100.0f;
        }
    }

    // 3) minimum guaranteed empty (top-1 down to bottom)
    emptyLevel_ = 0.0f;               // if top sensor is dry: unknown empty → 0 %
    for (int i = static_cast<int>(N) - 2; i >= 0; --i) {
        if (!touchSensors_[i].isActive()) {
            emptyLevel_ = 100.0f - fractions_[i] * 100.0f;
            break;
        }
    }

    // 4) bookkeeping
    capacity_    = settings.totalVolume;     // litres
    temperature_ = 20.0f;                    // default

    // 5) sanity checks
    if (level_ < 0.0f || level_ > 100.0f) {
        gLogger->println("ReservoirFillState: Invalid level: " + String(level_) + "%");
        level_ = 0.0f;
    }
    for (size_t i = 1; i < N; ++i) {
        if (touchSensors_[i].isActive() && !touchSensors_[i-1].isActive()) {
            gLogger->println("ReservoirFillState: Sensor " + String(i+1) +
                             " active while sensor " + String(i) + " is inactive");
        }
    }
}

// ─────────────────────────────────── FillStateDisplay ───────────────────────────

FillStateDisplay::FillStateDisplay(ReservoirFillState* fillState)
: fillState_(fillState),
  fillLevelDisplay_("Fill_Level", 1, settings.totalVolume, " l")
{
    const size_t N = fillState_->sensorCount();
    touchDisplays_.reserve(N);
    for (size_t i = 0; i < N; ++i) {
        touchDisplays_.emplace_back("Touch_" + String(i+1), 1, 0);
    }
}

void FillStateDisplay::begin() { /* nothing yet */ }

void FillStateDisplay::update()
{
    const size_t N = fillState_->sensorCount();
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