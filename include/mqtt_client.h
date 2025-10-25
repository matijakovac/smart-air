#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "sensors.h"
#include "fancontrol.h"

class MQTTManager {
public:
  MQTTManager(SensorManager& sensors, FanController& fan);
  void begin();
  void loop();
  bool isConnected() { return mqttClient.connected(); }
  
private:
  WiFiClient wifiClient;
  PubSubClient mqttClient;
  SensorManager& sensorManager;
  FanController& fanController;
  unsigned long lastPublish;
  unsigned long lastReconnectAttempt;
  bool discoveryPublished;
  
  void reconnect();
  void callback(char* topic, byte* payload, unsigned int length);
  void publishDiscovery();
  void publishSensors();
  void publishStatus();
  
  String getDeviceId() const;
};

#endif