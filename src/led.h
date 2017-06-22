#ifndef SOLED_H
#define SOLED_H

#if (ARDUINO >= 100)
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#include <Ticker.h>

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

// Here are some defaults for pulseLED step size
#ifndef SOLEDSTATES
  #define SOLEDSTATES
  #define REFRESH_RATE  10  // Refresh every 10 ms
  #define SO_LED_SLOW    0.005
  #define SO_LED_MEDIUM  0.01
  #define SO_LED_FAST    0.05
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
  BLINK,
  PULSE,
  DIM,
  SOLID,
  OFF
} led_mode_T;

class sonoffLed {
  public:

    // Constructors
    sonoffLed();
    sonoffLed(uint8_t pin);
    ~sonoffLed();

    // Gets
    const int getPin(){return _pin;}
    const bool getState(){return _on;}
    const led_mode_T getMode(){return _mode;}

    // Sets
    void setPin(uint8_t pin);
    void setStyle(gpio_style_T style);

    void setMode(led_mode_T mode);
    void setPulseSpeed(float step);  // pulseSpeed
    void setBlinkSpeed(unsigned long t); // blinkSpeed
    void setBlinkDuration(unsigned long t); // blinkDuration
    void setMaxBrightness(uint16_t power);  // ledMaxBrightness
    void setDimBrightness(uint16_t power);  // ledDimBrightness

    //Actions
    void toggle(uint16_t pow = PWMRANGE);
    void ledOn(uint16_t brightness = PWMRANGE);
    void ledOff(uint16_t brightness = 0);
    void _pulse();

    // Action wrappers
    static void wrap(void (sonoffLed::*pt2Function)(void));
    static void toggleWrap(void *ptr);
    static void pulseWrap(void *ptr);

    void handle();
    //String getSettings();

  private:
    uint8_t _pin;
    gpio_style_T _gpioStyle = SO_INVERTED;
    boolean _on;

    led_mode_T _mode = BLINK;
    led_mode_T _previousMode;

    float _pulsePosition = 0; //Tracks current brightness
    float _pulseSpeed = SO_LED_MEDIUM;
    uint16_t _ledMaxBrightness = PWMRANGE; // The analogWrite() maximum
    uint16_t _ledDimBrightness = 20;

    Ticker _actionTicker;
    static void _blinkWrap(void *ptr);
    Ticker _actionTicker2;

    unsigned long _blinkSpeed = 3000;  // Interval in ms between blinks
    unsigned long _blinkDuration = 200;  // LED time on during blink
    unsigned long _ledTimer = millis();
};

#endif
