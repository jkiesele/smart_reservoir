#pragma once

#include <vector>
#include <cstdint>

// Arduino/project headers
#include <Arduino.h>
#include <BasicWebInterface.h>
#include <SystemID.h>
#include <WebDisplay.h>
#include <ReservoirSettings.h>
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
#include <WebOTAUpload.h>

// The class that encapsulates your sketch behavior.
class SmartReservoir {
public:
  SmartReservoir(const std::vector<uint8_t>& touchPins,
                 const std::vector<float>&  fractions,
                 int                       circulationPumpPin = -1);

  // Replaces Arduino setup()
  void begin();

  // Optional: body of Arduino loop()
  void loop();

private:
  // ---- Configuration copied from ctor args
  std::vector<uint8_t> touchPins_;
  std::vector<float>   fractions_;
  int                 circulationPumpPin_{-1};

  // ---- Former globals (now members)
  ReservoirSettings   settings_;
  CirculationPumpSettings circPumpSettings_; // if hasCirculationPump_ is true
  ReservoirFillState  fillState_;          // depends on settings_ and ctor args
  FillStateDisplay    fillStateDisplay_;   // depends on fillState_
  EncryptionHandler   enc_;
  TCPMessenger        tcpMessenger_;
  DebugLED            led_;
  Scheduler           scheduler_;
  TimeManager         timeManager_;
  WiFiWrapper         wifi_;               // constructed with secrets
  BasicWebInterface        webInterface_;

  // ---- Helpers (former free functions)
  void sendState();
  void scheduleCirculationPump();
  void safeTurnOnCirculationPump();
  void turnOffCirculationPump();

  const uint8_t pwmChannel_ = 0;
  const uint8_t pwmRes_ = 8; //bits

  WebOTAUpload otaUpload_; // URL for OTA updates
};