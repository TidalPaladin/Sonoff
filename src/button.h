#ifndef SOBUTTON_H
#define SOBUTTON_H

#if (ARDUINO >= 100)
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#include <ESP8266WiFi.h>


#ifndef VERBOSE_PRINTLN
  #ifdef VERBOSE
    #define VERBOSE_PRINT(str) Serial.print(str)
    #define VERBOSE_PRINTLN(str) Serial.println(str)
  #else
    #define VERBOSE_PRINT(str)
    #define VERBOSE_PRINTLN(str)
  #endif
#endif

#ifndef _timeComp
  #define _timeComp(t1,t2) abs(millis() - t1) >= t2
#endif


#ifndef GPIOSTYLE
  #define GPIOSTYLE
  typedef enum {
    SO_REG,
    SO_INVERTED
  } gpio_style_T;
#endif

#ifndef SOACTIONS
  #define SOACTIONS
  typedef enum {
    SO_NONE,
    SO_RELAY_ON,
    SO_RELAY_OFF,
    SO_RELAY_TOGGLE,
    SO_BUTTON_PRESS,
    SO_BUTTON_HOLD
  } so_actions_t;
#endif

typedef enum {
  DOWN,
  UP,
  BOUNCE
} button_states_t;

class sonoffButton {

  public:
    // Constructors
    sonoffButton();
    sonoffButton(uint8_t pin);
    ~sonoffButton();

    // Gets
    const int getPin(){return _pin;}

    // Sets
    const void setPin(uint8_t pin);
    void setButtonCallback(void (func)(void));
    void setReleaseCallback(void (func)(void));
    void setButtonHoldDuration(byte button, unsigned long duration);
    void setRefractoryPeriod(unsigned long period);

    // Actions
    void detachInterrupts();
    button_states_t buttonISR(int state); // Figures out if its a press or hold
    so_actions_t handle();

  private:
    uint8_t _pin;
    gpio_style_T _gpioStyle = SO_REG;



    static void _ISRHandle();
    static boolean _hasIsr;

    boolean _needsButtonPressCallback = false;
    boolean _needsButtonHoldCallback = false;
    const so_actions_t _defaultPressCallback();
    const so_actions_t _defaultHoldCallback();

    volatile unsigned long _refractoryPeriod = 100; // Debouncing period
    unsigned long _buttonHoldDuration = 5000;
    int _previousButtonState = HIGH;  // Tracks button presses in ISR
};

#endif
