#include <Arduino.h>
#include <SmartReservoir.h>


// Provide the same values you had as globals:
static const std::vector<uint8_t> TOUCH_PINS   = {3, 4, 5, 6};
static const std::vector<float>   FRACTIONS    = {0.1f, 0.5f, 0.73f, 1.0f};
static const uint32_t             TOUCH_THRESH = 24000;

// If your build system prefers no dynamic initialization of std::vector, you can
// replace with std::array and construct vectors from them in setup.

SmartReservoir reservoir(TOUCH_PINS, FRACTIONS, TOUCH_THRESH,
                         /*hasCirculationPump=*/false); // or true, as needed

void setup() {
  reservoir.begin();
}

void loop() {
  reservoir.loop();
}