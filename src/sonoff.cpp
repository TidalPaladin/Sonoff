#include "sonoff.h"

/*
===============================================================================
                              Static Vars
===============================================================================
*/

/*
  Relays - 12,5,4,15
  Buttons - 0,9,10,14
  LED - 13 ONLY, others are tied to relay state
*/
const pinmap_T SONOFF::_pinmapCh1 = {12,0,13};
const pinmap_T SONOFF::_pinmapCh2 = {5,9,13};
const pinmap_T SONOFF::_pinmapCh3 = {4,10,13};
const pinmap_T SONOFF::_pinmapCh4 = {15,14,13};
const pinmap_T SONOFF::_pinmapSV = {12,0,13};
const pinmap_T SONOFF::_pinmapNodemcu = {16,0,2};

std::vector<sonoffChannel*> SONOFF::activeChannels;
sonoffLed sonoffChannel::_led;
//ESP8266WebServer SONOFF::_statusServer(80);

// Wifimanager statics
WiFiManager SONOFF::_wifiManager;
WiFiEventHandler SONOFF::_wifiConnectHandler;
WiFiEventHandler SONOFF::_wifiDisconnectHandler;
boolean SONOFF::_usingWifiManager;
boolean SONOFF::_usingOTA;
Ticker SONOFF::_otaHandle;
const char* SONOFF::_hostname;
char SONOFF::_wifiManagerSSID[64];
char SONOFF::_wifiManagerPW[64];

const void SONOFF::vectorIterator(void function (sonoffChannel *channel)){
  for(std::vector<sonoffChannel*>::iterator itor = SONOFF::activeChannels.begin(); itor!=SONOFF::activeChannels.end(); ++itor){
    function((*itor));
  }
}

/*
===============================================================================
                              Constructors / Destructors
===============================================================================
*/

SONOFF::SONOFF(){
  WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event) {
    sonoffMqtt::disconnect();
    wifiManagerConnect();
  });


}

sonoffChannel::sonoffChannel() {
  setChannelPinmap(_pinmapCh1);
}


// Setup channel by sonoff_T
sonoffChannel::sonoffChannel(sonoff_ch_T type){
  pinmap_T pinmap;
  switch(type) {

    case SO_CH1:
      pinmap = _pinmapCh1;
      break;

    case SO_NODEMCU:
      pinmap = _pinmapNodemcu;
      break;
  }
  setChannelPinmap(pinmap);
}

// Setup channel by pinmap
sonoffChannel::sonoffChannel(pinmap_T pinmap){
  setChannelPinmap(pinmap);
}

// Setup channel by pins
sonoffChannel::sonoffChannel(uint8_t r, uint8_t b, uint8_t l){
  pinmap_T pinmap = {r,b,l};
  setChannelPinmap(pinmap);
}

void sonoffChannel::setChannelPinmap(pinmap_T pinmap) {
  _led.setPin(pinmap.led);
  _relay.setPin(pinmap.relay);
  _button.setPin(pinmap.button);
  SONOFF::activeChannels.push_back(this);
}

void SONOFF::addChannel(sonoffChannel *channel){
  activeChannels.push_back(channel);
}

/*
===============================================================================
                              Setup Functions
===============================================================================
*/


void SONOFF::setHostname(const char* hostname) {
  _hostname = hostname;
  VERBOSE_PRINTLN("Set SONOFF hostname to "+String(_hostname));
}

void SONOFF::attachWifiManager(const char* ssid, const char* pass, unsigned long seconds){
  if(_usingWifiManager){return;}  // Dont rerun this on accident

  _usingWifiManager = true;
  if(!seconds){_wifiManager.setConfigPortalTimeout(seconds);} // Set timeout

  // Set SSID/pass, SSID defaults to SONOFF_XXXX
  if(!ssid){
    String cid = String( "SONOFF_" + String(ESP.getChipId()));
    strcpy(_wifiManagerSSID,cid.c_str());

  }
  else{strcpy(_wifiManagerSSID,ssid);}
  if(pass){strcpy(_wifiManagerPW,pass);}

  // Turn all the LEDs on solid if we go into AP mode
  _wifiManager.setAPCallback([](WiFiManager *myWiFiManager) {
    Serial.println("Launching WiFiManager AP...");
    sonoffChannel::setLedMode(SOLID);
  });

  wifiManagerConnect();

  // TODO here we define the wifimanager config parameters for MQTT/WEMO

  //_wifiManager.addParameter(&_MQTTServerParam);

  VERBOSE_PRINTLN("Attached WiFiManager to SONOFF");
}

