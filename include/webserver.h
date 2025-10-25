#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Update.h>
#include "config.h"
#include "sensors.h"
#include "fancontrol.h"

class WebServerManager {
public:
  WebServerManager(SensorManager& sensors, FanController& fan);
  void begin();
  
private:
  AsyncWebServer server;
  SensorManager& sensorManager;
  FanController& fanController;
  
  void setupRoutes();
  String getMainHTML() const;
  String getStatusJSON() const;
  String getConfigJSON() const;
  
  void handleSetMode(AsyncWebServerRequest *request);
  void handleSetConfig(AsyncWebServerRequest *request);
  void handleOTAUpload(AsyncWebServerRequest *request, String filename, 
                      size_t index, uint8_t *data, size_t len, bool final);
};

#endif