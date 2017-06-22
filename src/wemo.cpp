#include "wemo.h"

uint8_t sonoffWemo::_wemoCounter = 0;
IPAddress sonoffWemo::_ipMulti(239, 255, 255, 250);
WiFiUDP sonoffWemo::_UDP;
uint16_t sonoffWemo::_portMulti = 1900;
uint16_t sonoffWemo::_wemoPort = 40000;
boolean sonoffWemo::_udpConnected = false;
char sonoffWemo::_deviceUuid[64];

sonoffWemo::sonoffWemo(const char* deviceName){
  this->_index = _wemoCounter;
  _wemoCounter++;
  setDeviceName(deviceName);
  this->_wemoHTTP = ESP8266WebServer(_wemoPort);
  _prepareIds();
}

void sonoffWemo::begin(){
  _udpConnected = _connectUDP();
  if (_udpConnected){
    VERBOSE_PRINT("Starting Wemo HTTP server");
    _startHttpServer();
  }
}

sonoffWemo::~sonoffWemo(){
  _wemoCounter--;
}

void sonoffWemo::_prepareIds() {
  uint32_t chipId = ESP.getChipId();
  char uuid[64];  //64?
  sprintf_P(uuid, PSTR("Socket-1_0-38323636-4558-4dda-9188-cda0e6%02x%02x%02x%u"),
        (uint16_t) ((chipId >> 16) & 0xff),
        (uint16_t) ((chipId >>  8) & 0xff),
        (uint16_t)   chipId        & 0xff),
        _index;
  strcpy(_channelUuid,uuid);
  String newUuid = String(uuid);
  strcpy(_deviceUuid,newUuid.substring(0,newUuid.length()).c_str());
}


void sonoffWemo::_respondToSearch() {
    Serial.print("*WEMO\tSending UDP response to ");
    Serial.print(_UDP.remoteIP());
    Serial.print(":");
    Serial.println(_UDP.remotePort());

    // Get the SONOFF IP address
    IPAddress localIP = WiFi.localIP();
    char s[16];
    sprintf(s, "%d.%d.%d.%d", localIP[0], localIP[1], localIP[2], localIP[3]);

    // Build the response string
    Serial.println("*WEMO\tThere are "+String(_wemoCounter)+" active WEMOs to respond to");
    for(int i=0; i<_wemoCounter; i++) {

      String response =
           "HTTP/1.1 200 OK\r\n"
           "CACHE-CONTROL: max-age=86400\r\n"
           "DATE: Fri, 15 Apr 2016 04:56:29 GMT\r\n"  // TODO get correct date?
           "EXT:\r\n"
           "LOCATION: http://" + String(s) + ":"+ String(40000+i) +"/setup.xml\r\n"
           "OPT: \"http://schemas.upnp.org/upnp/1/0/\"; ns=01\r\n"
           "01-NLS: b9200ebb-736d-4b93-bf03-835149d13983\r\n"
           "SERVER: Unspecified, UPnP/1.0, Unspecified\r\n"
           "ST: urn:Belkin:device:**\r\n"
           "USN: uuid:" + String(_deviceUuid) + String(i) + "::urn:Belkin:device:**\r\n"
           "X-User-Agent: redsonic\r\n\r\n";

      // Send the response
      _UDP.beginPacket(_UDP.remoteIP(), _UDP.remotePort());
      _UDP.write(response.c_str());
      _UDP.endPacket();
      VERBOSE_PRINTLN("Response sent: ");
      //VERBOSE_PRINTLN(response);
    }
}


