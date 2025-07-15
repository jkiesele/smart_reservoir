#include <Arduino.h>
#include <WebSettings.h>

#ifndef SETTINGS_H
#define SETTINGS_H

class Settings : public SettingsBlockBase {

    public:
    Settings(): SettingsBlockBase("settings", "/settings"){}

    DEF_SETTING(int, th5percent, "Threshold 5%", 20000,1);
    DEF_SETTING(int, th50percent, "Threshold 50%", 20000,1);
    DEF_SETTING(int, th75percent, "Threshold 75%", 20000,1);
    DEF_SETTING(int, th100percent, "Threshold 100%", 20000,1);

};

extern Settings settings;

#endif