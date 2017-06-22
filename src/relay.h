#ifndef SORELAY_H
#define SORELAY_H

//#include "sonoff.h"
#if (ARDUINO >= 100)
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

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


class sonoffRelay {
  public:
    // Constructors
    sonoffRelay();
    sonoffRelay(uint8_t pin);
    ~sonoffRelay();

    // Gets
    const int getPin(){return _pin;}
    const boolean getState(){return _on;}

    // Sets
    void setPin(uint8_t pin);
    void setStyle(gpio_style_T style);
    const void setCooldown(unsigned long ms);

    // Actions
    const boolean switchOn();
    const boolean switchOff();
    const boolean toggleRelay();

    // Callbacks
    const void onChange( void (*func) (boolean state) );

  private:
    uint8_t _pin;
    gpio_style_T _gpioStyle = SO_REG;
    boolean _on;

    static unsigned long _cooldown;
    unsigned long _cooldownTimer = 0;

    void (*_onChange)(boolean state);
};

#endif
