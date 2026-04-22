#include <Arduino.h>
#include <WebSettings.h>

#ifndef RESERVOIRSETTINGS_H
#define RESERVOIRSETTINGS_H

class ReservoirSettings : public SettingsBlockBase {

    public:
    ReservoirSettings(int npads): SettingsBlockBase("settings", "/settings"), n_pads(npads) {}

    private:
    int n_pads;//must come first

    public:
    DEF_SETTING_ARRAY(int, thsTouch, "Touch Thresholds (ADC)", 24000,1, n_pads);
    DEF_SETTING(float, totalVolume, "Total Volume (L)", 10.0f, 0.1f);
    DEF_SETTING(String, injUrl, "Injector hostname", "injector.local", 0);
    //injector IP and MAC address
    DEF_SETTING(IPAddress, injIP, "Remote IP address", IPAddress(192,168,178,100), 0);
    DEF_SETTING(tcpmsg::MACAddress, injMAC, "Remote MAC address", tcpmsg::MACAddress(), 0);
    //own channel ID as int
    DEF_SETTING(int, channelID, "Channel ID", 1, 1);
    DEF_SETTING(int, tcpPort, "TCP Port of remote", 12345, 1);
    DEF_SETTING(bool, sendActive, "Send reports to remote", false, 0);

    bool sanityCheck() override;

};

//class for circulation pump settings
class CirculationPumpSettings : public SettingsBlockBase {
public:
    CirculationPumpSettings(): SettingsBlockBase("circPump", "/circPump") {}
    DEF_SETTING(bool, enabled, "Enable circulation pump", false, 0);
    DEF_SETTING(float, minLevel, "Min level for circulation [l]", 1.0f, 0.1f);
    DEF_SETTING(int, circPDay,   "Circ interval [s]",   60, 1);
    DEF_SETTING(int, circTDay,   "Circ time day [s]",   0, 1);
    DEF_SETTING(int, circTNight, "Circ time night [s]",   0, 1);
    DEF_SETTING(int, dutyCycle, "Duty cycle [%]",      100, 1);
    DEF_SETTING(int, pwmFreq, "PWM frequency [Hz]",  1000, 1);

    bool sanityCheck() override ;

};


//extern ReservoirSettings settings;

#endif