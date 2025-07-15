
#include <Arduino.h>
#include <Settings.h>
#include <WebDisplay.h>

class FillState {
public:
    FillState(
        uint8_t touch5percentPin,
        uint8_t touch50percentPin,
        uint8_t touch75percentPin, // default pin for empty state
        uint8_t touch100percentPin // default pin for full state
    ) : touch5percentPin_(touch5percentPin), touch50percentPin_(touch50percentPin),
        touch75percentPin_(touch75percentPin), touch100percentPin_(touch100percentPin) {}

        

    void begin();

    void update();

    bool isEmpty() const {
        return level_ < 5.0f;
    }
    bool isFull() const {
        return level_ >= 100.0f;
    }
    float level() const {
        return level_;
    }

    uint32_t touch5percentRead() const {
        return touchRead(touch5percentPin_);
    }
    uint32_t touch50percentRead() const {
        return touchRead(touch50percentPin_);
    }
    uint32_t touch75percentRead() const {
        return touchRead(touch75percentPin_);
    }
    uint32_t touch100percentRead() const {
        return touchRead(touch100percentPin_);
    }

private:
    //pins
    uint8_t touch5percentPin_;
    uint8_t touch50percentPin_;
    uint8_t touch75percentPin_;
    uint8_t touch100percentPin_;
    
    float level_;
};


class FillStateDisplay {
    public:
    FillStateDisplay(FillState *fillState) : fillState_(fillState),
    touch5percentDisplay_("Touch_5", 1, 0),
    touch50percentDisplay_("Touch_50", 1, 0),
    touch75percentDisplay_("Touch_75", 1, 0),
    touch100percentDisplay_("Touch_100", 1, 0),
    fillLevelDisplay_("Fill_Level", 1, 0.0f)
    {}

    void update() {
        touch5percentDisplay_.update(fillState_->touch5percentRead());
        touch50percentDisplay_.update(fillState_->touch50percentRead());
        touch75percentDisplay_.update(fillState_->touch75percentRead());
        touch100percentDisplay_.update(fillState_->touch100percentRead());
        fillLevelDisplay_.update(fillState_->level());
    }
    std::vector<std::pair<String, WebDisplayBase*>> getDisplays()  {
        return {
            {"Touch 5% adc value", &touch5percentDisplay_},
            {"Touch 50% adc value", &touch50percentDisplay_},
            {"Touch 75% adc value", &touch75percentDisplay_},
            {"Touch 100% adc value", &touch100percentDisplay_},
            {"Fill Level", &fillLevelDisplay_}
        };
    }

private:
    FillState *fillState_;
    WebDisplay<uint32_t> touch5percentDisplay_;
    WebDisplay<uint32_t> touch50percentDisplay_;
    WebDisplay<uint32_t> touch75percentDisplay_;
    WebDisplay<uint32_t> touch100percentDisplay_;
    WebBarDisplay<float> fillLevelDisplay_;
};