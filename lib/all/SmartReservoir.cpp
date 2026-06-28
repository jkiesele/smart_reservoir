#include "SmartReservoir.h"

// NOTE: webLog, gLogger, systemID, setLogger, setTimeProvider are assumed
// to be provided by the included project headers (as in your original sketch).

SmartReservoir::SmartReservoir(const FillSensorConfig& touchPinsAndFractions,
                               int                       circulationPumpPin,
                               int                       temperaturePin,
                               uint8_t               debugLEDPin)
: 
  circulationPumpPin_(circulationPumpPin),
  settings_(touchPinsAndFractions.points().size()), // pass number of pads to settings
  circPumpSettings_(), 
  // Construct fill state using ctor-exposed parameters and pointer to settings_
  fillState_(touchPinsAndFractions, settings_),//after settings_ is constructed!
  fillStateDisplay_(fillState_),
  pumpRunningDisplay_("pumpRunning", 2, false),//2s update interval, initial false
  temperatureDisplay_("temperature", 60, 0.0f), // updates every 60s, initial 0.0
  forceSendButton_("fSend", "Force Send", [this]() { onForceSendButtonClick(); }, 2), // callback for button click
  led_(debugLEDPin),
  scheduler_(),
  timeManager_(),
  wifi_(secret::ssid, secret::password),
  webInterface_(),
  reporter_(fillState_, settings_),
  wifiRSSIDisplay_("wifiRSSI", wifi_),
  temperatureGraph_("tempGraph", 2, "Temperature Over Time", "Time", "°C", 5*24*2),  // 5 days of data at 30 min intervals
  fillGraph_("fillGraph", 2, "Fill Level Over Time", "Time", "%", 5*24*2, 0, 100), // filled only when changes are detected
  otaUpload_(secret::otaPassword)
{
    /*
    
    
    WebTimeSeriesGraph(const String& id,
                       uint32_t updateIntervalSecs,
                       const String& title,
                       const String& xLabel,
                       const String& yUnit,
                       size_t maxEntries = 144)*/
  // Any global init-at-declaration from the sketch that isn't tied to hardware
  // can be placed here if needed.
  if (temperaturePin >= 0) {
      oneWirep_ = std::unique_ptr<OneWire>(new OneWire(temperaturePin));
      tempsens_.setOneWire(oneWirep_.get());
  }
  else{
      oneWirep_.reset(); //does nothing, just for clarity
  }
}

