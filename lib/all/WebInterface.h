#include <Arduino.h>
#include <WebServer.h>
#include <WebDisplay.h>
#include <Settings.h>

class FillState;

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

private:
    FillState *fillState_; // Pointer to FillState for accessing touch sensors
    WebServer server;
    std::vector<std::pair<String, WebDisplayBase*>> displays_;

};
#endif