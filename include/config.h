#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Hardware Pin Definitions
#define PIN_RELAY 5
#define PIN_DIMMER_ZC 22
#define PIN_DIMMER_PSM 21
#define PIN_INTERNAL_SDA 19
#define PIN_INTERNAL_SCL 18
// OLED on separate I2C (Wire1) - pins 25 (SDA) and 26 (SCL)
#define PIN_OLED_SDA 25
#define PIN_OLED_SCL 26

// I2C Addresses
#define BMP280_ADDR_INTERNAL 0x77
#define BMP280_ADDR_EXTERNAL 0x77
#define OLED_ADDR 0x3C
#define MUX_ADDR 0x70  // TCA9548A I2C Multiplexer

// Multiplexer Channels
#define MUX_CHANNEL_INTERNAL 0
#define MUX_CHANNEL_EXTERNAL 1

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
  bool use_static_ip;
  String static_ip;
  String gateway;
  String subnet;
  
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