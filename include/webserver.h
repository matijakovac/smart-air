#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <Arduino.h>
#include <WiFi.h>

// Define HTTP method constants BEFORE including ESPAsyncWebServer
#ifndef HTTP_GET
#define HTTP_GET 1
#define HTTP_POST 2
#define HTTP_DELETE 3
#define HTTP_PUT 4
#define HTTP_PATCH 5
#define HTTP_HEAD 6
#define HTTP_OPTIONS 7
#define HTTP_ANY 255
#endif

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