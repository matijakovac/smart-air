#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <LittleFS.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include "config.h"
#include "sensors.h"
#include "fancontrol.h"
#include "display.h"
#include "webserver.h"
#include "mqtt_client.h"

// Global instances
SystemConfig config;
ControlMode currentMode = MODE_AUTO;
unsigned long manualOverrideUntil = 0;

SensorManager sensors;
FanController fanController;
DisplayManager display;
WebServerManager* webServer = nullptr;
MQTTManager* mqttManager = nullptr;

// Timing variables
unsigned long lastSensorRead = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastDecision = 0;
unsigned long lastMQTTPublish = 0;

void setupWiFi() {
  Serial.println("\n🌐 Connecting to WiFi...");
  display.showMessage("WiFi", "Connecting...");
  
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(config.hostname.c_str());
  WiFi.begin(config.wifi_ssid.c_str(), config.wifi_password.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("✓ WiFi connected");
    Serial.printf("  IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("  Signal: %d dBm\n", WiFi.RSSI());
    
    // Setup mDNS
    if (MDNS.begin(config.hostname.c_str())) {
      Serial.printf("  mDNS: http://%s.local\n", config.hostname.c_str());
      MDNS.addService("http", "tcp", 80);
    }
    
    // Configure NTP for CET/CEST (Ljubljana timezone)
    configTime(3600, 3600, "pool.ntp.org", "time.nist.gov");
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();
    Serial.println("  NTP time sync started (Europe/Ljubljana)");
    
    display.showMessage("WiFi OK", WiFi.localIP().toString().c_str());
    
  } else {
    Serial.println("❌ WiFi connection failed - continuing without WiFi");
    display.showMessage("WiFi Failed", "Offline mode");
  }
  
  delay(2000);
}

void setupOTA() {
  ArduinoOTA.setHostname(config.hostname.c_str());
  ArduinoOTA.setPassword("cellar2024"); // Change this!
  
  ArduinoOTA.onStart([]() {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "firmware" : "filesystem";
    Serial.println("\n🔄 OTA Update started: " + type);
    display.clear();
    display.showMessage("OTA Update", type.c_str(), "Please wait...");
    
    // Stop fan during update
    fanController.setMode(MODE_MANUAL_OFF);
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\n✓ OTA Update complete!");
    display.showMessage("OTA Complete", "Rebooting...");
    delay(1000);
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    static unsigned int lastPct = 0;
    unsigned int pct = (progress / (total / 100));
    if (pct != lastPct && pct % 10 == 0) {
      Serial.printf("Progress: %u%%\n", pct);
      char msg[20];
      snprintf(msg, sizeof(msg), "Progress: %u%%", pct);
      display.showMessage("OTA Update", msg);
      lastPct = pct;
    }
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("❌ OTA Error[%u]: ", error);
    const char* errMsg = "Unknown Error";
    if (error == OTA_AUTH_ERROR) errMsg = "Auth Failed";
    else if (error == OTA_BEGIN_ERROR) errMsg = "Begin Failed";
    else if (error == OTA_CONNECT_ERROR) errMsg = "Connect Failed";
    else if (error == OTA_RECEIVE_ERROR) errMsg = "Receive Failed";
    else if (error == OTA_END_ERROR) errMsg = "End Failed";
    Serial.println(errMsg);
    display.showMessage("OTA Error", errMsg);
    delay(3000);
  });
  
  ArduinoOTA.begin();
  Serial.println("✓ OTA update ready");
}

void printSystemInfo() {
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║   Smart Cellar Ventilation System     ║");
  Serial.println("║            Version 1.0.0               ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println();
  Serial.printf("ESP32 Chip: %s\n", ESP.getChipModel());
  Serial.printf("CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("Flash Size: %d bytes\n", ESP.getFlashChipSize());
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  printSystemInfo();
  
  // Initialize LittleFS
  Serial.println("📁 Initializing filesystem...");
  if (!LittleFS.begin(true)) {
    Serial.println("❌ LittleFS mount failed");
    // Continue anyway with defaults
  } else {
    Serial.println("✓ LittleFS mounted");
    Serial.printf("  Total: %d bytes\n", LittleFS.totalBytes());
    Serial.printf("  Used: %d bytes\n", LittleFS.usedBytes());
  }
  
  // Load configuration
  Serial.println("\n⚙️  Loading configuration...");
  if (loadConfig()) {
    Serial.println("✓ Configuration loaded from file");
    printConfig();
  } else {
    Serial.println("⚠️  Using default configuration");
  }
  
  // Initialize display first for status messages
  Serial.println("\n🖥️  Initializing display...");
  if (display.begin()) {
    Serial.println("✓ Display initialized");
    display.showMessage("Cellar Fan", "Initializing...");
  } else {
    Serial.println("❌ Display init failed");
  }
  delay(1000);
  
  // Initialize sensors
  Serial.println("\n📊 Initializing sensors...");
  display.showMessage("Sensors", "Starting...");
  if (sensors.begin()) {
    Serial.println("✓ All sensors initialized");
  } else {
    Serial.println("⚠️  Some sensors failed to initialize");
  }
  delay(1000);
  
  // Initialize fan controller
  Serial.println("\n💨 Initializing fan controller...");
  display.showMessage("Fan Control", "Starting...");
  if (fanController.begin()) {
    Serial.println("✓ Fan controller initialized");
  } else {
    Serial.println("❌ Fan controller init failed");
  }
  delay(1000);
  
  // Connect to WiFi
  setupWiFi();
  
  // Setup OTA updates
  if (WiFi.status() == WL_CONNECTED) {
    setupOTA();
    
    // Initialize web server
    Serial.println("\n🌐 Starting web server...");
    webServer = new WebServerManager(sensors, fanController);
    webServer->begin();
    
    // Initialize MQTT if enabled
    if (config.mqtt_enabled) {
      Serial.println("\n📡 Starting MQTT client...");
      mqttManager = new MQTTManager(sensors, fanController);
      mqttManager->begin();
    }
  }
  
  Serial.println("\n✓✓✓ System Ready ✓✓✓\n");
  display.showMessage("System Ready", WiFi.localIP().toString().c_str());
  delay(3000);
}

void loop() {
  unsigned long now = millis();
  
  // Handle OTA updates
  ArduinoOTA.handle();
  
  // Handle MQTT
  if (mqttManager && config.mqtt_enabled) {
    mqttManager->loop();
  }
  
  // Read sensors periodically
  if (now - lastSensorRead >= SENSOR_READ_INTERVAL) {
    sensors.update();
    lastSensorRead = now;
  }
  
  // Run fan control logic
  if (now - lastDecision >= DECISION_INTERVAL) {
    SensorData internal = sensors.getInternalData();
    SensorData external = sensors.getExternalData();
    
    // Check manual override timeout
    if (currentMode != MODE_AUTO && manualOverrideUntil > 0) {
      if (now >= manualOverrideUntil) {
        Serial.println("⏰ Manual override expired, returning to AUTO");
        currentMode = MODE_AUTO;
        manualOverrideUntil = 0;
      }
    }
    
    // Update fan controller
    fanController.update(internal, external);
    lastDecision = now;
  }
  
  // Update display
  if (now - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
    SensorData internal = sensors.getInternalData();
    SensorData external = sensors.getExternalData();
    display.update(internal, external, fanController);
    lastDisplayUpdate = now;
  }
  
  // Small delay to prevent watchdog issues
  delay(10);
}