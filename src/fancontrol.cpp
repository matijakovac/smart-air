#include "fancontrol.h"
#include <time.h>

FanController::FanController() {
  currentSpeed = 0;
  runReason = REASON_OFF;
  lastStateChange = 0;
  lastForcedRun = 0;
  relayState = false;
  forcedRunActive = false;
  forcedRunStart = 0;
  dimmerChannel = nullptr;
}

bool FanController::begin() {
  // Initialize relay (active LOW)
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, HIGH); // OFF
  Serial.println("✓ Relay initialized (OFF)");
  
  // Initialize RBDimmer library
  rbdimmer_err_t err = rbdimmer_init();
  if (err != RBDIMMER_OK) {
    Serial.printf("❌ RBDimmer init failed: %d\n", err);
    return false;
  }
  
  // Register zero-cross detector (50Hz expected, 0 = auto-detect)
  err = rbdimmer_register_zero_cross(PIN_DIMMER_ZC, 0, 50);
  if (err != RBDIMMER_OK) {
    Serial.printf("❌ Zero-cross registration failed: %d\n", err);
    return false;
  }
  
  // Wait for frequency detection
  Serial.print("⏳ Detecting AC frequency");
  for (int i = 0; i < 30; i++) {
    delay(100);
    Serial.print(".");
    uint16_t freq = rbdimmer_get_frequency(0);
    if (freq > 0) {
      Serial.printf(" detected: %d Hz\n", freq);
      break;
    }
  }
  
  // Create dimmer channel with LINEAR curve (best for motors)
  rbdimmer_config_t dimmer_config = {
    .gpio_pin = PIN_DIMMER_PSM,
    .phase = 0,
    .initial_level = 0,
    .curve_type = RBDIMMER_CURVE_LINEAR
  };
  
  err = rbdimmer_create_channel(&dimmer_config, &dimmerChannel);
  if (err != RBDIMMER_OK) {
    Serial.printf("❌ Dimmer channel creation failed: %d\n", err);
    return false;
  }
  
  rbdimmer_set_active(dimmerChannel, true);
  rbdimmer_set_level(dimmerChannel, 0);
  
  Serial.println("✓ Dimmer initialized");
  return true;
}

void FanController::update(const SensorData& internal, const SensorData& external) {
  // Validate sensor data
  if (!internal.valid || !external.valid) {
    Serial.println("⚠️ Invalid sensor data - skipping control logic");
    return;
  }
  
  // Handle manual override modes
  if (currentMode == MODE_MANUAL_OFF) {
    setFanSpeed(0, REASON_MANUAL_OVERRIDE);
    return;
  } else if (currentMode == MODE_MANUAL_LOW) {
    setFanSpeed(config.low_speed, REASON_MANUAL_OVERRIDE);
    return;
  } else if (currentMode == MODE_MANUAL_HIGH) {
    setFanSpeed(config.high_speed, REASON_MANUAL_OVERRIDE);
    return;
  } else if (currentMode == MODE_DIAGNOSTIC) {
    // Diagnostic mode - manual control only
    return;
  }
  
  // AUTO MODE LOGIC
  
  // Priority 1: Check forced circulation
  if (checkForcedCirculation()) {
    if (!forcedRunActive) {
      Serial.println("🔄 Starting forced circulation");
      forcedRunActive = true;
      forcedRunStart = millis();
    }
    
    setFanSpeed(config.low_speed, REASON_FORCED_CIRCULATION);
    
    // Check if duration completed
    unsigned long elapsed = (millis() - forcedRunStart) / 60000; // minutes
    if (elapsed >= config.forced_duration_min) {
      Serial.println("✓ Forced circulation complete");
      forcedRunActive = false;
      lastForcedRun = millis();
    }
    return;
  }
  
  forcedRunActive = false;
  
  // Priority 2: Safety limits
  if (external.temperature < config.min_outside_temp) {
    Serial.printf("🛡️ Safety: Outside too cold (%.1f°C < %.1f°C)\n", 
                  external.temperature, config.min_outside_temp);
    setFanSpeed(0, REASON_SAFETY_LIMIT);
    return;
  }
  
  if (internal.temperature < config.min_cellar_temp) {
    Serial.printf("🛡️ Safety: Cellar too cold (%.1f°C < %.1f°C)\n",
                  internal.temperature, config.min_cellar_temp);
    setFanSpeed(0, REASON_SAFETY_LIMIT);
    return;
  }
  
  // Priority 3: Dew point protection
  if (!checkDewPointSafety(internal, external)) {
    Serial.println("🛡️ Safety: Dew point risk detected");
    setFanSpeed(0, REASON_SAFETY_LIMIT);
    return;
  }
  
  // Priority 4: Check if ventilation is beneficial
  if (shouldRun(internal, external)) {
    // Determine speed based on schedule
    int targetSpeed = isHighSpeedAllowed() ? config.high_speed : config.low_speed;
    
    // Determine reason
    RunReason reason = REASON_OFF;
    bool needsHumidity = (internal.humidity > config.target_humidity) && 
                         (external.humidity < internal.humidity - config.humidity_differential);
    bool needsTemp = (internal.temperature > config.target_temp) && 
                     (external.temperature < internal.temperature - config.temp_differential);
    
    if (needsHumidity && needsTemp) {
      reason = REASON_BOTH;
    } else if (needsHumidity) {
      reason = REASON_HUMIDITY;
    } else if (needsTemp) {
      reason = REASON_TEMPERATURE;
    }
    
    setFanSpeed(targetSpeed, reason);
  } else {
    setFanSpeed(0, REASON_OFF);
  }
}

