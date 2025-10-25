#ifndef FANCONTROL_H
#define FANCONTROL_H

#include <Arduino.h>
#include "rbdimmerESP32.h"
#include "config.h"
#include "sensors.h"

class FanController {
public:
  FanController();
  bool begin();
  void update(const SensorData& internal, const SensorData& external);
  
  void setMode(ControlMode mode, unsigned long durationMin = 0);
  void setManualSpeed(int speed);
  
  int getCurrentSpeed() const { return currentSpeed; }
  RunReason getRunReason() const { return runReason; }
  String getStatusText() const;
  unsigned long getNextForcedRun() const;
  bool isForcedRunActive() const { return forcedRunActive; }
  unsigned long getTimeSinceStateChange() const;
  
private:
  rbdimmer_channel_t* dimmerChannel;
  int currentSpeed;
  RunReason runReason;
  unsigned long lastStateChange;
  unsigned long lastForcedRun;
  bool relayState;
  bool forcedRunActive;
  unsigned long forcedRunStart;
  
  bool shouldRun(const SensorData& internal, const SensorData& external);
  bool isHighSpeedAllowed() const;
  bool checkDewPointSafety(const SensorData& internal, const SensorData& external) const;
  bool checkForcedCirculation();
  void setFanSpeed(int speed, RunReason reason);
  void setRelay(bool state);
  bool canChangeState() const;
};

#endif