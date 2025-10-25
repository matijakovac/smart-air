#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Hardware Pin Definitions
#define PIN_RELAY 5
#define PIN_DIMMER_ZC 22
#define PIN_DIMMER_PSM 21
#define PIN_INTERNAL_SDA 19
#define PIN_INTERNAL_SCL 18
#define PIN_EXTERNAL_SDA 16
#define PIN_EXTERNAL_SCL 17
#define PIN_OLED_SDA 13
#define PIN_OLED_SCL 12

// I2C Addresses
#define BMP280_ADDR_INTERNAL 0x77
#define BMP280_ADDR_EXTERNAL 0x77
#define OLED_ADDR 0x3C

// Timing Constants (milliseconds)
#define SENSOR_READ_INTERVAL 5000
#define DISPLAY_UPDATE_INTERVAL 2000
#define MQTT_PUBLISH_INTERVAL 30000
#define DECISION_INTERVAL 10000

// Control Modes
enum ControlMode {
  MODE_AUTO,
  MODE_MANUAL_OFF,
  MODE_MANUAL_LOW,
  MODE_MANUAL_HIGH,
  MODE_DIAGNOSTIC
};

// Fan Run Reasons
enum RunReason {
  REASON_OFF,
  REASON_HUMIDITY,
  REASON_TEMPERATURE,
  REASON_BOTH,
  REASON_FORCED_CIRCULATION,
  REASON_MANUAL_OVERRIDE,
  REASON_SAFETY_LIMIT
};

// System Configuration Structure
struct SystemConfig {
  // WiFi
  String wifi_ssid;
  String wifi_password;
  String hostname;
  
  // MQTT
  bool mqtt_enabled;
  String mqtt_broker;
  int mqtt_port;
  String mqtt_user;
  String mqtt_password;
  
  // Environmental Thresholds
  float target_temp;
  float target_humidity;
  float temp_differential;
  float humidity_differential;
  float min_outside_temp;
  float min_cellar_temp;
  float max_dew_point_increase;
  
  // Fan Configuration
  int low_speed;
  int high_speed;
  int min_run_time_sec;
  int min_idle_time_sec;
  
  // Forced Circulation
  int forced_interval_hours;
  int forced_duration_min;
};

// Global Variables
extern SystemConfig config;
extern ControlMode currentMode;
extern unsigned long manualOverrideUntil;

// Configuration Functions
bool loadConfig();
bool saveConfig();
void printConfig();

#endif