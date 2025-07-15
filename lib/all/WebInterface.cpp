#include <WebInterface.h>
#include <SystemID.h>
#include <ESPmDNS.h>
#include "WebStatus.h"

void WebInterface::begin() {

    if (!MDNS.begin(systemID.systemName())) {  // Replace with your desired hostname
        Serial.println("Error setting up MDNS responder!");
    } else {
        Serial.println("mDNS responder started. Access at http://"+systemID.systemName()+ ".local");
    }
    server.begin();
    setupRoutes();
}
void WebInterface::setupRoutes() {
    server.on("/", HTTP_GET, [this]() {
        server.send(200, "text/html", generateHTML());
    });

    for (const auto& display : displays_) {
        server.on(display.second->handle(), HTTP_GET, [display,this]() {
           server.send(200, "application/json", display.second->routeText());
        });
    }
    settings.setupRoutes(server);

    // System status route (return JSON)
    server.on("/status", HTTP_GET, [this]() {
        server.send(200, "application/json", WebStatus::getSystemStatus());
    });

    // Log route (for dynamic log updates)
    server.on("/log", HTTP_GET, [this]() {
        server.send(200, "text/plain", WebStatus::createLogText());
    });

    server.onNotFound([this]() {
        server.send(404, "text/plain", "Not Found");
    });
}

String WebInterface::generateHTML() const {
    String html = R"(
    <!DOCTYPE html>
    <html>
    <head>
    <meta charset="UTF-8">
    <title>ESP32 )" + systemID.systemName() + R"( Control</title>
    </head>
    <body>
    <h2>System Status</h2>
    )";

    html += WebStatus::createSystemStatHtmlFragment();

    for (const auto& display : displays_) {
        html += "<h2>" + display.first + "</h2>";
        html += display.second->createHtmlFragment();
    }
    html += "<h2>Settings</h2>";
    html += settings.generateHTML();
    html += "</body></html>";
    return html;
}