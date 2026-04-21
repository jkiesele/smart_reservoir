
#include <ReservoirSettings.h>

//ReservoirSettings settings;

bool ReservoirSettings::sanityCheck() {
    bool ok = true;

    if (tcpPort <= 0 || tcpPort > 65535) {
        gLogger->println("ReservoirSettings: tcpPort out of range, resetting to default");
        tcpPort = 12345;
        ok = false;
    }

    if (channelID < 0 || channelID > 255) {
        gLogger->println("ReservoirSettings: channelID out of range, resetting to default");
        channelID = 0;
        ok = false;
    }

    return ok;
}

bool CirculationPumpSettings::sanityCheck() {

    bool ret = true;
    //min level >= 0
    if (minLevel < 0) {
        gLogger->println("CirculationPumpSettings: minLevel < 0");
        minLevel=0;
        ret = false;
    }
    if (circPDay   < 10) {
        gLogger->println("CirculationPumpSettings: circPDay < 10");
        circPDay = 10;
        ret = false;
    }
    if (circTDay   < 0) {
        gLogger->println("CirculationPumpSettings: circTDay < 0");
        circTDay = 0;
        ret = false;
    }
    if (circTNight < 0) {
        gLogger->println("CirculationPumpSettings: circTNight < 0");
        circTNight = 0;
        ret = false;
    }
    if (circPDay < circTDay) {
        gLogger->println("CirculationPumpSettings: circPDay < circTDay");
        circPDay = circTDay; // use circTDay if larger
        ret = false;
    }
    if (circPDay < circTNight) {
        gLogger->println("CirculationPumpSettings: circPDay < circTNight");
        circPDay = circTNight; // use circTNight if larger
        ret = false;
    }
    if(dutyCycle < 0) {
        gLogger->println("CirculationPumpSettings: dutyCycle < 0");
        dutyCycle = 0;
        ret = false;
    }
    if(dutyCycle > 100) {
        gLogger->println("CirculationPumpSettings: dutyCycle > 100");
        dutyCycle = 100;
        ret = false;
    }
    if (pwmFreq < 1) {
        gLogger->println("CirculationPumpSettings: pwmFreq < 1");
        pwmFreq = 1000;
        ret = false;
    }
    if (pwmFreq > 20000) {
        gLogger->println("CirculationPumpSettings: pwmFreq too high");
        pwmFreq = 20000;
        ret = false;
    }
    return ret;
}