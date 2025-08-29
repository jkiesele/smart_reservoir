#include "SmartReservoir.h"

// NOTE: webLog, gLogger, systemID, setLogger, setTimeProvider are assumed
// to be provided by the included project headers (as in your original sketch).

SmartReservoir::SmartReservoir(const std::vector<uint8_t>& touchPins,
                               const std::vector<float>&  fractions,
                               int                       circulationPumpPin,
                               int                       temperaturePin)
: touchPins_(touchPins),
  fractions_(fractions),
  circulationPumpPin_(circulationPumpPin),
  settings_(touchPins.size()), // pass number of pads to settings
  circPumpSettings_(), 
  // Construct fill state using ctor-exposed parameters and pointer to settings_
  fillState_(touchPins_, fractions_, &settings_),//after settings_ is constructed!
  fillStateDisplay_(&fillState_),
  enc_(&secret::encryption_keys),
  tcpMessenger_(&enc_),
  led_(),
  scheduler_(),
  timeManager_(),
  wifi_(secret::ssid, secret::password),
  webInterface_(),
  otaUpload_(secret::otaPassword)
{
  // Any global init-at-declaration from the sketch that isn't tied to hardware
  // can be placed here if needed.
  if (temperaturePin >= 0) {
      oneWirep_ = new OneWire(temperaturePin);
      tempsens_.setOneWire(oneWirep_);
  }
}

void SmartReservoir::begin() {
  // Mirrors original setup()

  SettingsBlockBase::kSettingsPassword = secret::hydroPassword;

  delay(100);
  led_.begin();
  led_.setRed();

  fillState_.begin();
  settings_.begin();
  if(circulationPumpPin_>=0) {
    circPumpSettings_.begin();
    //set up output GPIO
    pinMode(circulationPumpPin_, OUTPUT);
    digitalWrite(circulationPumpPin_, LOW); // ensure pump is off initially
  }

  Serial.begin(115200);
  Serial.println("Starting system initialization...");


  if(oneWirep_) {
      tempsens_.begin();
      gLogger->println("Temperature sensor initialized");
  }

  webLog.begin();
  setLogger(&webLog);

  gLogger->println("Starting WiFi connection...");
  wifi_.begin();
  delay(1000);
  while (!wifi_.isStateReady()) {
    gLogger->println("Waiting for WiFi to be ready...");
    delay(1000);
  }

  timeManager_.begin();
  while (!timeManager_.isSynced()) {
    gLogger->println("Waiting for time synchronization...");
    delay(1000);
  }
  setTimeProvider(&timeManager_);
  gLogger->println("Time manager initialized");

  systemID.begin();
  gLogger->println("System ID initialized: " + systemID.systemName());

  webLog.mirrorToSerial = true;

  // Add displays to the web interface
  for (const auto& display : fillStateDisplay_.getDisplays()) {
    webInterface_.addDisplay(display.first, display.second);
  }
  // Add settings display
  webInterface_.addSettings("Reservoir Settings", &settings_);
  if (circulationPumpPin_>=0) {
      webInterface_.addSettings("Circulation Pump Settings", &circPumpSettings_);
  }

  webInterface_.addWebItem(&otaUpload_); // Add OTA upload handler

  webInterface_.begin();
  gLogger->println("Web interface started");

  gLogger->println("Touch sensors initialized");

  tcpMessenger_.beginServer(); // Starts TCP server (as before)
  gLogger->println("TCP Messenger server started on port 12345");

  // Scheduler: update fill state every second
  scheduler_.addTimedTask([this]() {
      fillState_.update();
      fillStateDisplay_.update();
    },
    1000,  // first delay
    true,  // repeat
    1000   // interval
  );

  //update temperature every 10 minutes
  scheduler_.addTimedTask([this]() {
      updateTemperature();
    },
    5*SECOND,  // first delay five seconds after start
    true,  // repeat
    10*MINUTE   // interval
  );

  // Scheduler: send state every 10 seconds
  scheduler_.addTimedTask([this]() {
      sendState();
    },
    10000, // first delay 10 seconds after start (this should be the last of the initial tasks to run)
    true,  // repeat
    10000  // interval
  );

  if (circulationPumpPin_ >= 0) {
    scheduleCirculationPump();
  }

  //init pwm for circulation pump if needed
  ledcSetup(pwmChannel_, //channel
    5000, //freq
    pwmRes_); //resolution bits
  ledcAttachPin(circulationPumpPin_, //pin
    pwmChannel_);//channel

  led_.setGreen();
  delay(1000);
  led_.setOff();
  //all good
  otaUpload_.markAppValid(); // prevent rollback on next boot
}