bool FanController::shouldRun(const SensorData& internal, const SensorData& external) {
  // Check humidity benefit
  bool humidityBenefit = (internal.humidity > config.target_humidity) && 
                         (external.humidity < internal.humidity - config.humidity_differential);
  
  // Check temperature benefit
  bool tempBenefit = (internal.temperature > config.target_temp) && 
                     (external.temperature < internal.temperature - config.temp_differential);
  
  return humidityBenefit || tempBenefit;
}

bool FanController::isHighSpeedAllowed() const {
  time_t now;
  struct tm timeinfo;
  
  time(&now);
  localtime_r(&now, &timeinfo);
  
  int hour = timeinfo.tm_hour;
  int wday = timeinfo.tm_wday; // 0=Sunday, 1=Monday, ..., 6=Saturday
  
  // NEVER on weekends (Saturday=6, Sunday=0)
  if (wday == 0 || wday == 6) {
    return false;
  }
  
  // Friday (5): Only 22:00-23:59
  if (wday == 5) {
    return (hour >= 22);
  }
  
  // Monday-Thursday: 22:00-15:00 next day
  // This means: hour >= 22 OR hour < 15
  return (hour >= 22 || hour < 15);
}

bool FanController::checkDewPointSafety(const SensorData& internal, const SensorData& external) const {
  // Calculate what the dew point would be if external air warmed to internal temp
  // This is a simplified check - we're comparing dew points directly
  
  float dewPointIncrease = external.dewPoint - internal.dewPoint;
  
  // If bringing in air would increase dew point significantly, don't run
  if (dewPointIncrease > config.max_dew_point_increase) {
    return false; // Risk of condensation
  }
  
  return true;
}

bool FanController::checkForcedCirculation() {
  unsigned long intervalMs = config.forced_interval_hours * 3600000UL;
  unsigned long timeSinceLast = millis() - lastForcedRun;
  
  return (timeSinceLast >= intervalMs);
}

void FanController::setFanSpeed(int speed, RunReason reason) {
  // Anti-short-cycle protection
  if (speed != currentSpeed && !canChangeState()) {
    return; // Too soon to change state
  }
  
  // Update speed if changed
  if (speed != currentSpeed) {
    currentSpeed = speed;
    runReason = reason;
    lastStateChange = millis();
    
    // Set relay
    setRelay(speed > 0);
    
    // Set dimmer
    if (dimmerChannel) {
      rbdimmer_set_level(dimmerChannel, speed);
    }
    
    // Log change
    Serial.printf("💨 Fan: %d%% - %s\n", speed, getStatusText().c_str());
  }
}

void FanController::setRelay(bool state) {
  if (state != relayState) {
    digitalWrite(PIN_RELAY, state ? LOW : HIGH); // Active LOW
    relayState = state;
  }
}

bool FanController::canChangeState() const {
  unsigned long elapsed = (millis() - lastStateChange) / 1000; // seconds
  
  if (currentSpeed > 0) {
    // Currently running - check min run time
    return elapsed >= config.min_run_time_sec;
  } else {
    // Currently off - check min idle time
    return elapsed >= config.min_idle_time_sec;
  }
}

void FanController::setMode(ControlMode mode, unsigned long durationMin) {
  currentMode = mode;
  
  if (durationMin > 0) {
    manualOverrideUntil = millis() + (durationMin * 60000UL);
    Serial.printf("⚙️ Mode set to %d for %lu minutes\n", mode, durationMin);
  } else {
    manualOverrideUntil = 0;
    Serial.printf("⚙️ Mode set to %d (permanent)\n", mode);
  }
}

void FanController::setManualSpeed(int speed) {
  if (speed < 0) speed = 0;
  if (speed > 100) speed = 100;
  
  if (dimmerChannel) {
    rbdimmer_set_level(dimmerChannel, speed);
    currentSpeed = speed;
    setRelay(speed > 0);
    Serial.printf("🎛️ Manual speed set: %d%%\n", speed);
  }
}

String FanController::getStatusText() const {
  switch (runReason) {
    case REASON_OFF: return "Off";
    case REASON_HUMIDITY: return "Dehumidifying";
    case REASON_TEMPERATURE: return "Cooling";
    case REASON_BOTH: return "Cooling & Dehumid";
    case REASON_FORCED_CIRCULATION: return "Forced Circulation";
    case REASON_MANUAL_OVERRIDE: return "Manual Override";
    case REASON_SAFETY_LIMIT: return "Safety Limit";
    default: return "Unknown";
  }
}

unsigned long FanController::getNextForcedRun() const {
  if (forcedRunActive) {
    return 0; // Currently running
  }
  
  unsigned long intervalMs = config.forced_interval_hours * 3600000UL;
  unsigned long timeSinceLast = millis() - lastForcedRun;
  
  if (timeSinceLast >= intervalMs) {
    return 0; // Due now
  }
  
  return (intervalMs - timeSinceLast) / 60000; // Return minutes until next run
}

unsigned long FanController::getTimeSinceStateChange() const {
  return (millis() - lastStateChange) / 1000; // seconds
}