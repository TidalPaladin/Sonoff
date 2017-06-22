# Sonoff Library #

## Introduction ##

This is a library designed to help you get up and running with your Sonoff as quickly as possible. The objectives of this library are to

1. Handle as much as possible behind the scenes so that you can quickly write clean code that adapts your Sonoff to general use cases.

2. Provide substantial customization so that you can adapt your Sonoff to advanced or varied use cases.

The `sonoff` library consists of two parts. The first is the `sonoff` object, which stores attributes and handles actions that are needed
for every `sonoffChannel`. An example would be the target MQTT server, as all `sonoffChannels` will connect to the same server. Another example is the built in OTA behavior, which is specific to the physical Sonoff and not any particular `sonoffChannel`.

The second part is the `sonoffChannel` object, which stores attributes and handles actions unique to a single relay channel. One example would be the `switchOff()` function, which switches only a single relay off.

** This library is still a work in progress **

## Documentation ##

### Setting up a Channel ###

##### Setting a pinmap #####

All sonoffChannel instances require information about which GPIO to use for the relay and button. By default new instances of sonoffChannel will use the pin map for a single channel Sonoff. You can use different constructors or `setChannelPinmap` to specify the pinmap for a channel.

The default constructor for a single channel Sonoff
```c++
  sonoffChannel ch1;
```

For multi-channel Sonoffs or dev boards, you can use a builtin preset
```c++
  sonoffChannel ch2(SO_CH2);
```
Available presets are
* `SO_CH1`

* `SO_CH2`

* `SO_CH3`

* `SO_CH4`

* `SO_NODEMCU`

* `SO_SV`

**Note:** I do not own a multi-channel Sonoff so those pinmaps are untested

Finally, you can set a custom pinmap by giving the GPIO pins
```c++
uint8_t relay = 1;
uint8_t button = 2;
uint8_t led = 3;
sonoffChannel custom_channel(relay, button, led);
```

**Note:** For all Sonoffs that I have examined, there is only one controllable LED. The other LEDs are tied directly to the state of the relay. Consequently, this library treats the LED as a `static` element of the `sonoffChannel` class.

### Available Sonoff functions ###

These are the functions available to `sonoff`

* `void setMqttServer(const char* host, uint16_t port=1883)`

* `void setMqttServer(IPAddress addr, uint16_t port = 1883)`

* `void setHostname(const char* hostname)`

* `void sonoffOTA(const char* pass)` - Built in OTA handling, password required

* `void attachWifiManager(const char* ssid=NULL, const char* pass=NULL, unsigned long seconds=0)`

* `void setWifiManagerTimeout(unsigned long seconds)`

##### Built in ArduinoOTA Handling #####

This library can handle `ArduinoOTA`, and includes some nice preset behavior including LED notifications at different steps of the update process. Tell `sonoff` to handle ArduinoOTA using

```c++
  sonoff.sonoffOTA(OTA_PASSWORD);  // Set up OTA with some defaults
```

Status LED behavior is as follows:
* On start - LED is solid ON
* During update - LED blinks
* On finish - LED pulses before rebooting

If you don't want to use this builtin feature you are free to handle OTA updates outside of `sonoff`


### Available Channel functions ###
These are the functions available for a `sonoffChannel`

Get functions
* `uint8_t getRelayPin()`

* `uint8_t getLedPin()`

* `uint8_t getButtonPin()`

* `boolean getRelayState()`

* `boolean getLedState()`

Set functions
* `void setChannelPinmap(pinmap_T pinmap)`

* `void setLedStyle(gpio_style_T style)` - REG if GPIO HIGH switches the LED on

* `const void setStateTopic(const char* topic, const char* plOn="ON", const char* plOff="OFF")`

* `const void setCommandTopic(const char* topic, const char* plOn="ON", const char* plOff="OFF")`
* `const void setPublishInterval(unsigned long interval)`

* `const void setWemoName(const char* name)`

* `static void setLedMode(led_mode_T mode)` - BLINK, PULSE, SOLID, OFF

* `static led_mode_T getLedMode()`

* `static void setLedPulseSpeed(float step)` - How fast the LED pulses, predefined values are SO_LED_SLOW, SO_LED_MEDIUM, SO_LED_FAST

* `static void setLedBlinkSpeed(unsigned long ms)` - How often the LED blinks

* `static void setLedBlinkDuration(unsigned long ms)` - How long the LED is on for when it blinks

* `static void setLedMaxBrightness(uint16_t power)` - You may need to set this really low to notice a reduction

Actions
* `void switchOn()` - Switch the relay on

* `void switchOff()` - Switch the relay off

* `int8_t publish(const char* topic, const char* payload, uint8_t qos = 1)`

* `void beginMqtt()`

* `void beginWemo()`

Note that it is possible to control one Sonoff channel using both MQTT and WEMO
simultaneously.


## To do / wishlist ##
This library is still in a beta stage. Other commitments have left me with little time to complete and maintain the library, but it proved effective enough for my use cases that I deemed it worth releasing.

Todo
* Add user definable callbacks for different actions, eg MQTT connect.

* When WiFiManager cannot connect, use of buttons for manual control likely doesn't work

* Some button/led functions need to be made accessible from the sonoffChannel class eg `setButtonHoldDuration()`

* **OTA updating** See below

#### Breaking loops for OTA ####
A major contribution for this and many other projects involves the use of asynchronous or ISR code to allow OTA updates. It is easy to make a careless coding mistake that leads to an infinite loop or similar blocking function that prevents `ArduinoOTA.handle()` from running. This requires that you remove the Sonoff from its working environment and upload new code manually. If we had a way to force OTA updating to run in such situations, it could save a lot of time. Unfortunately this is not as simple as calling `handle()` in an ISR.

Perhaps an asynchronous web server could call `handle()` on client request? A web based method would be superior to a button based method, as sometimes Sonoffs are mounted in locations that are difficult to physically access. This would of course be useless if a WiFi connection cannot be made unless AP mode is used.
