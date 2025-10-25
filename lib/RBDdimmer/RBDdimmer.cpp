// This file is deprecated - use rbdimmerESP32.h instead
#include "RBDdimmer.h"

static dimmerLamp* dimmer_instance;
static volatile int dimmer_value = 0;
static volatile int toggle_value = 0;
static volatile int dim_mode = 0;
static int freq_step = 75;

void IRAM_ATTR zero_cross_isr() {
  if (dim_mode == NORMAL_MODE) {
    if (dimmer_value > 0) {
      delayMicroseconds(dimmer_value * freq_step);
      digitalWrite(dimmer_instance->dimmer_pin, HIGH);
      delayMicroseconds(10);
      digitalWrite(dimmer_instance->dimmer_pin, LOW);
    }
  } else {
    if (toggle_value > 0) {
      digitalWrite(dimmer_instance->dimmer_pin, HIGH);
      delayMicroseconds(10);
      digitalWrite(dimmer_instance->dimmer_pin, LOW);
      delayMicroseconds(toggle_value * freq_step);
    }
  }
}

dimmerLamp::dimmerLamp(int user_dimmer_pin, int zc_dimmer_pin) {
  dimmer_pin = user_dimmer_pin;
  zc_pin = zc_dimmer_pin;
  dimmer_instance = this;
  toggle_min = 1;
  toggle_max = 100;
}

void dimmerLamp::begin(int mode, int on_off) {
  dimmer_mode = mode;
  dim_mode = mode;
  state_val = on_off;
  
  pinMode(dimmer_pin, OUTPUT);
  pinMode(zc_pin, INPUT_PULLUP);
  
  attachInterrupt(digitalPinToInterrupt(zc_pin), zero_cross_isr, RISING);
}

void dimmerLamp::setPower(int power) {
  if (power >= 0 && power <= 100) {
    power_val = power;
    dimmer_value = 100 - power;
    
    if (dimmer_mode == TOGGLE_MODE) {
      if (power == 0) {
        toggle_value = 0;
      } else {
        toggle_value = map(power, 0, 100, toggle_min, toggle_max);
      }
    }
  }
}

int dimmerLamp::getPower() {
  return power_val;
}

void dimmerLamp::setState(int on_off) {
  state_val = on_off;
  if (on_off == OFF) {
    dimmer_value = 0;
    toggle_value = 0;
  } else {
    dimmer_value = 100 - power_val;
  }
}

int dimmerLamp::getState() {
  return state_val;
}

void dimmerLamp::changeState() {
  state_val = !state_val;
  setState(state_val);
}

void dimmerLamp::toggleSettings(int minValue, int maxValue) {
  toggle_min = minValue;
  toggle_max = maxValue;
}