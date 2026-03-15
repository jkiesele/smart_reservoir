#include <Arduino.h>
#include <SmartReservoir.h>
#include "FillSensorConfig.h"


#define IS_LETTUCE_TREE
//#define IS_MAIN_RESERVOIR
//#define IS_TOMATO_RESERVOIR
//#define IS_CUCUMBER_RESERVOIR
//#define IS_STRAWBERRY_NFT

#ifdef IS_LETTUCE_TREE
// lettuce tree config, adjust pins and fractions as needed for your setup
FillSensorConfig config = {
    {4, 0.3f}, // bottom sensor at pin 4, 30% fill
    {5, 0.6f}, // middle sensor at pin 5, 60% fill
    {6, 1.0f}  // top sensor at pin 6, 100% fill
};
uint8_t circulationPumpPin = 7; // circulation pump on pin 7
#elif defined(IS_MAIN_RESERVOIR)
// main reservoir config (TBI)
#elif defined(IS_TOMATO_RESERVOIR)
// tomato reservoir config (TBI)
#elif defined(IS_CUCUMBER_RESERVOIR)
// cucumber reservoir config (TBI)
#elif defined(IS_STRAWBERRY_NFT)
// strawberry NFT config (TBI)
#else
#error "Please define a reservoir configuration (e.g. IS_LETTUCE_TREE) and adjust the FillSensorConfig and circulationPumpPin accordingly."
#endif


SmartReservoir reservoir(config, circulationPumpPin);

                         
void setup() {
  reservoir.begin();
  gLogger->print("Smart Reservoir System, software version: ");
  gLogger->println("Rev 2.0k");
}

void loop() {
  reservoir.loop();
}