#include "config.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

bool loadConfig() {
  if (!LittleFS.exists("/config.json")) {
    Serial.println("⚠️  Config file not found");
    
    // Set sensible defaults
    config.wifi_ssid = "YOUR_WIFI";
    config.wifi_password = "YOUR_PASSWORD";
    config.hostname = "cellar-fan";
    config.mqtt_enabled = false;
    config.mqtt_broker = "192.168.1.100";
    config.mqtt_port = 1883;
    config.mqtt_user = "";
    config.mqtt_password = "";
    config.target_temp = 18.0;
    config.target_humidity = 55.0;
    config.temp_differential = 3.0;
    config.humidity_differential = 10.0;
    config.min_outside_temp = -5.0;
    config.min_cellar_temp = 8.0;
    config.max_dew_point_increase = 3.0;
    config.low_speed = 60;
    config.high_speed = 100;
    config.min_run_time_sec = 300;
    config.min_idle_time_sec = 180;
    config.forced_interval_hours = 6;
    config.forced_duration_min = 10;
    
    return false;
  }
  
  File file = LittleFS.open("/config.json", "r");
  if (!file) {
    Serial.println("❌ Failed to open config.json");
    return false;
  }
  
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  
  if (error) {
    Serial.printf("❌ Failed to parse config.json: %s\n", error.c_str());
    return false;
  }
  
  // WiFi
  config.wifi_ssid = doc["wifi"]["ssid"].as<String>();
  config.wifi_password = doc["wifi"]["password"].as<String>();
  config.hostname = doc["wifi"]["hostname"] | "cellar-fan";
  
  // MQTT
  config.mqtt_enabled = doc["mqtt"]["enabled"] | false;
  config.mqtt_broker = doc["mqtt"]["broker"] | "192.168.1.100";
  config.mqtt_port = doc["mqtt"]["port"] | 1883;
  config.mqtt_user = doc["mqtt"]["user"] | "";
  config.mqtt_password = doc["mqtt"]["password"] | "";
  
  // Thresholds
  config.target_temp = doc["thresholds"]["target_temp"] | 18.0;
  config.target_humidity = doc["thresholds"]["target_humidity"] | 55.0;
  config.temp_differential = doc["thresholds"]["temp_differential"] | 3.0;
  config.humidity_differential = doc["thresholds"]["humidity_differential"] | 10.0;
  config.min_outside_temp = doc["thresholds"]["min_outside_temp"] | -5.0;
  config.min_cellar_temp = doc["thresholds"]["min_cellar_temp"] | 8.0;
  config.max_dew_point_increase = doc["thresholds"]["max_dew_point_increase"] | 3.0;
  
  // Fan
  config.low_speed = doc["fan"]["low_speed"] | 60;
  config.high_speed = doc["fan"]["high_speed"] | 100;
  config.min_run_time_sec = doc["fan"]["min_run_time_sec"] | 300;
  config.min_idle_time_sec = doc["fan"]["min_idle_time_sec"] | 180;
  
  // Circulation
  config.forced_interval_hours = doc["circulation"]["forced_interval_hours"] | 6;
  config.forced_duration_min = doc["circulation"]["forced_duration_min"] | 10;
  
  return true;
}

bool saveConfig() {
  JsonDocument doc;
  
  doc["wifi"]["ssid"] = config.wifi_ssid;
  doc["wifi"]["password"] = config.wifi_password;
  doc["wifi"]["hostname"] = config.hostname;
  
  doc["mqtt"]["enabled"] = config.mqtt_enabled;
  doc["mqtt"]["broker"] = config.mqtt_broker;
  doc["mqtt"]["port"] = config.mqtt_port;
  doc["mqtt"]["user"] = config.mqtt_user;
  doc["mqtt"]["password"] = config.mqtt_password;
  
  doc["thresholds"]["target_temp"] = config.target_temp;
  doc["thresholds"]["target_humidity"] = config.target_humidity;
  doc["thresholds"]["temp_differential"] = config.temp_differential;
  doc["thresholds"]["humidity_differential"] = config.humidity_differential;
  doc["thresholds"]["min_outside_temp"] = config.min_outside_temp;
  doc["thresholds"]["min_cellar_temp"] = config.min_cellar_temp;
  doc["thresholds"]["max_dew_point_increase"] = config.max_dew_point_increase;
  
  doc["fan"]["low_speed"] = config.low_speed;
  doc["fan"]["high_speed"] = config.high_speed;
  doc["fan"]["min_run_time_sec"] = config.min_run_time_sec;
  doc["fan"]["min_idle_time_sec"] = config.min_idle_time_sec;
  
  doc["circulation"]["forced_interval_hours"] = config.forced_interval_hours;
  doc["circulation"]["forced_duration_min"] = config.forced_duration_min;
  
  File file = LittleFS.open("/config.json", "w");
  if (!file) {
    Serial.println("❌ Failed to open config.json for writing");
    return false;
  }
  
  if (serializeJson(doc, file) == 0) {
    Serial.println("❌ Failed to write config.json");
    file.close();
    return false;
  }
  
  file.close();
  Serial.println("✓ Configuration saved");
  return true;
}

void printConfig() {
  Serial.println("Configuration:");
  Serial.printf("  WiFi SSID: %s\n", config.wifi_ssid.c_str());
  Serial.printf("  Hostname: %s\n", config.hostname.c_str());
  Serial.printf("  MQTT: %s\n", config.mqtt_enabled ? "Enabled" : "Disabled");
  Serial.printf("  Target Temp: %.1f°C\n", config.target_temp);
  Serial.printf("  Target Humidity: %.1f%%\n", config.target_humidity);
  Serial.printf("  Low Speed: %d%%, High Speed: %d%%\n", config.low_speed, config.high_speed);
}