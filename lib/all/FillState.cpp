#include "FillState.h"
#include <LoggingBase.h>
#include <ReservoirSettings.h>

// ─────────────────────────────────── ReservoirFillState ─────────────────────────
ReservoirFillState::ReservoirFillState(const FillSensorConfig& config, ReservoirSettings& settings)
: 
  settings_(settings),
    printedWarning_(false) // Initialize warning flag
  {
    if (!config.isValid()) {
        gLogger->println("ReservoirFillState: ERROR: config not valid!");
        return;
    }
    //build sensors_ vector from config
    sensors_.reserve(config.points().size());
    for (const auto& p : config.points()) 
        sensors_.emplace_back(p.pin, p.fraction);

}


void ReservoirFillState::begin()
{
    //iterate over touch sensors array and call begin on each 
    for (size_t i = 0; i < sensors_.size(); ++i) {
        sensors_[i].sensor.begin();
    }
}

void ReservoirFillState::update()
{
    const auto& thresholds = settings_.thsTouch;

    //reset thresholds
    for (size_t i = 0; i < sensors_.size(); ++i) {
        auto & sensor = sensors_[i].sensor;
        if(i < thresholds.value.size()) {
            sensor.setThreshold(thresholds[i]); // update threshold from settings
        } else {
            gLogger->println("ReservoirFillState: WARNING: thresholds array size is smaller than sensor count, setting remaining sensors inactive");
            sensor.setThreshold(65535); // use max uint16 as fallback
        } 
    }

    // 1) read all sensors
    for (auto & sensor : sensors_) {
        sensor.sensor.update();
    }

    // 2) minimum guaranteed fill  (bottom-to-top)
    level_ = 0.0f;
    for (size_t i = 0; i < sensors_.size(); ++i) {
        if (sensors_[i].sensor.isActive()) {
            level_ = sensors_[i].fraction * 100.0f;
        }
    }

    // 3) minimum guaranteed empty (top-1 down to bottom)
    emptyLevel_ = 0.0f;               // if top sensor is dry: unknown empty → 0 %
    for (int i = static_cast<int>(sensors_.size()) - 2; i >= 0; --i) {
        const auto& sensor = sensors_[i].sensor;
        const auto& fraction = sensors_[i].fraction;
        if (!sensor.isActive()) {
            // how much is empty: fill fractions are increasing, but which fraction is empty now
            float emptyFraction = (1 - fraction);
            emptyLevel_ = emptyFraction * 100.0f;
            break;
        }
    }

    // 4) bookkeeping
    capacity_    = settings_.totalVolume;     // litres
    // temperature_ updated externally through temp sensor

    bool allgood = true;

    // 5) sanity checks
    if (level_ < 0.0f || level_ > 100.0f) {
        gLogger->println("ReservoirFillState: Invalid level: " + String(level_) + "%");
        level_ = 0.0f;
        allgood = false;
    }
    for (size_t i = 1; i < sensors_.size(); ++i) {
        if (sensors_[i].sensor.isActive() 
            && !sensors_[i-1].sensor.isActive()
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
  fillLevelDisplay_("Fill_Level", 1, fillState->settings().totalVolume, " l")
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
    auto reads = fillState_->rawReads();
    //check N against reads size for safety
    if(reads.size() != N) {
        gLogger->println("FillStateDisplay: WARNING: sensor count and raw reads size mismatch");
        return;
    }
    for (size_t i = 0; i < N; ++i) {
        touchDisplays_[i].update(reads[i]); //guaranteed to be in range cause of sensorCount
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