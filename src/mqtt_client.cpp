#include "mqtt_client.h"

MQTTManager::MQTTManager(SensorManager& sensors, FanController& fan)
  : mqttClient(wifiClient), sensorManager(sensors), fanController(fan) {
  lastPublish = 0;
  lastReconnectAttempt = 0;
  discoveryPublished = false;
}

void MQTTManager::begin() {
  if (!config.mqtt_enabled) {
    Serial.println("MQTT disabled in config");
    return;
  }
  
  mqttClient.setServer(config.mqtt_broker.c_str(), config.mqtt_port);
  mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) {
    this->callback(topic, payload, length);
  });
  
  Serial.println("✓ MQTT client initialized");
  reconnect();
}

void MQTTManager::loop() {
  if (!config.mqtt_enabled) return;
  
  unsigned long now = millis();
  
  // Reconnect if needed
  if (!mqttClient.connected()) {
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      reconnect();
    }
  } else {
    mqttClient.loop();
    
    // Publish periodically
    if (now - lastPublish >= MQTT_PUBLISH_INTERVAL) {
      publishSensors();
      publishStatus();
      lastPublish = now;
    }
  }
}

void MQTTManager::reconnect() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  
  String clientId = "cellar-fan-" + getDeviceId();
  
  Serial.print("Connecting to MQTT... ");
  
  bool connected;
  if (config.mqtt_user.length() > 0) {
    connected = mqttClient.connect(clientId.c_str(), 
                                   config.mqtt_user.c_str(), 
                                   config.mqtt_password.c_str());
  } else {
    connected = mqttClient.connect(clientId.c_str());
  }
  
  if (connected) {
    Serial.println("connected");
    
    // Subscribe to command topic
    mqttClient.subscribe("cellar/fan/command");
    mqttClient.subscribe("cellar/mode/set");
    
    // Publish discovery if not done yet
    if (!discoveryPublished) {
      publishDiscovery();
      discoveryPublished = true;
    }
    
    // Publish initial state
    publishSensors();
    publishStatus();
    
  } else {
    Serial.printf("failed, rc=%d\n", mqttClient.state());
  }
}

void MQTTManager::callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.printf("MQTT message [%s]: %s\n", topic, message.c_str());
  
  if (strcmp(topic, "cellar/mode/set") == 0) {
    if (message == "auto") {
      fanController.setMode(MODE_AUTO);
    } else if (message == "off") {
      fanController.setMode(MODE_MANUAL_OFF, 60);
    } else if (message == "low") {
      fanController.setMode(MODE_MANUAL_LOW, 60);
    } else if (message == "high") {
      fanController.setMode(MODE_MANUAL_HIGH, 60);
    }
  }
}

void MQTTManager::publishDiscovery() {
  String deviceId = getDeviceId();
  String baseTopic = "homeassistant/sensor/cellar_fan_";
  
  // Device info
  JsonDocument device;
  device["identifiers"][0] = deviceId;
  device["name"] = "Cellar Ventilation";
  device["manufacturer"] = "DIY";
  device["model"] = "ESP32 Fan Controller";
  device["sw_version"] = "1.0.0";
  
  // Internal Temperature
  JsonDocument tempIn;
  tempIn["name"] = "Cellar Temperature";
  tempIn["unique_id"] = deviceId + "_temp_in";
  tempIn["state_topic"] = "cellar/sensors/internal";
  tempIn["value_template"] = "{{ value_json.temperature }}";
  tempIn["unit_of_measurement"] = "°C";
  tempIn["device_class"] = "temperature";
  tempIn["device"] = device;
  
  String payload;
  serializeJson(tempIn, payload);
  mqttClient.publish((baseTopic + "temp_in/config").c_str(), payload.c_str(), true);
  
  // Add more discovery messages for other sensors...
  // (humidity, pressure, fan state, etc.)
  
  Serial.println("✓ MQTT Discovery published");
}

void MQTTManager::publishSensors() {
  SensorData internal = sensorManager.getInternalData();
  SensorData external = sensorManager.getExternalData();
  
  if (!internal.valid || !external.valid) {
    return;
  }
  
  // Internal sensors
  JsonDocument intDoc;
  intDoc["temperature"] = internal.temperature;
  intDoc["humidity"] = internal.humidity;
  intDoc["pressure"] = internal.pressure;
  intDoc["dewpoint"] = internal.dewPoint;
  
  String intPayload;
  serializeJson(intDoc, intPayload);
  mqttClient.publish("cellar/sensors/internal", intPayload.c_str());
  
  // External sensors
  JsonDocument extDoc;
  extDoc["temperature"] = external.temperature;
  extDoc["humidity"] = external.humidity;
  extDoc["pressure"] = external.pressure;
  extDoc["dewpoint"] = external.dewPoint;
  
  String extPayload;
  serializeJson(extDoc, extPayload);
  mqttClient.publish("cellar/sensors/external", extPayload.c_str());
}

void MQTTManager::publishStatus() {
  JsonDocument doc;
  
  doc["speed"] = fanController.getCurrentSpeed();
  doc["state"] = fanController.getCurrentSpeed() > 0 ? "ON" : "OFF";
  doc["reason"] = fanController.getStatusText();
  
  const char* modeNames[] = {"AUTO", "MANUAL_OFF", "MANUAL_LOW", "MANUAL_HIGH", "DIAGNOSTIC"};
  doc["mode"] = modeNames[currentMode];
  
  doc["wifi_rssi"] = WiFi.RSSI();
  doc["uptime"] = millis() / 1000;
  doc["free_heap"] = ESP.getFreeHeap();
  
  String payload;
  serializeJson(doc, payload);
  mqttClient.publish("cellar/status", payload.c_str());
}

String MQTTManager::getDeviceId() const {
  uint64_t chipid = ESP.getEfuseMac();
  char id[13];
  snprintf(id, sizeof(id), "%04X%08X", 
           (uint16_t)(chipid >> 32), 
           (uint(uint32_t)chipid);
  return String(id);
}