void SONOFF::setWifiManagerTimeout(unsigned long seconds){
  _wifiManager.setConfigPortalTimeout(seconds);
  VERBOSE_PRINTLN("Set wifi manager config timeout to "+String(seconds));
}

// // TODO does this need to be public? When would a user need to call this?
void SONOFF::wifiManagerConnect(){
  // Abort if not using WifiManager or were connected
  if(!_usingWifiManager || WiFi.status() == WL_CONNECTED){return;}

  if(_wifiManagerSSID[0] != '\0' && _wifiManagerPW[0] != '\0'){
    VERBOSE_PRINTLN("Starting WM autoConnect using SSID and password");
    _wifiManager.autoConnect(_wifiManagerSSID,_wifiManagerPW);
  }
  else if(_wifiManagerSSID[0] != '\0'){
    VERBOSE_PRINTLN("Starting WM autoConnect using SSID");
    VERBOSE_PRINTLN("SSID: "+String(_wifiManagerSSID));
    _wifiManager.autoConnect(_wifiManagerSSID);
  }
  else{
    VERBOSE_PRINTLN("Starting WM autoConnect using SSID and password");
    _wifiManager.autoConnect();
  } // Just as backup

  sonoffChannel::setLedMode(BLINK); // TODO handle this better if user sets a custom mode
}

// TODO this can probably be merged with checkWifiManager
void SONOFF::_handleWifiManager(){
  if(!_usingWifiManager){return;}
  wifiManagerConnect();
}

void sonoffChannel::beginWemo(){
  _usingWemo = true;
  wemo.begin();
}



void sonoffChannel::_handleCallbacks() {

}



void sonoffChannel::handle() {
  mqtt_actions_t mqttAction;
  wemo_actions_t wemoAction;
  so_actions_t action;

  action = _button.handle();
  if(action == SO_RELAY_TOGGLE){
    Serial.println("Toggling relay from button");
    _relay.toggleRelay();
    if(_usingMqtt){mqtt.publish(_relay.getState());}
  }


  if( _usingMqtt) {
    mqttAction = mqtt.handle(_relay.getState());

    // If disconnected, make sure led mode is PULSE
    if(mqttAction == MQTT_DISCONNECT && _led.getMode() != PULSE){
      Serial.println("*SONOFF\tSetting LED mode to pulse bc dc");
      _led.setMode(PULSE);
    }
    else if( mqttAction == MQTT_CONNECT && _led.getMode() != BLINK ){
      _led.setMode(BLINK);
    }
    else if( mqttAction == MQTT_PAYLOAD_ON && !_relay.getState() ){
      Serial.println("*SONOFF\tSwitching relay to on from MQTT");
      _relay.switchOn();
      mqtt.publish(_relay.getState());
    }
    else if( mqttAction == MQTT_PAYLOAD_OFF && _relay.getState()){
      VERBOSE_PRINTLN("*SONOFF\tSwitching relay to off from MQTT");
      _relay.switchOff();
      mqtt.publish(_relay.getState());
    }
  }

  if(_usingWemo){
    wemoAction = wemo.handle();
    if(wemoAction == WEMO_ON){
      _relay.switchOn();
    }
    else if(wemoAction == WEMO_OFF){
      _relay.switchOff();
    }

    if(wemoAction == WEMO_ON || wemoAction == WEMO_OFF && _usingMqtt){
      mqtt.publish(_relay.getState());
    }
  }
}

// Static function that handles all SONOFFs and shared static handles
void SONOFF::handle(){
  //if(_usingOTA){ArduinoOTA.handle();}
  _handleWifiManager();

  // // Handle all active channels
  // vectorIterator([](sonoffChannel *channel) {
  //   channel->handle();
  // });

}

// TODO this will have problems if no SONOFF objects exist
// Need to change onXXX behavior when last object is destructed
void SONOFF::sonoffOTA(const char* pass) {
  if(_usingOTA){return;}

  // Dont rerun this code
  ArduinoOTA.setHostname(_hostname);
  ArduinoOTA.setPassword(pass);

  // On start, turn all relays off for safety and detach button interrupts
  ArduinoOTA.onStart([]() {
    Serial.println("Started OTA update");
    sonoffChannel::setLedMode(SOLID);
  });

  // On end pulse LED for a while
  ArduinoOTA.onEnd([]() {
    Serial.println("Finished OTA update");
    sonoffChannel::setLedMode(SOLID);
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");

    // Reset LED to normal behavior
    // TODO store these values onStart() incase user has changed them
    sonoffChannel::setLedBlinkSpeed(3000);
    sonoffChannel::setLedBlinkDuration(200);
    sonoffChannel::setLedMode(BLINK);
  });

  // On progress, blink the led every 200 ms
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    if(sonoffChannel::getLedMode() != BLINK && progress){
      //_otaHandle.attach_ms(30,_otaWrap);  // Call ArduinoOTA.handle() every 30ms
      sonoffChannel::setLedBlinkSpeed(200);
      sonoffChannel::setLedBlinkDuration(50);
      sonoffChannel::setLedMode(BLINK);
    }
  });
  _usingOTA = true;
  ArduinoOTA.begin();
  VERBOSE_PRINTLN("Attached OTA updater to SONOFF class");
}

