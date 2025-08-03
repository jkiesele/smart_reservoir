#include <Arduino.h>
#include <SmartReservoir.h>


// If your build system prefers no dynamic initialization of std::vector, you can
// replace with std::array and construct vectors from them in setup.

SmartReservoir reservoir(/*TOUCH_PINS*/ {3, 4, 5, 6},
                         /*FRACTIONS*/  {0.1f, 0.5f, 0.73f, 1.0f},
                         -1); // no circulation pump
                         
void setup() {
  reservoir.begin();
}

void loop() {
  reservoir.loop();
}