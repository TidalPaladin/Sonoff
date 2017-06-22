#ifndef SOWEMO_H
#define SOWEMO_H

#if (ARDUINO >= 100)
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>

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
  WEMO_OK,
  WEMO_ON,
  WEMO_OFF,
  WEMO_NO_UDP
} wemo_actions_t;

class sonoffWemo {

  public:
    sonoffWemo(const char* deviceName = String( "SONOFF["+String(ESP.getChipId()).substring(1,4)+String(_wemoCounter)+"]").c_str() );
    ~sonoffWemo();

    // Gets
    const char* getDeviceName(){return _deviceName;}
    uint16_t getPort(){return _wemoPort;}

    // Sets
    void setDeviceName(const char* name){strcpy(_deviceName,name);}

    wemo_actions_t handle();
    void begin();

  private:
    void _prepareIds();
    void _startHttpServer();

    static IPAddress _ipMulti;
    static WiFiUDP _UDP; //WEMO
    static boolean _udpConnected;
    static uint16_t _portMulti;      // local port to listen on
    static uint16_t _wemoPort;
    static boolean _connectUDP();
    static void _respondToSearch();
    static uint8_t _wemoCounter;
    static boolean _wifiConnected;
    static boolean _cannotConnectToWifi;
    char _packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,
    //static std::vector<uint16_t> _wemoPorts;

    ESP8266WebServer _wemoHTTP;
    uint8_t _index;
    wemo_actions_t _wemoAction;

    char _channelUuid[64];
    static char _deviceUuid[64];
    char _deviceName[50];

};

#endif
