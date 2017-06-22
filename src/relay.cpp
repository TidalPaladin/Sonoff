#include "relay.h"

unsigned long sonoffRelay::_cooldown = 500;

sonoffRelay::sonoffRelay(){}

sonoffRelay::sonoffRelay(uint8_t p){
  this->_pin = p;
  pinMode(_pin,OUTPUT);
  switchOff();
}

sonoffRelay::~sonoffRelay(){
  switchOff();
}

void sonoffRelay::setPin(uint8_t pin){
  _pin = pin;
  pinMode(_pin,OUTPUT);
  switchOff();
}

void sonoffRelay::setStyle(gpio_style_T style) {
  _gpioStyle = style;
}

const boolean sonoffRelay::switchOn() {
  // Enforce a switch delay so that bad code cant flutter the relay
  if( abs(millis() - _cooldownTimer ) < _cooldown){
    Serial.println("Not switching relay because of cooldown");
    return _on;
  }

  // Write to the relay
  if(_gpioStyle == SO_REG){digitalWrite(this->_pin,HIGH);}
  else{digitalWrite(_pin,LOW);}
  _on = true;
  _cooldownTimer = millis();
  VERBOSE_PRINTLN("Relay switched ON");

  // Run callback if set
  if(_onChange){
    _onChange(_on);
  }
  return _on;
}

const boolean sonoffRelay::switchOff() {
  if(_gpioStyle == SO_REG){digitalWrite(_pin,LOW);}
  else{digitalWrite(_pin,HIGH);}
  _on = false;
  _cooldownTimer = millis();
  VERBOSE_PRINTLN("Relay switched OFF");

  // Run callback if set
  if(_onChange){
    _onChange(_on);
  }
  return _on;
}

const boolean sonoffRelay::toggleRelay() {
  Serial.print("Toggling relay to ");
  if(_on){
    Serial.println("off");
    return switchOff();
  }
  else{
    Serial.println("on");
    return switchOn();
  }
}

const void sonoffRelay::setCooldown(unsigned long ms){
  _cooldown = ms;
}

const void sonoffRelay::onChange( void (*func) (boolean state) ){
  _onChange = func;
}
