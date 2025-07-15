#include <Arduino.h>
#include <WebInterface.h>
#include <SystemID.h>
#include <WebDisplay.h>
#include <Settings.h>
#include <WiFiWrapper.h>
#include <passwords.h>
#include <loggingBase.h>
#include <WebLog.h>
#include <TimeManager.h>
#include <FillState.h>

TimeManager timeManager;

WiFiWrapper wifi(secret::ssid, secret::password);
WebInterface webInterface;


FillState fillState(8,6,9,10);// Touch pins for empty and full states
FillStateDisplay fillStateDisplay(&fillState);

void setup() {
   delay(100); // wait a moment for the system to stabilize
   fillState.begin();
   settings.begin();

  Serial.begin(115200);
  while (!Serial) {
    delay(100); // Wait for Serial to be ready
  }
  Serial.println("Starting system initialization...");

  gLogger->println("Starting WiFi connection...");
  wifi.begin();

  timeManager.begin();
  gLogger->println("Time manager initialized");

  systemID.begin();

  gLogger->println("System ID initialized: " + systemID.systemName());
  setLogger(&webLog);
  webLog.begin();
  webLog.mirrorToSerial = true; // Enable mirroring to Serial

  // Add displays to the web interface
  for (const auto& display : fillStateDisplay.getDisplays()) {
      webInterface.addDisplay(display.first, display.second);
  }

  webInterface.begin();
  gLogger->println("Web interface started");

  gLogger->println("Touch sensors initialized");
}

void loop() {
  wifi.loop();
  webInterface.loop();
  fillState.update();

  // Example usage of WebDisplay
  fillState.update();
  fillStateDisplay.update();
  
  delay(100); // Adjust as needed for your application
}
