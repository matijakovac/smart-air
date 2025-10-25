#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>

struct SensorData {
  float temperature;
  float humidity;
  float pressure;
  float dewPoint;
  bool valid;
  unsigned long lastUpdate;
  
  SensorData() : temperature(0), humidity(0), pressure(0), 
                 dewPoint(0), valid(false), lastUpdate(0) {}
};

class SensorManager {
public:
  SensorManager();
  bool begin();
  void update();
  
  SensorData getInternalData() const { return internal; }
  SensorData getExternalData() const { return external; }
  
  static float calculateDewPoint(float temp, float humidity);
  bool isDataFresh(unsigned long maxAge = 30000) const;
  
private:
  Adafruit_AHTX0 aht_internal;
  Adafruit_AHTX0 aht_external;
  Adafruit_BMP280 bmp_internal;
  Adafruit_BMP280 bmp_external;
  
  SensorData internal;
  SensorData external;
  
  void selectMuxChannel(uint8_t channel);
  void readInternalSensors();
  void readExternalSensors();
};

#endif