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

//extern ReservoirSettings settings;

#endif