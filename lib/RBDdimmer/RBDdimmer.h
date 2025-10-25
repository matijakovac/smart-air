#ifndef RBDIMMER_H
#define RBDIMMER_H

#include <Arduino.h>

#define NORMAL_MODE 0
#define TOGGLE_MODE 1

#define ON  1
#define OFF 0

class dimmerLamp {
  public:
    dimmerLamp(int user_dimmer_pin, int zc_dimmer_pin);
    void begin(int mode, int on_off);
    void setPower(int power);
    int getPower();
    void setState(int on_off);
    int getState();
    void changeState();
    void toggleSettings(int minValue, int maxValue);
    
    int dimmer_pin;  // Made public for ISR access
    
  private:
    int zc_pin;
    int toggle_state;
    int dimmer_mode;
    int power_val;
    int state_val;
    int toggle_min;
    int toggle_max;
};

#endif