// Defines what HTTP server should send to clients
void sonoffWemo::_startHttpServer() {

    _wemoHTTP.on("/upnp/control/basicevent1", HTTP_POST, [this]() {
      Serial.println("*WEMO\tResponding to /upnp/control/basicevent1");

      // for (int x=0; x <= _wemoHTTP.args(); x++) {
      //  Serial.println(_wemoHTTP.arg(x));
      // }

      String request = _wemoHTTP.arg(0);
      VERBOSE_PRINT("Basicevent request: ");
      VERBOSE_PRINTLN(request);

      if(request.indexOf("<BinaryState>1</BinaryState>") > 0) {
          Serial.println("*WEMO\tGot WEMO turn on request - "+String(_deviceName) );
          _wemoAction = WEMO_ON;
      }

      if(request.indexOf("<BinaryState>0</BinaryState>") > 0) {
          Serial.println("*WEMO\tGot WEMO turn off request - "+String(_deviceName) );
          _wemoAction = WEMO_OFF;
      }
      _wemoHTTP.send(200, "text/plain", "");
    });

    _wemoHTTP.on("/eventservice.xml", HTTP_GET, [this](){
      Serial.println("*WEMO \tResponding to eventservice.xml");
      String eventservice_xml = "<?scpd xmlns=\"urn:Belkin:service-1-0\"?>"
            "<actionList>"
              "<action>"
                "<name>SetBinaryState</name>"
                "<argumentList>"
                  "<argument>"
                    "<retval/>"
                    "<name>BinaryState</name>"
                    "<relatedStateVariable>BinaryState</relatedStateVariable>"
                    "<direction>in</direction>"
                  "</argument>"
                "</argumentList>"
                 "<serviceStateTable>"
                  "<stateVariable sendEvents=\"yes\">"
                    "<name>BinaryState</name>"
                    "<dataType>Boolean</dataType>"
                    "<defaultValue>0</defaultValue>"
                  "</stateVariable>"
                  "<stateVariable sendEvents=\"yes\">"
                    "<name>level</name>"
                    "<dataType>string</dataType>"
                    "<defaultValue>0</defaultValue>"
                  "</stateVariable>"
                "</serviceStateTable>"
              "</action>"
            "</scpd>\r\n"
            "\r\n";

      _wemoHTTP.send(200, "text/plain", eventservice_xml.c_str());
    });

    _wemoHTTP.on("/setup.xml", HTTP_GET, [this](){
      Serial.println("*WEMO\tResponding to setup.xml");

      IPAddress localIP = WiFi.localIP();
      char s[16];
      sprintf(s, "%d.%d.%d.%d", localIP[0], localIP[1], localIP[2], localIP[3]);

      String setup_xml = "<?xml version=\"1.0\"?>"
          "<root>"
              "<device>"
                  "<deviceType>urn:Belkin:device:controllee:1</deviceType>"
                  "<friendlyName>"+ String(_deviceName) +"</friendlyName>"
                  "<manufacturer>Belkin International Inc.</manufacturer>"
                  "<modelName>Emulated Socket</modelName>"
                  "<modelNumber>3.1415</modelNumber>"
                  "<UDN>uuid:"+ _channelUuid +"</UDN>"
                  "<_serialNumber>221517K0101769</_serialNumber>"
                  "<binaryState>0</binaryState>"
                  "<serviceList>"
                    "<service>"
                        "<serviceType>urn:Belkin:service:basicevent:1</serviceType>"
                        "<serviceId>urn:Belkin:serviceId:basicevent1</serviceId>"
                        "<controlURL>/upnp/control/basicevent1</controlURL>"
                        "<eventSubURL>/upnp/event/basicevent1</eventSubURL>"
                        "<SCPDURL>/eventservice.xml</SCPDURL>"
                    "</service>"
                "</serviceList>"
              "</device>"
          "</root>\r\n\r\n";
        _wemoHTTP.send(200, "text/xml", setup_xml.c_str());

        VERBOSE_PRINTLN("Sending :");
        VERBOSE_PRINTLN(setup_xml);
    });

    _wemoHTTP.begin();
    Serial.println("*WEMO\tWemo HTTP Server started");
}

boolean sonoffWemo::_connectUDP(){
  boolean state = false;

  VERBOSE_PRINT("\r\nConnecting to UDP");

  if(_UDP.beginMulticast(WiFi.localIP(), _ipMulti, _portMulti)) {
    VERBOSE_PRINTLN("Connection to UDP successful");
    state = true;
  }
  else{
    Serial.println("Connection to UDP failed");
  }

  return state;
}

wemo_actions_t sonoffWemo::handle() {
  this->_wemoHTTP.handleClient(); // Handle HTTP clients
  wemo_actions_t ret = _wemoAction;

  // Take action in reponse to binarystate commands ifneeded
  if(ret != WEMO_OK){
    _wemoAction = WEMO_OK;  // Reset wemoaction
    return ret;
  }

  if(!_udpConnected){return WEMO_NO_UDP;}

  // if there’s data available, read a packet
  int packetSize = _UDP.parsePacket();
  if(!packetSize){return WEMO_OK;}

  VERBOSE_PRINT("\r\nReceived packet of size ");
  VERBOSE_PRINT(packetSize);
  VERBOSE_PRINT(" from ");
  IPAddress remote = _UDP.remoteIP();

  for (int i =0; i < 4; i++) {
    VERBOSE_PRINT(String(remote[i], DEC));
    if (i < 3) {
      VERBOSE_PRINT(".");
    }
  }

  VERBOSE_PRINT(", port ");
  VERBOSE_PRINTLN(_UDP.remotePort());

  int len = _UDP.read(_packetBuffer, 255);

  if (len > 0) {
      _packetBuffer[len] = 0;
  }

  String request = _packetBuffer;
  VERBOSE_PRINTLN("Request:");
  VERBOSE_PRINT(request);
  VERBOSE_PRINTLN("");

  // Handle search response if needed
  if(request.indexOf("M-SEARCH") > -1) {
    if(request.indexOf("urn:Belkin:device:**") > -1) {
        Serial.println("*WEMO\tResponding to UDP search request...");
        _respondToSearch();
    }
  }
  VERBOSE_PRINTLN("\r\n");
}