void SmartReservoir::begin() {
  // Mirrors original setup()

  SettingsBlockBase::kSettingsPassword = secret::hydroPassword;

  delay(100);
  led_.begin();
  led_.setRed();

  Serial.begin(115200);
  Serial.println("Starting system initialization...");
  webLog.begin();
  setLogger(&webLog);
  webLog.mirrorToSerial = true;
  webLog.setLogSize(40); // keep last 40 messages in memory for web display

  if(oneWirep_)
      tempsens_.begin();

  settings_.begin();
  fillState_.begin();
  if(circulationPumpPin_>=0) {
    circPumpSettings_.begin();
    //set up output GPIO
    pinMode(circulationPumpPin_, OUTPUT);
    digitalWrite(circulationPumpPin_, LOW); // ensure pump is off initially
  }



  if(oneWirep_) {
      tempsens_.begin();
      gLogger->println("Temperature sensor initialized");
  }


  gLogger->println("Starting WiFi connection...");
  wifi_.begin();//true, false); // don't run in high power mode
  delay(1000);

  timeManager_.begin();
  setTimeProvider(&timeManager_);
  gLogger->println("Time manager initialized");

  systemID.begin(); //does not hurt to call begin again if already called, and ensures systemID is ready for use
  gLogger->println("System ID initialized: " + systemID.systemName());


  webInterface_.addDisplay("", &wifiRSSIDisplay_);
  // Add pump running display if circulation pump is used
  if (circulationPumpPin_ >= 0) {
      webInterface_.addDisplay("", &pumpRunningDisplay_);
  }
  if (oneWirep_) {
      webInterface_.addDisplay("Temperature [˚C]", &temperatureDisplay_);
      webInterface_.addDisplay("", &temperatureGraph_);
  }
  // Add displays to the web interface
  for (const auto& display : fillStateDisplay_.getDisplays()) {
    webInterface_.addDisplay(display.first, display.second);
  }
  //add fill graph
  webInterface_.addDisplay("", &fillGraph_);
  // Add force send button to web interface
  webInterface_.addDisplay("Force Send",&forceSendButton_);
  // Add settings display
  webInterface_.addSettings("Reservoir Settings - " + revision, &settings_);
  if (circulationPumpPin_>=0) {
      webInterface_.addSettings("Circulation Pump Settings", &circPumpSettings_);
  }

  webInterface_.addWebItem(&otaUpload_); // Add OTA upload handler

  webInterface_.begin();
  gLogger->println("Web interface started");

  gLogger->println("Touch sensors initialized");

  delay(100);
  //do an update so that the reporter has valid initial data to send and the web interface shows something immediately
  for(int i = 0; i < 80; ++i) {
      fillState_.update(false); // skip sanity check for initial update to avoid warnings about sensor state at startup
      delay(50);
  }
  fillStateDisplay_.update();
  updateTemperature();
  
  ReservoirReporter::Config reporterConfig;
  reporterConfig.heartbeatIntervalMs = 60000; // 1 minute
  reporterConfig.minAttemptIntervalMs = 1000; // 1 second
  if(reporter_.begin(reporterConfig))
      gLogger->println("Reporter started");
  else
      gLogger->println("Failed to start reporter");

  // Scheduler: update fill state every 250ms
  // also turn off circulation pump immediately if fill level is too low, just in case
  scheduler_.addTimedTask([this]() {
      fillState_.update();
      fillStateDisplay_.update();
      if (circulationPumpPin_ >= 0 
        && fillState_.litersFullMin() < circPumpSettings_.minLevel
        && isCirculationPumpOn()) {
        gLogger->println("Circulation pump turned OFF due to low fill level");
          turnOffCirculationPump();
      }
    },
    250,  // first delay
    true,  // repeat
    250   // interval
  );


  //update temperature every 10 minutes
  scheduler_.addTimedTask([this]() {
      updateTemperature();
      temperatureDisplay_.update(fillState_.temperature());
    },
    5*SECOND,  // first delay five seconds after start
    true,  // repeat
    1*MINUTE   // interval
  );

   scheduler_.addTimedTask([this]() {
       //make sure we hav valid data before adding to graph
        if(oneWirep_){
            const float temp = fillState_.temperature();
            const uint32_t ts = timeManager_.getUnixUTCTime();
            if (oneWirep_ && ts > 1767225600UL && temp > -30.0f) { 
                temperatureGraph_.append(ts, temp);
            }
        }
   },
   10*SECOND,  // first delay ten seconds after start
   true,  // repeat
   30*MINUTE   // interval: 30 minutes
   );

   if(!hasPump()){ 
       //check every minute if the fill graph needs an update
        scheduler_.addTimedTask([this]() {
            //if no pump
            //otherwise do that in the pump schedule task
            //the reason is that the running pump can cause small fluctuations
            //and we want a defined measurement every time right before the pump runs.
            updateFillGraphIfNeeded();
        },
        1*MINUTE,  // first delay
        true,  // repeat
        1*MINUTE   // interval: 1 minute
        );
    }
    updateFillGraphIfNeeded(); // initial update of fill graph


  if (hasPump()) {
    //init pwm for circulation pump if needed
    auto actual = ledcSetup(pwmChannel_, //channel
      circPumpSettings_.pwmFreq, //freq
      pwmRes_); //resolution bits
    if (actual == 0) {
        gLogger->println("Failed to configure PWM, falling back to 1000 Hz");
        ledcSetup(pwmChannel_, 1000, pwmRes_);
        currentFreq = 1000; // set to fallback frequency
    }
    else{
        currentFreq = circPumpSettings_.pwmFreq; // set to configured frequency
    }
    ledcAttachPin(circulationPumpPin_,pwmChannel_);
    scheduleCirculationPump();
  }

  //report RSSI once
  int32_t rssi = wifi_.getSignalStrength();
  gLogger->println("WiFi RSSI: " + String(rssi) + " dBm");
  gLogger->println("Initial fill level: " + String(fillState_.level()) + " %");

  led_.setGreen();
  delay(1000);
  led_.setOff();
  //all good
  otaUpload_.markAppValid(); // prevent rollback on next boot
}

void SmartReservoir::loop() {
  wifi_.loop();
  handleNetworkResetEvent();

  scheduler_.loop();
  webInterface_.loop();
  reporter_.loop();
}


