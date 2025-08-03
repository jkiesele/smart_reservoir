#include <Arduino.h>
#include <WebServer.h>
#include <WebDisplay.h>
#include <WebSettings.h>

class ReservoirFillState;

#ifndef WEBINTERFACE_H
#define WEBINTERFACE_H

class WebInterface {
public:

    WebInterface(){}
    void begin();
    void loop(){
        server.handleClient();
    }

    void setupRoutes();
    String generateHTML() const;

    void addDisplay(const String & descText, WebDisplayBase *display) {
        displays_.emplace_back(descText, display);
    }
    void addSettingsDisplay(const String & descText, SettingsBlockBase *settings) {
        settingsDisplays_.emplace_back(descText, settings);
    }

private:
    ReservoirFillState *fillState_; // Pointer to FillState for accessing touch sensors
    WebServer server;
    std::vector<std::pair<String, WebDisplayBase*>> displays_;
    std::vector<std::pair<String, SettingsBlockBase*>> settingsDisplays_;

};
#endif