/*
===============================================================================
                          Set Callback Functions
===============================================================================
*/

// void SONOFF::setMQTTConnectCallback(callbackFunction f){
//   _MQTTConnectCallback  = f;
//   VERBOSE_PRINTLN("Set MQTT connect callback");
// }
// void SONOFF::setMQTTFailedCallback(callbackFunction f){
//   _MQTTFailedCallback = f;
//   VERBOSE_PRINTLN("Set MQTT failure callback");
// }
// void SONOFF::setButtonPressCallback(callbackFunction f){
//   _pinPressCallback = f;
//   VERBOSE_PRINTLN("Set custom button press callback");
// }
// void SONOFF::setButtonHoldCallback(callbackFunction f){
//   _pinHoldCallback = f;
//   VERBOSE_PRINTLN("Set custom button hold callback");
// }
// void SONOFF::setMQTTErrorCallback(callbackFunction f){
//   _MQTTErrorCallback = f;
//   VERBOSE_PRINTLN("Set MQTT error callback");
// }
//
/*
===============================================================================
                            Utility Functions
===============================================================================
*/

// String sonoffMqtt::getSettings(){
//   String result =
//     "* \tState topic - [" + String(_state.topic) + "] - "+ String(_state.payloadOn) + "/" + String(_state.payloadOff) + ""
//     "* \tCommand topic - [" + String(_command.topic)+ "] - " + String(_state.payloadOn) + "/" + String(_state.payloadOff) + ""
//     "* \tPublish interval (ms) - " + String(_publishInterval) + ""
//     "* \tReconnect interval (ms) - " + String(_reconnectInterval) + ""
//     "* \tCurrent MQTT state - (" + String(_client->state()) + ")\r\n"
//     ;
//   return result;
// }
//
// String sonoffLed::getSettings(){
//   String result =
//     "* LED GPIO: "+String(getPin()) + ""
//     "* LED Max Brightness - "+String(_ledMaxBrightness) + ""
//     "* LED Dim Brightness - "+String(_ledDimBrightness) + "\r\n"
//     ;
//   return result;
// }
//
// // Prints settings for the channel
// String sonoffChannel::getSettings() {
//   String output =
//     ""
//     "SONOFF Channel Information"
//     "* Relay GPIO: "+ String(_relay.getPin()) +""
//     "* Button GPIO: "+ String(_button.getPin()) +""
//     "* LED GPIO: "+ String(_led.getPin()) + "\r\n";
//
//   if(_usingMqtt){
//     output = output + "* Using MQTT\r\n";
//     output = output + mqtt.getSettings();
//   }
//   else{
//     output = output + "* MQTT not attached\r\n";
//   }
//
//   if(this->_usingWemo){
//     output = output + "* Using WEMO\r\n";
//     // wemo.printSettings();
//     // Serial.println(" \t* WEMO name - "+String(this->_deviceName));
//     // Serial.println(" \t* WEMO port - "+String(this->_portWemo));
//   }
//   else {
//     output = output + "* WEMO not attached\r\n";
//   }
//   // Serial.println("* Button press refractory period (ms) - "+String(_refractoryPeriod) );
//   // Serial.println("* Button hold duration (ms) - "+String(_pinHoldDuration) );
//
//   output = output +
//   "* There are currently "+String(SONOFF::activeChannels.size())+" active SONOFF channels"
//   "* Current free heap - " + String(ESP.getFreeHeap()) + ""
//   "* Current sketch size - " + String(ESP.getSketchSize()) + ""
//   "* CPU clock speed - " + String(ESP.getCpuFreqMHz())+" MHz"
//   ;
// }

// void SONOFF::printAll() {
//   vectorIterator([](sonoffChannel* channel){
//     //channel->printSettings();
//   });
// }

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_SONOFF)
  SONOFF sonoff;
#endif
