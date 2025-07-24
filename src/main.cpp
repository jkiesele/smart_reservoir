#include <Arduino.h>
#include <WebInterface.h>
#include <SystemID.h>
#include <WebDisplay.h>
#include <Settings.h>
#include <WiFiWrapper.h>
#include <passwords.h>
#include <loggingBase.h>
#include <TimeProviderBase.h>
#include <WebLog.h>
#include <TimeManager.h>
#include <FillState.h>
#include <DebugLED.h>
#include <TCPMessenger.h>
#include <EncryptionHandler.h>
#include <WebSettings.h>
#include <Scheduler.h>

TimeManager timeManager;

WiFiWrapper wifi(secret::ssid, secret::password);
WebInterface webInterface;

ReservoirFillState fillState({3,4,5,6}, {0.1, 0.5, 0.73, 1.0}, 24000); // Touch pins for empty and full states
FillStateDisplay fillStateDisplay(&fillState);

EncryptionHandler enc(&secret::encryption_keys);  
TCPMessenger tcpMessenger(&enc);

DebugLED led; // Use built-in LED for debugging


Scheduler scheduler; // Global scheduler for periodic tasks

void send(){
   if (((String)settings.injUrl).length() > 0) {
       led.setBlue(); // Set the LED to blue while sending
       auto res = tcpMessenger.sendToHost(
           fillState, 0,//obj, channel
           settings.injUrl);
       if (res != TCPMSG_OK) {
           gLogger->println("Failed to send fill state: " + String(res));
           led.setRed(); // Set the LED to red on failure
       }
       else {
           led.setOff(); // Set the LED to green on success
       }
   }
   else {
       led.setColor(10, 0, 10); // Set the LED to purple if no URL is set
   }
}


void setup() {

   SettingsBlockBase::kSettingsPassword = secret::hydroPassword;

   delay(100); // wait a moment for the system to stabilize
   led.begin(); // Initialize the debug LED
   led.setRed(); // Set the LED to red for debugging
   fillState.begin();
   settings.begin();

  Serial.begin(115200);
  
  Serial.println("Starting system initialization...");

  webLog.begin();
  setLogger(&webLog);

  gLogger->println("Starting WiFi connection...");
  wifi.begin();
  delay(1000); // Allow some time for WiFi to connect
  while(!wifi.isStateReady()) {
      gLogger->println("Waiting for WiFi to be ready...");
      delay(1000);
  }

  timeManager.begin();

  while(!timeManager.isSynced()) {
      gLogger->println("Waiting for time synchronization...");
      delay(1000);
  }
  setTimeProvider(&timeManager);
  gLogger->println("Time manager initialized");

  systemID.begin();

  gLogger->println("System ID initialized: " + systemID.systemName());
  webLog.mirrorToSerial = true; // Enable mirroring to Serial

  // Add displays to the web interface
  for (const auto& display : fillStateDisplay.getDisplays()) {
      webInterface.addDisplay(display.first, display.second);
  }

  webInterface.begin();
  gLogger->println("Web interface started");

  gLogger->println("Touch sensors initialized");

  tcpMessenger.beginServer(); // Start TCP server on port 12345

  gLogger->println("TCP Messenger server started on port 12345");


  ///now set up scheduler to update the fill state every second
  scheduler.addTimedTask([&]() {
        fillState.update();
        fillStateDisplay.update();
  }, 1000,
     true, // Repeat every second
     1000); // Interval of 1 second)

  // after that add a task to send the fill state every 10 seconds
  scheduler.addTimedTask(
        [&]() {
            send();
    }, 10000, true, 10000); // Repeat every 10 seconds

  led.setGreen(); // Set the LED to green to indicate successful setup
  delay(1000); // Allow time for the LED to show green before starting the loop
  led.setOff(); // Turn off the LED after setup
}

//send status every second for now

void loop() {
  wifi.loop();
  scheduler.loop(); // Run the scheduler to execute periodic tasks
  webInterface.loop(); // Handle web interface updates
}
