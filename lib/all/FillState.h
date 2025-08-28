#ifndef FILLSTATE_H
#define FILLSTATE_H

#include <Arduino.h>
#include <vector>
#include <ReservoirSettings.h>
#include <WebDisplay.h>
#include <SharedDataFormats.h>
#include <TouchSensor.h>

#define TOUCH_HYSTERESIS 200 // default hysteresis for touch sensors
#define TOUCH_SAMPLES    3   // default number of samples for touch sensors

/**
 * ReservoirFillState
 * ------------------
 * Derives from SharedDataFormats::ReservoirInfo and turns a list of capacitive
 * touch sensors into a bounded fill-level estimate.
 *
 *  • touchPins[i]      — GPIO number of sensor i (bottom-to-top)
 *  • fractions[i]      — geometric height of sensor i  (0.0 … 1.0)
 *                        The top sensor should be 1.0; others < 1.0.
 *  • touchThreshold    — raw ADC threshold used for ALL sensors
 */
class ReservoirFillState : public SharedDataFormats::ReservoirInfo
{
public:
    ReservoirFillState(const std::vector<uint8_t>& touchPins,
                       const std::vector<float>&  fractions,
                        ReservoirSettings* settings);

    void begin();          ///< call from setup()
    void update();         ///< call periodically

    const ReservoirSettings* settings() const { return settings_; }

    // setters
    const std::vector<TouchSensor>& getTouchPins() const { return touchSensors_; }
    std::vector<TouchSensor>& getTouchPins() { return touchSensors_; }
    void setFractions(const std::vector<float>& fractions) { fractions_ = fractions; }

    // convenience helpers  ----------------------------------------------
    bool  isEmpty()  const { return level_ < 5.0f; }
    bool  isFull()   const { return level_ >= 100.0f; }

    // sensor access ------------------------------------------------------
    size_t            sensorCount()            const { return touchSensors_.size(); }
    uint32_t          rawRead(size_t idx)      const { return touchSensors_[idx].lastValue(); }
    std::vector<uint32_t> rawReads()    const { 
        std::vector<uint32_t> reads;
        reads.reserve(touchSensors_.size());
        for (const auto& ts : touchSensors_) {
            reads.push_back(ts.lastValue());
        }
        return reads;
    }

private:
    std::vector<TouchSensor>  touchSensors_;
    std::vector<float>    fractions_;   ///< same size as touchPins_, ascending
    ReservoirSettings* settings_;
    bool printedWarning_; ///< true if we printed a warning about sensor count
};

/**
 * FillStateDisplay
 * ----------------
 * A thin wrapper that exposes WebDisplay objects for the raw ADC readings
 * and for the calculated fill level.  It builds everything dynamically
 * from the sensor count.
 */
class FillStateDisplay
{
public:
    explicit FillStateDisplay(ReservoirFillState* fillState);

    void begin();   ///< optional (does nothing for now)
    void update();

    /// Returns references to all display objects for easy registration
    std::vector<std::pair<String, WebDisplayBase*>> getDisplays();

private:
    ReservoirFillState*               fillState_;
    std::vector<WebDisplay<uint32_t>> touchDisplays_;
    WebBarDisplay<float>              fillLevelDisplay_;
};

#endif // FILLSTATE_H