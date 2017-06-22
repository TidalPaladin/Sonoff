#include "mqtt.h"

AsyncMqttClient sonoffMqtt::_mqttClient;

unsigned long sonoffMqtt::_reconnectInterval = 30000;
payload_t sonoffMqtt::_lastPayload;

Ticker sonoffMqtt::_reconnectTimer;

boolean sonoffMqtt::_needsSubscribe = true;

uint8_t sonoffMqtt::_mqttCounter = 0;

sonoffMqtt::sonoffMqtt(){
  _mqttCounter++;
  setCommandTopic("command","ON","OFF");
  setStateTopic("state","ON","OFF");
  _publishTicker.attach_ms(_publishInterval, publishWrap, (void*)this);
  // TODO why cant I set onXXX functions in constructor? Theyre ignored
}

void sonoffMqtt::setServer(const char* host, uint16_t port) {
  VERBOSE_PRINTLN("Setting MQTT server");
  _mqttClient.setServer(host,port);
  _mqttClient.onConnect(onConnect);
  _mqttClient.onDisconnect(onDisconnect);
  _mqttClient.onSubscribe(onSubscribe);
  _mqttClient.onUnsubscribe(onUnsubscribe);
  _mqttClient.onMessage(onMessage);
  _mqttClient.onPublish(onPublish);
}

void sonoffMqtt::setServer(IPAddress addr, uint16_t port){
  VERBOSE_PRINTLN("Setting MQTT server");
  _mqttClient.setServer(addr,port);
  _mqttClient.onConnect(onConnect);
  _mqttClient.onDisconnect(onDisconnect);
  _mqttClient.onSubscribe(onSubscribe);
  _mqttClient.onUnsubscribe(onUnsubscribe);
  _mqttClient.onMessage(onMessage);
  _mqttClient.onPublish(onPublish);

}

void sonoffMqtt::connect() {
  Serial.println("*MQTT\tConnecting...");
  _mqttClient.connect();
}

void sonoffMqtt::disconnect(){
  _reconnectTimer.detach();
  _mqttClient.disconnect();
}

void sonoffMqtt::onConnect(bool sessionPresent){
  Serial.print("*MQTT\tConnected, ");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
  _needsSubscribe = true; // Tell this->handle() that we need to subscribe
}

void sonoffMqtt::onDisconnect(AsyncMqttClientDisconnectReason reason){
  Serial.println("*MQTT\tDisconnected");

  if (WiFi.isConnected()) {
    Serial.println("*MQTT\tConnecting...");
    _reconnectTimer.once(2, connect);
  }
}

void sonoffMqtt::onMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total){
  String t = topic;
  String p = String(payload).substring(index,len);
  Serial.println("*MQTT\tMessage - ["+ t + "] : "+p);

  // Store topic/payload for comparison in this->handle()
  strcpy(_lastPayload.topic,t.c_str());
  strcpy(_lastPayload.payload,p.c_str());
}

void sonoffMqtt::onSubscribe(uint16_t packetId, uint8_t qos){
  Serial.println("*MQTT\tSubscribe acknowledged.");
}
void sonoffMqtt::onUnsubscribe(uint16_t packetId){
  Serial.println("*MQTT\tUnsubscribe acknowledged.");
}

void sonoffMqtt::onPublish(uint16_t packetId){
  Serial.println("*MQTT\tPublish acknowledged.");
}

// Publish to a given topic and payload (public function)
int8_t sonoffMqtt::publish(const char* topic, const char* payload, uint8_t qos) {

  // Abort if not connected or trying to publish to command topic
  if( !strcmp(this->_command.topic, topic) || !this->connected() ){
    return -3;
  }
  Serial.print("*MQTT\tPublishing - ["+String(this->_state.topic)+"] : "+String(payload));
  _mqttClient.publish(this->_state.topic, qos, true, payload);
}

