#ifndef SONOFF_H
#define SONOFF_H

#if (ARDUINO >= 100)
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#define VERBOSE   // Print verbose debug info

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h>

#include <Hash.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266WiFiMulti.h>
#include <algorithm>
#include <Ticker.h>
#include <AsyncMqttClient.h>

#include "mqtt.h"
#include "wemo.h"
#include "led.h"
#include "relay.h"
#include "button.h"



// Verbose debugging
#ifndef VERBOSE_PRINTLN
  #ifdef VERBOSE
    #define VERBOSE_PRINT(str) Serial.print(str)
    #define VERBOSE_PRINTLN(str) Serial.println(str)
  #else
    #define VERBOSE_PRINT(str)
    #define VERBOSE_PRINTLN(str)
  #endif
#endif

#ifndef CHMAPS
  #define CHMAPS
  typedef struct{
    uint8_t relay;
    uint8_t button;
    uint8_t led;
  } pinmap_T;

  typedef enum {
    SO_CH1,
    SO_CH2,
    SO_CH3,
    SO_CH4,
    SO_NODEMCU,
    SO_SV
  } sonoff_ch_T;
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

class sonoffChannel;

class SONOFF {
  public:

    // Constructor
    SONOFF();

    //Gets

    //Sets
    void setMqttServer(const char* host, uint16_t port=1883){
      sonoffMqtt::setServer(host,port);
      //sonoffMqtt::connect();
    }
    void setMqttServer(IPAddress addr, uint16_t port = 1883){
      sonoffMqtt::setServer(addr,port);
      //sonoffMqtt::connect();
    }
    void setHostname(const char* hostname);
    static void sonoffOTA(const char* pass);
    static void attachWifiManager(const char* ssid=NULL, const char* pass=NULL, unsigned long seconds=0);
    static void setWifiManagerTimeout(unsigned long seconds);


    // Actions
    static void wifiManagerConnect();
    void allRelaysOff();
    void allLEDsOff();
    void addChannel(sonoffChannel *channel);
    static void _payloadSearch(char* topic, byte* payload, unsigned int length);

    static void handle();
    void printAll();

    static std::vector<sonoffChannel*> activeChannels;
    static const void vectorIterator(void function(sonoffChannel *channel) );

    static const char* _hostname;


    // Default pinmaps
    static const pinmap_T _pinmapCh1;
    static const pinmap_T _pinmapCh2;
    static const pinmap_T _pinmapCh3;
    static const pinmap_T _pinmapCh4;
    static const pinmap_T _pinmapSV;
    static const pinmap_T _pinmapNodemcu;

  private:
    // Struct to hold all usual channels
    static std::vector<sonoffChannel&> defaultChannels;
    static ESP8266WebServer _statusServer;

    // WifiManager / OTA
    static WiFiManager _wifiManager;
    static boolean _usingWifiManager;
    static void _handleWifiManager();
    static char _wifiManagerSSID[64];
    static char _wifiManagerPW[64];

    static WiFiEventHandler _wifiConnectHandler;
    static WiFiEventHandler _wifiDisconnectHandler;
    //static void onWifiDisconnect();

    static boolean _usingOTA;
    static Ticker _otaHandle;

};

class sonoffChannel : public SONOFF {
 public:
   typedef std::function<void(void)> callbackFunction;

   // Constructors
   sonoffChannel();
   sonoffChannel(uint8_t relay, uint8_t button, uint8_t led);
   sonoffChannel(pinmap_T pinmap);
   sonoffChannel(sonoff_ch_T);
   //sonoffChannel();
   //~sonoffChannel();

   // Gets
   uint8_t getRelayPin(){return _relay.getPin();}
   uint8_t getLedPin(){return _led.getPin();}
   uint8_t getButtonPin(){return _button.getPin();}
   boolean getRelayState(){return _relay.getState();}
   boolean getLedState(){return _led.getState();}


   // Sets
   void setChannelPinmap(pinmap_T pinmap);
   void setLedStyle(gpio_style_T style){_led.setStyle(style);}
   //void useMqtt(PubSubClient *client = NULL);
   const void setStateTopic(const char* topic, const char* plOn="ON", const char* plOff="OFF"){
     mqtt.setStateTopic(topic,plOn,plOff);
   }
   const void setCommandTopic(const char* topic, const char* plOn="ON", const char* plOff="OFF"){
     mqtt.setCommandTopic(topic,plOn,plOff);
   }
   const void setPublishInterval(unsigned long interval){
     mqtt.setPublishInterval(interval);
   }
   const void setWemoName(const char* name){wemo.setDeviceName(name);}
   static void setLedMode(led_mode_T mode){_led.setMode(mode);}
   static led_mode_T getLedMode(){return _led.getMode();}
   static void setLedPulseSpeed(float step){_led.setPulseSpeed(step);}
   static void setLedBlinkSpeed(unsigned long ms){_led.setBlinkSpeed(ms);}
   static void setLedBlinkDuration(unsigned long ms){_led.setBlinkDuration(ms);}
   static void setLedMaxBrightness(uint16_t power){_led.setMaxBrightness(power);}

   // Actions
   void switchOn() {_relay.switchOn();}
   void switchOff() {_relay.switchOff();}
   int8_t publish(const char* topic, const char* payload, uint8_t qos = 1){mqtt.publish(topic,payload,qos);}

   // Callback setting functions
   void setMQTTConnectCallback(callbackFunction f);
   void setMQTTFailedCallback(callbackFunction f);
   void setButtonPressCallback(callbackFunction f);
   void setButtonHoldCallback(callbackFunction f);
   void setMQTTErrorCallback(callbackFunction f);

   // WEMO functions


   // General functions
   void handle();
   void beginMqtt(){
     _usingMqtt = true;
     mqtt.connect();
   }
   void beginWemo();
   String getSettings();

 private:

   sonoffButton _button;
   sonoffRelay _relay;
   static sonoffLed _led;

   sonoffMqtt mqtt;
   Ticker _publishTicker;
   boolean _publishing = false;
   sonoffWemo wemo;

   boolean _usingWemo = false;
   boolean _usingMqtt = false;

   // Handle Functions
   void _MQTTHandle();
   void _handleErrorNotify(byte numBlinks);
   void _handleCallbacks();
   volatile boolean _needButtonPressCallback = false;
   volatile boolean _needButtonHoldCallback = false;

};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_SONOFF)
  extern SONOFF sonoff;
#endif

#endif
