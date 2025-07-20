#include <Arduino.h>
#include <WebSettings.h>

#ifndef SETTINGS_H
#define SETTINGS_H

class Settings : public SettingsBlockBase {

    public:
    Settings(): SettingsBlockBase("settings", "/settings"){}

    DEF_SETTING(int, thTouch, "Touch Threshold (ADC)", 24000,1);
    DEF_SETTING(float, totalVolume, "Total Volume (L)", 10.0f, 0.1f);

    DEF_SETTING(String, injUrl, "Injector hostname", "injector.local", 0);

};

extern Settings settings;

#endif