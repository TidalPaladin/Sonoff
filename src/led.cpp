#include "led.h"

sonoffLed::sonoffLed(){
  setMode(BLINK);
}

sonoffLed::sonoffLed(uint8_t p) {
  this->_pin = p;
  pinMode(_pin,OUTPUT);
  this->ledOff();
}

sonoffLed::~sonoffLed() {
  ledOff();
}

void sonoffLed::setPin(uint8_t pin) {
  _pin = pin;
  pinMode(_pin,OUTPUT);
  ledOff();
}

void sonoffLed::setStyle(gpio_style_T style) {
  _gpioStyle = style;
}

void sonoffLed::setMode(led_mode_T mode) {
  this->_mode = mode; // Set the mode
  _pulsePosition = 0; // Reset pulse position

  if( mode == BLINK) {
    _blinkWrap(this);
  }
  else if( mode == PULSE ){
    _actionTicker.attach_ms(REFRESH_RATE, pulseWrap, (void*)this );
  }
  else if( mode == OFF){
    _actionTicker.detach();
    ledOff();
  }
  else if( mode == SOLID){
    _actionTicker.detach();
    ledOn();
  }

  // Print debug info about what we've done
  VERBOSE_PRINT("Set SONOFF LED mode - ");
  if(mode == BLINK){VERBOSE_PRINTLN("BLINK");}
  else if(mode == PULSE){VERBOSE_PRINTLN("PULSE");}
  else if(mode == DIM){VERBOSE_PRINTLN("DIM");}
  else{
    VERBOSE_PRINTLN("Unrecognized mode, ignoring...");
  }
  return;
}

// void sonoffLed::_blink(){
//   if(!_on){
//     ledOn();
//     _actionTicker.once_ms(_blinkDuration,_)
//   }
// }

void sonoffLed::toggle(uint16_t power) {

  if(_on) {
    ledOff();
    VERBOSE_PRINTLN("Toggled LED off");
  }
  else{
    ledOn();  // TODO ledOn(power)?
    VERBOSE_PRINTLN("Toggled LED on");
  }

}

void sonoffLed::_blinkWrap(void* ptr){
  sonoffLed* self = (sonoffLed*) ptr;
  if(self->_on){
    self->ledOff();
    self->_actionTicker.once_ms(self->_blinkSpeed, _blinkWrap, (void*)self );
  }
  else {
    self->ledOn();
    self->_actionTicker.once_ms(self->_blinkDuration, _blinkWrap, (void*)self );
  }
}

void sonoffLed::pulseWrap(void* ptr){
  sonoffLed* self = (sonoffLed*) ptr;
  self->_pulse();
}

void sonoffLed::wrap(void (sonoffLed::*func)(void)){
  //func();
}

void sonoffLed::ledOn(uint16_t brightness) {
  if(brightness > this->_ledMaxBrightness){
    brightness = this->_ledMaxBrightness;
  }
  if(this->_gpioStyle == SO_REG) {
    analogWrite(this->_pin, brightness);
  }
  else {
    analogWrite(this->_pin, PWMRANGE-brightness);
  }
  _on = true;
}

void sonoffLed::ledOff(uint16_t brightness){
  if(this->_mode == DIM){brightness = this->_ledDimBrightness;}  // Handle DIM case

  if(this->_gpioStyle == SO_REG) {
    analogWrite(this->_pin,brightness);
  }
  else {
    analogWrite(this->_pin,PWMRANGE-brightness);
  }
  _on = false;
}

void sonoffLed::setMaxBrightness(uint16_t power){
  if(power <= PWMRANGE && power > 0){
    this->_ledMaxBrightness = power;
    VERBOSE_PRINTLN("Set LED max brightness - "+String(_ledMaxBrightness) );
  }
  else{
    VERBOSE_PRINTLN("Invalid arg passed to set LED max brightness");
    return;
  }

}

void sonoffLed::setDimBrightness(uint16_t power){
  if(power <= PWMRANGE && power >= 0){
    this->_ledDimBrightness = power;
    if(power >= this->_ledMaxBrightness){
      Serial.println("LED dim brighness set to a value >= ledMaxBrightness");
    }
    else {
      VERBOSE_PRINTLN("Set LED dim brightness - "+String(_ledDimBrightness) );
    }
  }
  else {
    VERBOSE_PRINTLN("Invalid arg passed to set LED dim brightness");
  }


}

void sonoffLed::setPulseSpeed(float step) {
  step = abs(step);
  _pulseSpeed = step;
  VERBOSE_PRINTLN("Set SONOFF LED pulse speed - " + String(_pulseSpeed) );
}

void sonoffLed::setBlinkSpeed(unsigned long t){
  _blinkSpeed = t;
  VERBOSE_PRINTLN("Set SONOFF LED pulse speed - " + _blinkSpeed);
}

void sonoffLed::setBlinkDuration(unsigned long t){
  this->_blinkDuration = t;
  VERBOSE_PRINTLN("Set SONOFF LED blink duration - " + _blinkDuration);
}

/*
  Handles pusling of a LED
  Each function call will iterate brightness
  Passing step = 0 will mean no iteration
  Passing step = -1 will set brightness to 0
*/
void sonoffLed::_pulse(){


  // // If passed -1, turn off the LED and reset start position
  // // if(step == -1) {
  // //   ledOff();
  // //   this->_pulsePosition = 0;
  // //   return;
  // // }
  // // If passed a 0, just leave the function
  // else if(!step){
  //   return;
  // }

  /*
    MATH: Start at pi/2, makes sine 1. Move on [0,pi] from 0->1->0
    abs() function not necessary, just to help
  */
  float out;
  float amp = (float)_ledMaxBrightness;

  _pulsePosition = _pulsePosition + _pulseSpeed; // Step forward
  if (_pulsePosition > 3.14)  // If position > pi, reset to 0
    _pulsePosition = 0;
  out = abs( amp * sin(_pulsePosition) );  // Pass radians to sine function
  this->ledOn(out);
}


// void sonoffLed::handle() {
//   // TODO maybe an if here to take different action if error blinks are needed
//   switch(_mode) {
//
//     case BLINK:
//
//       // TODO do we really need this synchronous blink handing?
//       if(_blinkDuration < 50){ // Handle short blinks synchronously
//         ledOn(_ledMaxBrightness);
//         delay(_blinkDuration);
//         ledOff();
//         _ledTimer = millis();
//       }
//       else if( _timeComp(_ledTimer, _blinkSpeed + _blinkDuration) ){
//           ledOff();
//           _ledTimer = millis();
//       }
//       else if( _timeComp(_ledTimer, _blinkSpeed) && !_on ){
//           ledOn();
//       }
//       break;
//
//     case PULSE:
//       // Every 10ms step forward an increment of brightness (100 Hz)
//       if( _timeComp(_ledTimer,10) ) {
//         _pulse();
//         _ledTimer = millis();
//       }
//       break;
//   }
// }
