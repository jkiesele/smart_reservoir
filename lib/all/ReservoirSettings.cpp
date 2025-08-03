
#include <ReservoirSettings.h>

//ReservoirSettings settings;

bool CirculationPumpSettings::sanityCheck() {

    bool ret = true;
    //min level >= 0
    if (minLevel < 0) {
        gLogger->println("CirculationPumpSettings: minLevel < 0");
        minLevel=0;
        ret = false;
    }
    if (minLevel   < 0){
        gLogger->println("CirculationPumpSettings: minLevel < 0");
        minLevel = 0;
        ret = false;
    }
    if (circPDay   < 0) {
        gLogger->println("CirculationPumpSettings: circPDay < 0");
        circPDay = 0;
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
    return ret;
}