#include <Arduino.h>
#include <WebSettings.h>

#ifndef RESERVOIRSETTINGS_H
#define RESERVOIRSETTINGS_H

class ReservoirSettings : public SettingsBlockBase {

    public:
    ReservoirSettings(): SettingsBlockBase("settings", "/settings"){}

    DEF_SETTING(int, thTouch, "Touch Threshold (ADC)", 24000,1);
    DEF_SETTING(float, totalVolume, "Total Volume (L)", 10.0f, 0.1f);

    DEF_SETTING(String, injUrl, "Injector hostname", "injector.local", 0);

};

//class for circulation pump settings
class CirculationPumpSettings : public SettingsBlockBase {
public:
    CirculationPumpSettings(): SettingsBlockBase("circPump", "/circPump") {}
    DEF_SETTING(float, minLevel, "Min level for circulation [l]", 1.0f, 0.1f);
    DEF_SETTING(float, circPDay,   "Circ period day [min]",  60.0f, 1.0f);
    DEF_SETTING(float, circTDay,   "Circ time day [min]",    10.0f, 1.0f);
    DEF_SETTING(float, circTNight, "Circ time night [min]",   5.0f, 1.0f);

};


//extern ReservoirSettings settings;

#endif