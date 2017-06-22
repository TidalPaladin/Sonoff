#include "button.h"

boolean sonoffButton::_hasIsr = false;

sonoffButton::sonoffButton(){}

sonoffButton::sonoffButton(uint8_t p) {
  this->_pin = p;
  pinMode(_pin,INPUT);
  _previousButtonState = digitalRead(this->_pin);
}

sonoffButton::~sonoffButton() {
  // detachInterrupt

}

const void sonoffButton::setPin(uint8_t pin) {

  _pin = pin;
  pinMode(_pin,INPUT);
  attachInterrupt(_pin,_ISRHandle,CHANGE);
}


/*
  Called when _pin is held longer than BUTTON_HOLD_DURATION
  Forgets wifi networks and resets the chip
*/
const so_actions_t sonoffButton::_defaultHoldCallback(){
  WiFi.disconnect();  // Clear Wifimanager memory
  ESP.restart();
  ESP.reset();
}

/*
  Called when button is pressed
*/
const so_actions_t sonoffButton::_defaultPressCallback(){
  return SO_RELAY_TOGGLE;
}

// All SONOFFs have this as an ISR, it finds the appropriate ISR for each instance
void sonoffButton::_ISRHandle(){
  _hasIsr = true;
  return;
}

button_states_t sonoffButton::buttonISR(int state) {
  volatile static unsigned long pressTimer;
  volatile static unsigned long lastPress = 0; // Tracks time between button presses
  _previousButtonState = state;

  // Figure out if the button is being pressed or released
  if(!state){
    VERBOSE_PRINTLN("Button pressed down - GPIO "+String(this->_pin));
    pressTimer = millis();
    return DOWN;
  }
  else{VERBOSE_PRINTLN("Button released - GPIO "+String(this->_pin));}

  // Handle debouncing
  if( !_timeComp(lastPress,_refractoryPeriod) ) {
    VERBOSE_PRINTLN("ISR refractory period not satisfied");
    return BOUNCE;
  }

  // Figure out time difference between push down and up
  volatile unsigned long pulseLength = abs(millis() - pressTimer);
  VERBOSE_PRINTLN("Button held down for "+String(pulseLength)+" ms");

  // If user holds button for a while, run the hold callback
  if (pulseLength >= _buttonHoldDuration ){
    Serial.print("Detected button hold... ");
    this->_needsButtonHoldCallback = true;
  }
  else {  // Run the press callback
    Serial.print("Detected button press... ");
    this->_needsButtonPressCallback = true;
  }
  return UP;
}

void sonoffButton::detachInterrupts(){
  detachInterrupt(digitalPinToInterrupt(this->_pin));
  VERBOSE_PRINTLN("Interrupts detached on GPIO "+_pin);
}
//
// // TODO this can go
// void sonoffChannel::setButtonCallback(void (func)(void)){
//   attachInterrupt(digitalPinToInterrupt(this->_pin), func, CHANGE);
// }
//
// void sonoffButton::setRefractoryPeriod(unsigned long period) {
//   _refractoryPeriod = period;
//   VERBOSE_PRINTLN("Set button refractory period - "+String(period));
// }



so_actions_t sonoffButton::handle() {
  int state = digitalRead(_pin);
  if(_hasIsr && state != _previousButtonState) {
    this->buttonISR(state);
    _hasIsr = false;
  }

  if(_needsButtonPressCallback){
    VERBOSE_PRINTLN("Handling button press callback");
    _needsButtonPressCallback = false;
    return SO_RELAY_TOGGLE;
  }
  if(_needsButtonHoldCallback){
    VERBOSE_PRINTLN("Handling button hold callback");
    _needsButtonHoldCallback = false;
    return _defaultHoldCallback();
  }
  return SO_NONE;
}
