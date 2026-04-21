#include <Arduino.h>
#include <SmartReservoir.h>
#include "FillSensorConfig.h"


#define IS_TEST_RESERVOIR
//#define IS_LETTUCE_TREE
//#define IS_STRAWBERRY_NFT
//#define IS_MAIN_RESERVOIR
//#define IS_TOMATO_RESERVOIR
//#define IS_CUCUMBER_RESERVOIR

#define REWRITE_SYSTEM_NAME true // set to false after first flash to avoid overwriting system name and other settings
#define REVISION "Rev 3.0c"

#ifdef IS_TEST_RESERVOIR
// test reservoir config, adjust pins and fractions as needed for your setup
FillSensorConfig config = {
    {1, 0.1f},   // bottom sensor at pin 4, 10% fill
    {4, 0.25f}, // bottom sensor at pin 4,
    {5, 0.5f},  // middle sensor at pin 5,
    {6, 0.75f}, // upper-middle sensor at pin 6,
    {7, 1.0f}   // top sensor at pin 7,
};
//no circulation pump for test reservoir
int8_t circulationPumpPin = -1; // set to -1 if no pump
int8_t temperaturePin = -1; // optional temperature sensor on pin 2
const uint8_t ledPin = 48;
const String systemName = "test-reservoir";

#elif defined(IS_LETTUCE_TREE)
// lettuce tree config, adjust pins and fractions as needed for your setup
FillSensorConfig config = {
    {1, 0.2f}, // 
    {2, 0.25f}, //
    {3, 0.5f},  // 
    {4, 0.75f}, //
    {5, 1.0f}   //
};
uint8_t circulationPumpPin = 7; // circulation pump on pin 7
int temperaturePin = -1; // optional temperature sensor on pin 9
const uint8_t ledPin = 21;
const String systemName = "lettuce-tree";

#elif defined(IS_MAIN_RESERVOIR)
FillSensorConfig config = {
    {1, 0.125f},
    {2, 0.25f},
    {3, 0.375f},
    {4, 0.5f},
    {5, 0.625f},
    {6, 0.75f},
    {7, 0.875f},
    {8, 1.0f}
};
int circulationPumpPin = -1; // no circulation pump for main reservoir
int temperaturePin = 13; // optional temperature sensor on pin 9
const uint8_t ledPin = 21;
const String systemName = "main-reservoir";
// main reservoir config (TBI)
#elif defined(IS_TOMATO_RESERVOIR)
// tomato reservoir config, adjust pins and fractions as needed for your setup
FillSensorConfig config = {
    {3, 0.1f}, // bottom sensor at pin 4, 25% fill
    {4, 0.5f},  // middle sensor at pin 5, 50% fill
    {5, 0.75f}, // upper-middle sensor at pin 6, 75% fill
    {6, 1.0f}   // top sensor at pin 7, 100% fill
};
int circulationPumpPin = -1; // circulation pump on pin 8
int temperaturePin = -1; // optional temperature sensor on pin 2
const uint8_t ledPin = 21; //no effect as it's an S2
const String systemName = "tomato-reservoir";

#elif defined(IS_CUCUMBER_RESERVOIR)
// cucumber reservoir config (TBI)
#elif defined(IS_STRAWBERRY_NFT)
// strawberry NFT config (TBI)
#else
#error "Please define a reservoir configuration (e.g. IS_LETTUCE_TREE) and adjust the FillSensorConfig and circulationPumpPin accordingly."
#endif


SmartReservoir reservoir(config, circulationPumpPin, temperaturePin, ledPin);

                         
void setup() {
  delay(1000); // allow time for psu to stabilise

  systemID.begin();
  // Enforce compile-time hardware identity; will skip NVS write if unchanged.
  systemID.setSystemName(systemName); 

  reservoir.begin();
  gLogger->print("Smart Reservoir System, software version: ");
  gLogger->println(REVISION);
}

void loop() {
  reservoir.loop();
}