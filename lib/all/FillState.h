#ifndef FILLSTATE_H
#define FILLSTATE_H

#include <Arduino.h>
#include <vector>
#include <Settings.h>
#include <WebDisplay.h>
#include <SharedDataFormats.h>

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
                       uint32_t                   touchThreshold);

    void begin();          ///< call from setup()
    void update();         ///< call periodically

    // setters
    void setTouchThreshold(uint32_t threshold) { threshold_ = threshold; }
    void setFractions(const std::vector<float>& fractions) { fractions_ = fractions; }

    // convenience helpers  ----------------------------------------------
    bool  isEmpty()  const { return level_ < 5.0f; }
    bool  isFull()   const { return level_ >= 100.0f; }

    // sensor access ------------------------------------------------------
    size_t            sensorCount()            const { return rawValues_.size(); }
    uint32_t          rawRead(size_t idx)      const { return rawValues_[idx]; }
    const std::vector<uint32_t>& rawReads()    const { return rawValues_; }

private:
    std::vector<uint8_t>  touchPins_;
    std::vector<float>    fractions_;   ///< same size as touchPins_, ascending
    uint32_t              threshold_;   ///< common ADC threshold
    std::vector<uint32_t> rawValues_;   ///< updated each cycle
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