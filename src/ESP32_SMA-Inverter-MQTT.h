#pragma once 
#ifndef ESP32_SMA_INVERTER_MQTT_H
#define ESP32_SMA_INVERTER_MQTT_H
/* MIT License

Copyright (c) 2022 Lupo135
Copyright (c) 2023 darrylb123

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <Esp.h>
#include "Arduino.h"
//#define RX_QUEUE_SIZE 2048
//#define TX_QUEUE_SIZE 64

#include "Config.h"
#include "SMA_Utils.h"
#include "SMA_Inverter.h"
#include "ESP32Loggable.h"

#include <WiFiClient.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <logging.hpp>
#include <ets-appender.hpp>
#include <udp-appender.hpp>

#include "ESP32_SMA_MQTT.h"
#include "Config_example.h"


#define DEBUG_SMA 1

struct AppConfig {
    String mqttBroker;
    uint16_t mqttPort;
    String mqttUser;
    String mqttPasswd;
    String mqttTopic;
    String smaInvPass;
    String smaBTAddress;
    int scanRate;
    bool hassDisc;
    String ntphostname;
    String timezone;
};


class ESP32_SMA_Inverter_MQTT : public ESP32Loggable {

  public:

          // Static method to get the instance of the class.
          static ESP32_SMA_Inverter_MQTT& getInstance() {
              // This guarantees that the instance is created only once.
              static ESP32_SMA_Inverter_MQTT instance;
              return instance;
          }

          // Delete the copy constructor and the assignment operator to prevent cloning.
          ESP32_SMA_Inverter_MQTT(const ESP32_SMA_Inverter_MQTT&) = delete;
          ESP32_SMA_Inverter_MQTT& operator=(const ESP32_SMA_Inverter_MQTT&) = delete;

    void appSetup();
    void appLoop();
    //void wifiStartup();
    void logBuild();

    static int smartConfig;

    static WebServer webServer;
    static WiFiClient espClient;
    static PubSubClient client;

    AppConfig appConfig;

    //Prototypes
     void loadConfiguration();
     void saveConfiguration();
     void printFile();
     void configSetup();
     void rmfiles();

  protected:
      //extern BluetoothSerial serialBT;
        bool nightTime = false;
        bool firstTime = true;
        bool dayNight = false;

   private: 
        ESP32_SMA_Inverter_MQTT() :  ESP32Loggable("ESP32_SMA_Inverter_MQTT") {
            logger().setLevel(esp32m::Debug);

            appConfig = AppConfig();
            strcpy(smaInvPass, "0000");
            /*for (int i=0;i<6;i++) {
                smaBTAddress[i]='0';
            }*/
        };

        ~ESP32_SMA_Inverter_MQTT() {}

        char smaInvPass[12];  
        uint8_t smaBTAddress[6]; // SMA bluetooth address
        //uint8_t  espBTAddress[6]; // is retrieved from BT packet

        uint32_t nextTime = 0;
        const String confFile = "/config.txt"; //extern const char *confFile = "/config.txt";  



};


/*
// Configuration structure
Config config;
//const uint16_t AppSUSyID = 125;
//uint32_t AppSerial;
char SmaInvPass[12]; 

// SMA blutooth address
uint8_t SmaBTAddress[6]; 

//
#define maxpcktBufsize 512
uint8_t  BTrdBuf[256];    //  Serial.printf("Connecting to %s\n", ssid);
uint8_t  pcktBuf[maxpcktBufsize];
uint16_t pcktBufPos = 0;
uint16_t pcktBufMax = 0; // max. used size of PcktBuf
uint16_t pcktID = 1;
const char BTPin[] = {'0','0','0','0',0}; // BT pin Always 0000. (not login passcode!)

// const int scanRate = 60;
uint8_t  EspBTAddress[6]; // is retrieved from BT packet
uint32_t nextTime = 0;
// uint32_t nextInterval = scanRate *1000; // 10 sec.
uint8_t  errCnt = 0;
bool     btConnected = false;

#define CHAR_BUF_MAX 2048
char timeBuf[24];
char charBuf[CHAR_BUF_MAX];
int  charLen = 0;
bool firstTime = true;
bool nightTime = false;
bool dayNight = false;


ESP32_SMA_Inverter& smaInverter = ESP32_SMA_Inverter::getInstance();

WiFiClient espClient;
PubSubClient client(espClient);

// External variables
extern WebServer webServer;
extern int smartConfig;

*/


#endif