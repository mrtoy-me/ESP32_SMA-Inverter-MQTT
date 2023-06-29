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
#include "BluetoothSerial.h"
//#define RX_QUEUE_SIZE 2048
//#define TX_QUEUE_SIZE 64

#include "Config.h"
#include "Utils.h"
#include "SMA_bluetooth.h"
#include "SMA_Inverter.h"

// Configuration structure
Config config;
const uint16_t AppSUSyID = 125;
uint32_t AppSerial;
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




// External variables
extern WebServer webServer;
extern int smartConfig;

void setup() { 
  extern BluetoothSerial SerialBT;
  extern InverterData *pInvData;
  Serial.begin(115200); 
  delay(1000);
  configSetup();
  wifiStartup();
  
  if ( !smartConfig) {
    // Convert the MAC address string to binary
    sscanf(config.SmaBTAddress.c_str(), "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx", 
            &SmaBTAddress[0], &SmaBTAddress[1], &SmaBTAddress[2], &SmaBTAddress[3], &SmaBTAddress[4], &SmaBTAddress[5]);
    // Zero the array, all unused butes must be 0
    for(int i = 0; i < sizeof(SmaInvPass);i++)
       SmaInvPass[i] ='\0';
    strlcpy(SmaInvPass , config.SmaInvPass.c_str(), sizeof(SmaInvPass));

    pInvData->SUSyID = 0x7d;
    pInvData->Serial = 0;
    nextTime = millis();
    // reverse inverter BT address
    for(uint8_t i=0; i<6; i++) pInvData->BTAddress[i] = SmaBTAddress[5-i];
    DEBUG2_PRINTF("pInvData->BTAddress: %02X:%02X:%02X:%02X:%02X:%02X\n",
                pInvData->BTAddress[5], pInvData->BTAddress[4], pInvData->BTAddress[3],
                pInvData->BTAddress[2], pInvData->BTAddress[1], pInvData->BTAddress[0]);
    // *** Start BT
    SerialBT.begin("ESP32test", true);   // "true" creates this device as a BT Master.
    SerialBT.setPin(&BTPin[0]); 
  }
  // *** Start WIFI and WebServer



} 

  // **** Loop ************
void loop() { 
  extern BluetoothSerial SerialBT;

  // connect or reconnect after connection lost 

  if ( !smartConfig && (nextTime < millis()) && (!btConnected)) {
    nextTime = millis() + (config.ScanRate * 1000);
    /* Serial.println("");
    Serial.print(millis());
    Serial.print(" + ");
    Serial.print(nextInterval);
    Serial.print(" = ");
    Serial.println(nextTime); */
    pcktID = 1;
    // **** Connect SMA **********
    DEBUG1_PRINT("\nConnecting SMA inverter: ");
    if (SerialBT.connect(SmaBTAddress)) {
      btConnected = true;
      // nextInterval = 60*1000; // 60 sec.
      // **** Initialize SMA *******
      DEBUG1_PRINTLN("connected");
      E_RC rc = initialiseSMAConnection();
     
      getBT_SignalStrength();
      
      // **** logon SMA ************
      DEBUG1_PRINT("\n*** logonSMAInverter");
      rc = logonSMAInverter(SmaInvPass, USERGROUP);
      ReadCurrentData();
      SerialBT.disconnect();
      btConnected = false;
    } // else {  // failed to connect
      // if (nextInterval<10*60*1000) nextInterval += 1*60*1000;
    // } 
  }
  // DEBUG1_PRINT(".");
  wifiLoop();
    
  delay(100);
}
