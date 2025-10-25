#include "display.h"
#include "config.h"

DisplayManager::DisplayManager() 
  : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, OLED_RESET) {
  lastUpdate = 0;
  currentPage = 0;
}

bool DisplayManager::begin() {
  // Initialize I2C for OLED on Wire1 (separate bus from sensors)
  // OLED on pins 4 (SDA) and 15 (SCL)
  Wire1.begin(PIN_OLED_SDA, PIN_OLED_SCL);
  Wire1.setClock(400000);
  delay(50);
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("❌ SSD1306 allocation failed");
    return false;
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Cellar Ventilation");
  display.println("Initializing...");
  display.display();
  
  return true;
}

void DisplayManager::update(const SensorData& internal, const SensorData& external, const FanController& fan) {
  display.clearDisplay();
  drawMainScreen(internal, external, fan);
  display.display();
}

void DisplayManager::drawMainScreen(const SensorData& internal, const SensorData& external, const FanController& fan) {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Header
  display.setCursor(0, 0);
  display.print("CELLAR FAN");
  
  // WiFi status
  if (WiFi.status() == WL_CONNECTED) {
    int rssi = WiFi.RSSI();
    display.setCursor(100, 0);
    if (rssi > -60) display.print("|||");
    else if (rssi > -70) display.print("|| ");
    else display.print("|  ");
  }
  
  // Draw line
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  
  // Internal conditions
  display.setCursor(0, 14);
  display.print("In:");
  if (internal.valid) {
    display.setCursor(24, 14);
    display.print(internal.temperature, 1);
    display.print("C ");
    display.print((int)internal.humidity);
    display.print("%");
  } else {
    display.print(" ERROR");
  }
  
  // External conditions
  display.setCursor(0, 24);
  display.print("Out:");
  if (external.valid) {
    display.setCursor(24, 24);
    display.print(external.temperature, 1);
    display.print("C ");
    display.print((int)external.humidity);
    display.print("%");
  } else {
    display.print(" ERROR");
  }
  
  // Draw line
  display.drawLine(0, 34, 127, 34, SSD1306_WHITE);
  
  // Fan status
  display.setCursor(0, 38);
  int speed = fan.getCurrentSpeed();
  if (speed > 0) {
    display.setTextSize(2);
    display.print("ON ");
    display.print(speed);
    display.print("%");
  } else {
    display.setTextSize(2);
    display.print("OFF");
  }
  
  // Status text
  display.setTextSize(1);
  display.setCursor(0, 56);
  String status = fan.getStatusText();
  if (status.length() > 21) {
    status = status.substring(0, 21);
  }
  display.print(status);
  
  // Mode indicator
  if (currentMode != MODE_AUTO) {
    display.setCursor(110, 56);
    display.print("MAN");
  }
}

void DisplayManager::showMessage(const char* line1, const char* line2, const char* line3) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  display.setCursor(0, 10);
  display.println(line1);
  
  if (line2) {
    display.setCursor(0, 30);
    display.println(line2);
  }
  
  if (line3) {
    display.setCursor(0, 45);
    display.println(line3);
  }
  
  display.display();
}

void DisplayManager::clear() {
  display.clearDisplay();
  display.display();
}