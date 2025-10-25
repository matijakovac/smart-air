#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "sensors.h"
#include "fancontrol.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

class DisplayManager {
public:
  DisplayManager();
  bool begin();
  void update(const SensorData& internal, const SensorData& external, const FanController& fan);
  void showMessage(const char* line1, const char* line2 = nullptr, const char* line3 = nullptr);
  void clear();
  
private:
  Adafruit_SSD1306 display;
  unsigned long lastUpdate;
  uint8_t currentPage;
  
  void drawMainScreen(const SensorData& internal, const SensorData& external, const FanController& fan);
  void drawDetailScreen(const SensorData& internal, const SensorData& external);
};

#endif