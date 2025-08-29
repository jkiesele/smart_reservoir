#include <Arduino.h>
#include <SmartReservoir.h>


// If your build system prefers no dynamic initialization of std::vector, you can
// replace with std::array and construct vectors from them in setup.



//

#if CONFIG_IDF_TARGET_ESP32S2
//this is the lettuce tree on an ESP32-S2
SmartReservoir reservoir(/*TOUCH_PINS*/ {4, 5, 6},  /*FRACTIONS*/  {0.3f, 0.6f, 1.0f}, 7); // circulation pump on pin 8
#else
//this is the tomato reservoir on an ESP32-S3
SmartReservoir reservoir(/*TOUCH_PINS*/ {3, 4, 5, 6},  /*FRACTIONS*/  {0.1f, 0.5f, 0.73f, 1.0f}, -1); // no circulation pump
#endif

                         
void setup() {
  reservoir.begin();
  gLogger->print("Smart Reservoir System, software version: ");
  gLogger->println("Rev 2.0c");
}

void loop() {
  reservoir.loop();
}