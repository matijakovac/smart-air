#include "sensors.h"
#include "config.h"
#include <math.h>

SensorManager::SensorManager() 
  : bmp_internal(&Wire), bmp_external(&Wire) {
}

void SensorManager::selectMuxChannel(uint8_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(MUX_ADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();
  delay(10);
}

bool SensorManager::begin() {
  bool success = true;
  
  // Initialize internal I2C bus (Wire) with multiplexer
  Wire.begin(PIN_INTERNAL_SDA, PIN_INTERNAL_SCL);
  Wire.setClock(100000);
  delay(100);
  
  // Initialize internal sensors on MUX channel 0
  selectMuxChannel(MUX_CHANNEL_INTERNAL);
  
  if (!aht_internal.begin(&Wire)) {
    Serial.println("❌ Internal AHT20 not found");
    success = false;
  } else {
    Serial.println("✓ Internal AHT20 initialized");
  }
  
  if (!bmp_internal.begin(BMP280_ADDR_INTERNAL)) {
    Serial.println("❌ Internal BMP280 not found");
    success = false;
  } else {
    Serial.println("✓ Internal BMP280 initialized");
    bmp_internal.setSampling(Adafruit_BMP280::MODE_NORMAL,
                            Adafruit_BMP280::SAMPLING_X2,
                            Adafruit_BMP280::SAMPLING_X16,
                            Adafruit_BMP280::FILTER_X16,
                            Adafruit_BMP280::STANDBY_MS_500);
  }
  
  // Initialize external sensors on MUX channel 1
  selectMuxChannel(MUX_CHANNEL_EXTERNAL);
  
  if (!aht_external.begin(&Wire)) {
    Serial.println("❌ External AHT20 not found");
    success = false;
  } else {
    Serial.println("✓ External AHT20 initialized");
  }
  
  if (!bmp_external.begin(BMP280_ADDR_EXTERNAL)) {
    Serial.println("❌ External BMP280 not found");
    success = false;
  } else {
    Serial.println("✓ External BMP280 initialized");
    bmp_external.setSampling(Adafruit_BMP280::MODE_NORMAL,
                            Adafruit_BMP280::SAMPLING_X2,
                            Adafruit_BMP280::SAMPLING_X16,
                            Adafruit_BMP280::FILTER_X16,
                            Adafruit_BMP280::STANDBY_MS_500);
  }
  
  return success;
}

void SensorManager::update() {
  readInternalSensors();
  readExternalSensors();
}

void SensorManager::readInternalSensors() {
  selectMuxChannel(MUX_CHANNEL_INTERNAL);
  sensors_event_t humidity, temp;
  
  if (aht_internal.getEvent(&humidity, &temp)) {
    internal.temperature = temp.temperature;
    internal.humidity = humidity.relative_humidity;
    internal.pressure = bmp_internal.readPressure() / 100.0;
    internal.dewPoint = calculateDewPoint(internal.temperature, internal.humidity);
    internal.valid = true;
    internal.lastUpdate = millis();
  } else {
    Serial.println("⚠️ Failed to read internal sensors");
    internal.valid = false;
  }
}

void SensorManager::readExternalSensors() {
  selectMuxChannel(MUX_CHANNEL_EXTERNAL);
  sensors_event_t humidity, temp;
  
  if (aht_external.getEvent(&humidity, &temp)) {
    external.temperature = temp.temperature;
    external.humidity = humidity.relative_humidity;
    external.pressure = bmp_external.readPressure() / 100.0;
    external.dewPoint = calculateDewPoint(external.temperature, external.humidity);
    external.valid = true;
    external.lastUpdate = millis();
  } else {
    Serial.println("⚠️ Failed to read external sensors");
    external.valid = false;
  }
}

float SensorManager::calculateDewPoint(float temp, float humidity) {
  // Magnus-Tetens formula for dew point
  const float a = 17.27;
  const float b = 237.7;
  
  if (humidity <= 0.0 || humidity > 100.0) {
    return 0.0;
  }
  
  float alpha = ((a * temp) / (b + temp)) + log(humidity / 100.0);
  float dewPoint = (b * alpha) / (a - alpha);
  
  return dewPoint;
}

bool SensorManager::isDataFresh(unsigned long maxAge) const {
  unsigned long now = millis();
  bool internalFresh = internal.valid && (now - internal.lastUpdate < maxAge);
  bool externalFresh = external.valid && (now - external.lastUpdate < maxAge);
  return internalFresh && externalFresh;
}