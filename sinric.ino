
//Sinric Pro Google Home Alexa

#ifdef ENABLE_DEBUG
       #define DEBUG_ESP_PORT Serial
       #define NODEBUG_WEBSOCKETS
       #define NDEBUG
#endif 

#include <Arduino.h>
#include <WiFi.h>
#include "SinricPro.h"
#include "SinricProSwitch.h"

#include <map>

#define WIFI_SSID         "paiz"    
#define WIFI_PASS         "10112542"
#define APP_KEY           "0b0f6b03-51d1-4a14-a539-6af87f67de26"     
#define APP_SECRET        "f5e56127-1a23-454e-97d9-3f8a317488a1-64b81f08-424f-451d-b2e8-1c7f7077fc61"   


#define device_ID_1   "6179633deb3dca182822779f"
#define device_ID_2   "61796353eb3dca18282277a1"


#define RelayPin1 23  //D23
#define RelayPin2 22  //D22

#define SwitchPin1 13  //D13
#define SwitchPin2 12  //D12

#define wifiLed   2   //D2


#define TACTILE_BUTTON 1

#define BAUD_RATE   9600

#define DEBOUNCE_TIME 250

typedef struct {     
  int relayPIN;
  int flipSwitchPIN;
} deviceConfig_t;

std::map<String, deviceConfig_t> devices = {
 
    {device_ID_1, {  RelayPin1, SwitchPin1 }},
    {device_ID_2, {  RelayPin2, SwitchPin2 }},
};

typedef struct {      
  String deviceId;
  bool lastFlipSwitchState;
  unsigned long lastFlipSwitchChange;
} flipSwitchConfig_t;

std::map<int, flipSwitchConfig_t> flipSwitches;   

void setupRelays() { 
  for (auto &device : devices) {           
    int relayPIN = device.second.relayPIN; 
    pinMode(relayPIN, OUTPUT);             T
    digitalWrite(relayPIN, HIGH);
  }
}

void setupFlipSwitches() {
  for (auto &device : devices)  {                     
    flipSwitchConfig_t flipSwitchConfig;             

    flipSwitchConfig.deviceId = device.first;         
    flipSwitchConfig.lastFlipSwitchChange = 0;        
    flipSwitchConfig.lastFlipSwitchState = true;     

    int flipSwitchPIN = device.second.flipSwitchPIN;  

    flipSwitches[flipSwitchPIN] = flipSwitchConfig;  
    pinMode(flipSwitchPIN, INPUT_PULLUP);                 
  }
}

bool onPowerState(String deviceId, bool &state)
{
  Serial.printf("%s: %s\r\n", deviceId.c_str(), state ? "on" : "off");
  int relayPIN = devices[deviceId].relayPIN; 
  digitalWrite(relayPIN, !state);            
  return true;
}

void handleFlipSwitches() {
  unsigned long actualMillis = millis();                                          
  for (auto &flipSwitch : flipSwitches) {                                        
    unsigned long lastFlipSwitchChange = flipSwitch.second.lastFlipSwitchChange;  

    if (actualMillis - lastFlipSwitchChange > DEBOUNCE_TIME) {                   

      int flipSwitchPIN = flipSwitch.first;                                       
      bool lastFlipSwitchState = flipSwitch.second.lastFlipSwitchState;          
      bool flipSwitchState = digitalRead(flipSwitchPIN);                         
      if (flipSwitchState != lastFlipSwitchState) {                               
#ifdef TACTILE_BUTTON
        if (flipSwitchState) {                                                    
#endif      
          flipSwitch.second.lastFlipSwitchChange = actualMillis;                  
          String deviceId = flipSwitch.second.deviceId;                          
          int relayPIN = devices[deviceId].relayPIN;                              
          bool newRelayState = !digitalRead(relayPIN);                            
          digitalWrite(relayPIN, newRelayState);                                 

          SinricProSwitch &mySwitch = SinricPro[deviceId];                        
          mySwitch.sendPowerStateEvent(!newRelayState);                          
#ifdef TACTILE_BUTTON
        }
#endif      
        flipSwitch.second.lastFlipSwitchState = flipSwitchState;                
      }
    }
  }
}

void setupWiFi()
{
  Serial.printf("\r\n[Wifi]: Connecting");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.printf(".");
    delay(250);
  }
  digitalWrite(wifiLed, HIGH);
  Serial.printf("connected!\r\n[WiFi]: IP-Address is %s\r\n", WiFi.localIP().toString().c_str());
}

void setupSinricPro()
{
  for (auto &device : devices)
  {
    const char *deviceId = device.first.c_str();
    SinricProSwitch &mySwitch = SinricPro[deviceId];
    mySwitch.onPowerState(onPowerState);
  }

  SinricPro.begin(APP_KEY, APP_SECRET);
  SinricPro.restoreDeviceStates(true);
}

void setup()
{
  Serial.begin(BAUD_RATE);

  pinMode(wifiLed, OUTPUT);
  digitalWrite(wifiLed, LOW);

  setupRelays();
  setupFlipSwitches();
  setupWiFi();
  setupSinricPro();
}

void loop()
{
  SinricPro.handle();
  handleFlipSwitches();
}