void SmartReservoir::loop() {
  wifi_.loop();
  scheduler_.loop();
  webInterface_.loop();
}

void SmartReservoir::sendState() {
  if (((String)settings_.injUrl).length() > 0) {
    led_.setBlue(); // sending
    auto res = tcpMessenger_.sendToHost(
        fillState_, /*channel*/ 0, settings_.injUrl);

    if (res != TCPMSG_OK) {
      gLogger->println("Failed to send fill state: " + String(res));
      led_.setRed();
    } else {
      led_.setOff(); // success (same behavior as original)
    }
  } else {
    // No URL set → purple
    led_.setColor(10, 0, 10);
  }
}

void SmartReservoir::scheduleCirculationPump(){
    if(circulationPumpPin_<0) 
       return;

    int periodMin = circPumpSettings_.circPDay;
    // if this is set to zero (at the moment), just schedule another check in 1 minute and return, don't run the pump
    // this allows to disable the pump from the web interface without changing the code
    if (periodMin <= 0) {
        scheduler_.addTimedTask([this]() {
            scheduleCirculationPump();
        }, 1 * MINUTE, false); // don't repeat
        return;
    }
    // it is already ensured that the circulation period is at least as long as the time the pump is on during day or night
    int runMin = circPumpSettings_.circTDay; 

    if(timeManager_.isNightTime()) {
        runMin = circPumpSettings_.circTNight;
    }
    periodMin -= runMin; // ensure period is time between end of one run and start of next

    //schedule
    scheduler_.addTimedTask([this, runMin]() {
        // turn off after timeMS
        if(runMin <= 0){
            turnOffCirculationPump(); // somewhat obsolete but safer
            scheduleCirculationPump(); // this is safe in the scheduler implementation!
            return;
        }
        safeTurnOnCirculationPump();

        scheduler_.addTimedTask([this]() {
            turnOffCirculationPump();
            // Reschedule the next circulation pump run
            // the scheduler can deal with nested tasks and will run them in order
            scheduleCirculationPump();
        }, 
        TimeManager::minToMs(runMin), 
        false); // do not repeat

    }, 
    TimeManager::minToMs(periodMin), 
    false); // don't repeat, self-reschedule

}

  void SmartReservoir::safeTurnOnCirculationPump(){
    if(circulationPumpPin_<0) 
       return;
    //check if fill state is high enough
    if (fillState_.litersFullMin() < circPumpSettings_.minLevel){
        gLogger->println("Circulation pump not turned ON: fill level too low");
        return;
    }
    int duty = map(circPumpSettings_.dutyCycle, 0, 100, 0, (1 << pwmRes_) - 1);
    // change PWM frequency if needed
    static int currentFreq = 5000;
    if (currentFreq != circPumpSettings_.pwmFreq) {
        ledcSetup(pwmChannel_, circPumpSettings_.pwmFreq, pwmRes_);
        currentFreq = circPumpSettings_.pwmFreq;
    }
    ledcWrite(pwmChannel_, duty);
    // gLogger->println("Circulation pump turned ON with duty cycle " + String(circPumpSettings_.dutyCycle) + "%");
    
  }
  void SmartReservoir::turnOffCirculationPump(){
    if(circulationPumpPin_<0) 
       return;
    ledcWrite(pwmChannel_, 0);
    // gLogger->println("Circulation pump turned OFF");
  }

  void SmartReservoir::updateTemperature() {
      if (!oneWirep_) return; // No sensor configured

      tempsens_.requestTemperatures(); // Send the command to get temperatures
      float tempC = tempsens_.getTempCByIndex(0); // Get temperature in Celsius
      if (tempC == DEVICE_DISCONNECTED_C) {
          gLogger->println("Error: Could not read temperature data");
          return;
      }
      fillState_.setTemperature(tempC);
  }