void SmartReservoir::scheduleCirculationPump(){
    if(!hasPump()) 
       return;

    int periodS = circPumpSettings_.circPDay;
    // if this is set to zero (at the moment), just schedule another check in 1 minute and return, don't run the pump
    // this allows to disable the pump from the web interface without changing the code
    if (periodS <= 0) {
        scheduler_.addTimedTask([this]() {
            scheduleCirculationPump();
        }, 1 * MINUTE, false); // don't repeat
        return;
    }
    // it is already ensured that the circulation period is at least as long as the time the pump is on during day or night
    int runS = circPumpSettings_.circTDay; 

    if(timeManager_.isNightTime()) {
        runS = circPumpSettings_.circTNight;
    }
    periodS -= runS; // ensure period is time between end of one run and start of next
    
    if(periodS < 1) periodS = 1; // just in case, to avoid too fast rescheduling; 
    //schedule
    scheduler_.addTimedTask([this, runS]() {
        // turn off after timeMS
        if(runS <= 0){
            turnOffCirculationPump(); // somewhat obsolete but safer
            scheduleCirculationPump(); // this is safe in the scheduler implementation!
            return;
        }
        //update fill graph
        updateFillGraphIfNeeded();
        safeTurnOnCirculationPump();

        scheduler_.addTimedTask([this]() {
            turnOffCirculationPump();
            // Reschedule the next circulation pump run
            // the scheduler can deal with nested tasks and will run them in order
            scheduleCirculationPump();
        }, 
        runS * SECOND, 
        false); // do not repeat

    }, 
    periodS * SECOND, // schedule the next run for the correct time after the current one ends
    false); // don't repeat, self-reschedule

}

  void SmartReservoir::safeTurnOnCirculationPump(){
    if(!hasPump()) 
       return;

    if(! circPumpSettings_.enabled)
        return;

    //check if fill state is high enough
    if (fillState_.litersFullMin() < circPumpSettings_.minLevel){
        gLogger->println("Circulation pump not turned ON: fill level too low");
        return;
    }
    int duty = map(circPumpSettings_.dutyCycle, 0, 100, 0, (1 << pwmRes_) - 1);
    // change PWM frequency if needed
    if (currentFreq != circPumpSettings_.pwmFreq) {
        auto actual = ledcSetup(pwmChannel_, circPumpSettings_.pwmFreq, pwmRes_);
        if (actual == 0) {
            gLogger->println("Failed to configure PWM, falling back to 1000 Hz");
            ledcSetup(pwmChannel_, 1000, pwmRes_);
        }
        currentFreq = circPumpSettings_.pwmFreq; //set it anyway so the warning gets printed only once and the code does no re-attempt to set an invalid frequency
    }
    ledcWrite(pwmChannel_, duty);
    // gLogger->println("Circulation pump turned ON with duty cycle " + String(circPumpSettings_.dutyCycle) + "%");
    pumpRunningDisplay_.update(true);
  }
  void SmartReservoir::turnOffCirculationPump(){
    if(!hasPump()) 
       return;
    ledcWrite(pwmChannel_, 0);
    pumpRunningDisplay_.update(false);
    // gLogger->println("Circulation pump turned OFF");
  }

  bool SmartReservoir::isCirculationPumpOn() const {
    if(!hasPump()) 
       return false;
    // use the pwm setting here to determine
    return ledcRead(pwmChannel_) > 0;
  }

  void SmartReservoir::updateTemperature() {
      if (!oneWirep_){
        fillState_.setTemperature(-127.0f); // set to a default error value if no sensor
        return; // No sensor configured
      }

      tempsens_.requestTemperatures(); // Send the command to get temperatures
      float tempC = tempsens_.getTempCByIndex(0); // Get temperature in Celsius
      if (tempC == DEVICE_DISCONNECTED_C) {
          gLogger->println("Error: Could not read temperature data");
          fillState_.setTemperature(-127.0f);
          return;
      }
      fillState_.setTemperature(tempC);
  }

  void SmartReservoir::updateFillGraphIfNeeded() {
    // This function can be called whenever the fill level is updated to append the new level to the graph
    auto pastpoint = fillGraph_.lastPoint();
    float lastlevel = pastpoint.y;
    float currentLevel = fillState_.level();

    bool addpoint = isnan(lastlevel) || fabs(lastlevel - currentLevel) > 0.01f;

    //now add a new point at least every 6 hours even if the level does not change
    uint32_t now = timeManager_.getUnixUTCTime();
    if(!isnan(pastpoint.y) && now - pastpoint.x >= 6*3600){
        addpoint = true;
    }

    auto timeStamp = timeManager_.getUnixUTCTime();
    //check if the time stamp makes sense (e.g. after Jan 1, 2026) 
    //to avoid adding points with wrong timestamps at the beginning if the time is not set yet
    if(timeStamp < 1767225600UL){ // Jan 1, 2026 in Unix time
        addpoint = false;
    }

    //also a sanity check: check if time stamp is in the future w.r.t. last point
    if(!isnan(pastpoint.y) && timeStamp < pastpoint.x){
        addpoint = false;
    }

    //they should actually be float identical unless sth changes to = should be ok, but just in case...
    //last level can also be NAN
    if(addpoint) { // only update if level changed by at least 0.01% to avoid noise
        fillGraph_.append(timeManager_.getUnixUTCTime(), currentLevel);
    }
  }