int8_t sonoffMqtt::publish(boolean isOn, uint8_t qos) {

  // Abort if not connected or trying to publish to command topic
  if( !strcmp(this->_command.topic, this->_state.topic) || !this->connected() ){
    return -3;
  }
  Serial.print("*MQTT\tPublishing - ["+String(this->_state.topic)+"] : ");

  // Figure out state and publish appropriate payload
  if(isOn){
    Serial.println(String(this->_state.payloadOn));
    _mqttClient.publish(this->_state.topic, qos, true, _command.payloadOn);
  }
  else{
    Serial.println(String(this->_state.payloadOff));
    _mqttClient.publish(this->_state.topic, qos, true, _command.payloadOff);
  }
}

// Wrapper for Ticker
void sonoffMqtt::publishWrap(void * ptr){
  sonoffMqtt* self = (sonoffMqtt*) ptr;
  self->publish(self->_relayIsOn);
}

const void sonoffMqtt::setCommandTopic(const char* topic, const char* plOn, const char* plOff){
  //_client->unsubscribe(this->_command.topic);
  strcpy(_command.topic,topic);
  strcpy(_command.payloadOn,plOn);
  strcpy(_command.payloadOff,plOff);
  VERBOSE_PRINTLN("Set SONOFF CMD topic ["+String(topic)+"] - "+String(plOn)+"/"+String(plOff) );
  //_client->subscribe(this->_command.topic);
}

const void sonoffMqtt::setStateTopic(const char* topic, const char* plOn, const char* plOff){
  strcpy(_state.topic,topic);
  strcpy(_state.payloadOn,plOn);
  strcpy(_state.payloadOff,plOff);
  VERBOSE_PRINTLN("Set SONOFF STATE topic ["+String(topic)+"] - "+String(plOn)+"/"+String(plOff) );
}

const void sonoffMqtt::setPublishInterval(unsigned long interval){
  _publishInterval = interval;
  _publishTicker.attach_ms(_publishInterval, publishWrap, (void*)this);
  VERBOSE_PRINTLN("Set SONOFF MQTT publish interval - "+_publishInterval);
}

// Pass a MQTT topic to see if it matches the state/command topic for this instance
boolean sonoffMqtt::matchTopic(const char* topic){
  if( !strcmp(topic,this->_command.topic) ){
    return true;
  }
  return false;
}


mqtt_actions_t sonoffMqtt::handle(boolean state) {
  mqtt_actions_t ret;
  _relayIsOn = state; // Take state passed from sonoffChannel and store it

  if(
    !_lastPayload.topic[0] == 0 // If we have an incoming topic
    && !_lastPayload.payload[0] == 0  // and an incoming payload
    && matchTopic(_lastPayload.topic) // And the topic matches this channel's
  ){

    if( !strcmp(_lastPayload.payload,this->_command.payloadOn) ) {
      ret = MQTT_PAYLOAD_ON;
    }
    else if( !strcmp(_lastPayload.payload,this->_command.payloadOff) ) {
      ret = MQTT_PAYLOAD_OFF;
    }
    else{ ret = MQTT_UNK_PAYLOAD; }

    // Clear _lastPayload
    memset(_lastPayload.topic, 0, sizeof _lastPayload.topic);
    memset(_lastPayload.payload, 0, sizeof _lastPayload.payload);
    return ret;
  }
  static uint8_t iter = 0;

  // If we need to subscribe, iterate over all instances
  if(_needsSubscribe && _mqttClient.connected() ){
    if( iter < _mqttCounter){
      uint16_t packetIdSub = _mqttClient.subscribe(this->_command.topic, 2);
      Serial.println("*MQTT\tSubscribing to command at QoS 2");
      publish(_relayIsOn);  // If we need a resub, we probably need to publish
      iter++;
      return MQTT_CONNECT;
    }
    else{
      iter = 0;
      _needsSubscribe = false;
    }
  }
  else if( !_mqttClient.connected() ){
    return MQTT_DISCONNECT;
  }
  return MQTT_OK;
}
