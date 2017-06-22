#include "sonoff.h"

/*
  NOTE SONOFF LEDs are HIGH for off and LOW for on
*/

IPAddress mqtt_server(192,168,1,200); // Set the IP address of the MQTT server
#define HOSTNAME      "SONOFF"        // Set your hostname here
#define OTA_PASSWORD  "PASSWORD"      // Password to perform OTA updates

#define MQTT_CMD    "path/to/cmd"
#define MQTT_STATE  "path/to/state"

sonoffChannel ch1;  // Create an instance of a channel with default pinmapping

void setupSONOFFS(){
  /*
    Setup properties for the entire sonoff
  */
  sonoff.setHostname(HOSTNAME);       // Defaults to SONOFF_XXXX
  sonoff.attachWifiManager();         // Optional built in wifi handling, you can use your own
  sonoff.setMqttServer(mqtt_server);  // Tell all sonoff channels to use a given MQTT server
  sonoff.sonoffOTA(OTA_PASSWORD);     // Set up OTA, including default LED behavior

  /*
    Setup properties for an individual channel
  */
  ch1.setCommandTopic(MQTT_CMD);  // Set the command topic for this channel
  ch1.setStateTopic(MQTT_STATE);  // Set the state topic for this channel
  ch1.beginMqtt();                // Start using MQTT
  ch1.setWemoName("Desk Lamp");   // Set the WEMO device name
  ch1.beginWemo();                // Start using Wemo
}

void setup() {
  Serial.begin(115200);
  setupSONOFFS();
}

void loop() {
  sonoff.handle();
  ch1.handle();
  ArduinoOTA.handle();  // This shouldn't necessary, just as a precaution
}
