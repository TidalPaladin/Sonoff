#ifndef SOMQTT_H
#define SOMQTT_H

#if (ARDUINO >= 100)
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>
#include "relay.h"

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
  MQTT_OK,
  MQTT_PAYLOAD_ON,
  MQTT_PAYLOAD_OFF,
  MQTT_UNK_PAYLOAD,
  MQTT_DISCONNECT,
  MQTT_CONNECT,
  MQTT_PUBLISH
} mqtt_actions_t;

#define MQTT_LEN  80

typedef struct {
  char topic[MQTT_LEN];
  char payloadOn[MQTT_LEN];
  char payloadOff[MQTT_LEN];
} topic_t;

typedef struct {
  char topic[MQTT_LEN];
  char payload[MQTT_LEN];
} payload_t;



class sonoffMqtt {
  public:

    // Constructor
    sonoffMqtt();
    void begin();

    // Gets
    static boolean connected(){return _mqttClient.connected();}

    // Sets
    static void setServer(const char* host, uint16_t port = 1883);
    static void setServer(IPAddress addr, uint16_t port = 1883);
    const void setStateTopic(const char* topic, const char* plOn="ON", const char* plOff="OFF");
    const void setCommandTopic(const char* topic, const char* plOn="ON", const char* plOff="OFF");
    const void setPublishInterval(unsigned long interval);
    const void setPayloadCallback(void (func)(char* topic, byte* payload, unsigned int length));

    // Actions
    static void connect();
    static void disconnect();

    static void onConnect(bool sessionPresent);
    static void onDisconnect(AsyncMqttClientDisconnectReason reason);
    static void onSubscribe(uint16_t packetId, uint8_t qos);
    static void onUnsubscribe(uint16_t packetId);
    static void onMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
    static void onPublish(uint16_t packetId);

    static void payloadCallback(char* topic, byte* payload, unsigned int length);
    int8_t publish(const char* topic, const char* payload, uint8_t qos = 1);
    int8_t publish(boolean isOn, uint8_t qos = 1);
    boolean matchTopic(const char* topic); // TODO make private?
    void publishState();

    mqtt_actions_t handle(boolean state);

    String getSettings();

  private:
    static uint8_t _mqttCounter;

    boolean _relayIsOn;

    static AsyncMqttClient _mqttClient;

    static Ticker _reconnectTimer;
    Ticker _publishTicker;
    static void publishWrap(void* ptr);

    static boolean _needsSubscribe;

    boolean _publishState();
    //boolean *_relay = NULL;

    topic_t _command;
    topic_t _state;
    static payload_t _lastPayload;

    unsigned long _publishInterval = 30000;  // Time in ms
    static unsigned long _reconnectInterval;
    unsigned long _reconnTimer = 0; // Tracks time during reconnects
    void _reconnect();

};

#endif
