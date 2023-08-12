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

#include "ESP32_SMA-Inverter-MQTT.h"



// SMA blutooth address
//uint8_t SmaBTAddress[6]; 

//
//#define maxpcktBufsize 512
//uint8_t  BTrdBuf[256];    //  Serial.printf("Connecting to %s\n", ssid);
//uint8_t  pcktBuf[maxpcktBufsize];
//uint16_t pcktBufPos = 0;
//uint16_t pcktBufMax = 0; // max. used size of PcktBuf
//uint16_t pcktID = 1;
//const char BTPin[] = {'0','0','0','0',0}; // BT pin Always 0000. (not login passcode!)

// const int scanRate = 60;
//uint8_t  EspBTAddress[6]; // is retrieved from BT packet
//uint32_t nextTime = 0;
// uint32_t nextInterval = scanRate *1000; // 10 sec.
//uint8_t  errCnt = 0;
//bool     btConnected = false;

//#define CHAR_BUF_MAX 2048
//char timeBuf[24];
//char charBuf[CHAR_BUF_MAX];
//int  charLen = 0;
//bool firstTime = true;
//bool nightTime = false;


ESP32_SMA_Inverter_MQTT&  smaInverterApp =  ESP32_SMA_Inverter_MQTT::getInstance();
ESP32_SMA_Inverter& smaInverter = ESP32_SMA_Inverter::getInstance();
ESP32_SMA_MQTT& mqttInstance = ESP32_SMA_MQTT::getInstance();

WiFiClient ESP32_SMA_Inverter_MQTT::espClient = WiFiClient();
PubSubClient ESP32_SMA_Inverter_MQTT::client = PubSubClient(espClient);
WebServer ESP32_SMA_Inverter_MQTT::webServer(80);

int ESP32_SMA_Inverter_MQTT::smartConfig = 0;

// External variables
//extern WebServer webServer;
//extern int smartConfig;


void setup() { 

  Logging::setLevel(esp32m::Info);
  Logging::addAppender(&ETSAppender::instance());
#ifdef SYSLOG_HOST
  udpappender.setMode(UDPAppender::Format::Syslog);
  Logging::addAppender(&udpappender);
#endif
  //Serial.println("added appenders");
  smaInverterApp.logBuild();
  smaInverterApp.appSetup();
}

void ESP32_SMA_Inverter_MQTT::logBuild() {
  logW("v1 Build 2w d (%s) t (%s) "  ,__DATE__ , __TIME__) ; 
}


void ESP32_SMA_Inverter_MQTT::appSetup() { 
  //extern BluetoothSerial SerialBT;
  extern InverterData *pInvData;
  Serial.begin(115200); 
  delay(1000);
  configSetup();
  mqttInstance.wifiStartup();
  
  InverterData& invData = ESP32_SMA_Inverter::getInstance().invData;
  DisplayData& dispData = ESP32_SMA_Inverter::getInstance().dispData;  


  if ( !smartConfig) {
    // Convert the MAC address string to binary
    sscanf(appConfig.smaBTAddress.c_str(), "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx", 
            &smaBTAddress[0], &smaBTAddress[1], &smaBTAddress[2], &smaBTAddress[3], &smaBTAddress[4], &smaBTAddress[5]);
    // Zero the array, all unused butes must be 0
    for(int i = 0; i < sizeof(smaInvPass);i++)
       smaInvPass[i] ='\0';
    strlcpy(smaInvPass , appConfig.smaInvPass.c_str(), sizeof(smaInvPass));

    invData.SUSyID = 0x7d;
    invData.Serial = 0;
    nextTime = millis();
    // reverse inverter BT address
    for(uint8_t i=0; i<6; i++) invData.BTAddress[i] = smaBTAddress[5-i];
    DEBUG2_PRINTF("invData.BTAddress: %02X:%02X:%02X:%02X:%02X:%02X\n",
                invData.BTAddress[5], invData.BTAddress[4], invData.BTAddress[3],
                invData.BTAddress[2], invData.BTAddress[1], invData.BTAddress[0]);
    // *** Start BT
    smaInverter.begin("ESP32toSMA", true);   // "true" creates this device as a BT Master.
    //setpin moved to inverter

  }
  // *** Start WIFI and WebServer



} 


  // **** Loop ************
void loop() { 
  smaInverterApp.appLoop();
}


  // **** Loop ************
void ESP32_SMA_Inverter_MQTT::appLoop() { 
  //extern BluetoothSerial SerialBT;
  int adjustedScanRate;
  // connect or reconnect after connection lost 
  if (nightTime)  // Scan every 15min
    adjustedScanRate = 900000;
  else
    adjustedScanRate = (appConfig.scanRate * 1000);
  if ( !smartConfig && (nextTime < millis()) && (!smaInverter.isBtConnected())) {
    nextTime = millis() + adjustedScanRate;
    if(nightTime)
      DEBUG1_PRINT("Night time - 15min scans\n");

    smaInverter.setPcktID(1);//pcktID = 1;
    // **** Connect SMA **********
    DEBUG1_PRINT("Connecting SMA inverter: \n");
    if (smaInverter.connect(smaBTAddress)) {
      
      // **** Initialize SMA *******
      DEBUG1_PRINTLN("BT connected \n");
      E_RC rc = smaInverter.initialiseSMAConnection();
      DEBUG2_PRINTF("SMA %d \n",rc);
      smaInverter.getBT_SignalStrength();

#ifdef LOGOFF
      // not sure the purpose but SBfSpot code logs off before logging on and this has proved very reliable for me: mrtoy-me 
      smaInverter.logoffSMAInverter();
#endif
      // **** logon SMA ************
      DEBUG1_PRINT("*** logonSMAInverter\n");
      rc = smaInverter.logonSMAInverter(smaInvPass, USERGROUP);
      DEBUG2_PRINTF("Logon return code %d\n",rc);
      smaInverter.ReadCurrentData();
#ifdef LOGOFF    
      //logoff before disconnecting
      smaInverter.logoffSMAInverter();
#endif
      
      smaInverter.disconnect();
      
      //Send Home Assistant autodiscover
      if(appConfig.mqttBroker.length() > 0 && appConfig.hassDisc && firstTime){
        mqttInstance.hassAutoDiscover(appConfig.scanRate *2);
        mqttInstance.logViaMQTT("First boot");
        firstTime=false;
        delay(5000);
      }

      nightTime = mqttInstance.publishData();
      if ( nightTime != dayNight) {
        if (appConfig.mqttBroker.length() > 0) {
          if (nightTime) { // Change the expire time in home Assistant
            mqttInstance.hassAutoDiscover(1800);
          } else {
            mqttInstance.hassAutoDiscover(appConfig.scanRate *2);
          }
        }
        dayNight = nightTime;
      }
    } else {  
      mqttInstance.logViaMQTT("Bluetooth failed to connect");
    } 
  }
  // DEBUG1_PRINT(".");
  mqttInstance.wifiLoop();
    
  delay(100);
}


// Loads the configuration from a file
void ESP32_SMA_Inverter_MQTT::loadConfiguration() {
  // Open file for reading
  File file = LittleFS.open("/config.txt","r");

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<1024> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error)
    log_e("Failed to read file, using default configuration");

  // Copy values from the JsonDocument to the Config         
  std::vector<std::string> keyNames = {"mqttBroker", "mqttPort", "mqttUser","mqttPasswd", "mqttTopic","smaInvPass", "smaBTAddress", "scanRate", "hassDisc"};
  for (uint i=0;i<keyNames.size();i++) {
    std::string k = keyNames[i];
    std::string v = doc[k];
    log_w("load key: %s , value: %s", k.c_str(), v.c_str());
  }

  #ifdef SMA_WIFI_CONFIG_VALUES_H
    appConfig.mqttBroker =  doc["mqttBroker"] | MQTT_BROKER;
    appConfig.mqttPort = doc["mqttPort"] | MQTT_PORT ;
    appConfig.mqttUser = doc["mqttUser"] | MQTT_USER;
    appConfig.mqttPasswd = doc["mqttPasswd"] | MQTT_PASS;
    appConfig.mqttTopic = doc["mqttTopic"] | MQTT_topic;
    appConfig.smaInvPass = doc["smaInvPass"] | SMA_PASS;
    appConfig.smaBTAddress = doc["smaBTAddress"] | SMA_BTADDRESS;
    appConfig.scanRate = doc["scanRate"] | SCAN_RATE ;
    appConfig.hassDisc = doc["hassDisc"] | HASS_DISCOVERY ;
    appConfig.timezone = doc["timezone"] | TIMEZONE;
    appConfig.ntphostname = doc["ntphostname"] | NTPHOSTNAME;
  #else
    appConfig.mqttBroker =  doc["mqttBroker"] | "";
    appConfig.mqttPort = doc["mqttPort"] | 1883 ;
    appConfig.mqttUser = doc["mqttUser"] | "";
    appConfig.mqttPasswd = doc["mqttPasswd"] | "";
    appConfig.mqttTopic = doc["mqttTopic"] | "SMA";
    appConfig.smaInvPass = doc["smaInvPass"] | "password";
    appConfig.smaBTAddress = doc["smaBTAddress"] | "AA:BB:CC:DD:EE:FF";
    appConfig.scanRate = doc["scanRate"] | 60 ;
    appConfig.hassDisc = doc["hassDisc"] | true ;
    appConfig.timezone = doc["timezone"] | "";
    appConfig.ntpHostname = doc["ntphostname"] | "";
  #endif

  
  // Close the file (Curiously, File's destructor doesn't close the file)
  file.close();
  
}



// Saves the configuration to a file
void ESP32_SMA_Inverter_MQTT::saveConfiguration() {
  // Delete existing file, otherwise the configuration is appended to the file
  if (LittleFS.remove("/config.txt")) {
    log_w("removed file %s", "/config.txt");
  } else {
    log_e("failed to removed file %s", "/config.txt");
  }

  // Open file for writing
  log_i("creating file %s mode w", "/config.txt");
  File file = LittleFS.open("/config.txt", "w");
  if (!file) {
    log_e("Failed to create file");
    return;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonDocument<1024> doc;

  // Set the values in the document
  doc["mqttBroker"] = appConfig.mqttBroker;
  doc["mqttPort"] = appConfig.mqttPort;
  doc["mqttPort"] = appConfig.mqttPort;
  doc["mqttUser"] = appConfig.mqttUser;
  doc["mqttPasswd"] = appConfig.mqttPasswd;
  doc["mqttTopic"] = appConfig.mqttTopic; 
  doc["smaInvPass"] = appConfig.smaInvPass;
  doc["smaBTAddress"] = appConfig.smaBTAddress;
  doc["scanRate"] = appConfig.scanRate;
  doc["hassDisc"] = appConfig.hassDisc;
  doc["timezone"] = appConfig.timezone;
  doc["ntphostname"] = appConfig.ntphostname;

  std::vector<std::string> keyNames = {"mqttBroker", "mqttPort", "mqttUser","mqttPasswd", "mqttTopic","smaInvPass", "smaBTAddress", "scanRate", "hassDisc", "timezone", "ntphostname"};
  for (uint i=0;i<keyNames.size();i++) {
    std::string k = keyNames[i];
    std::string v = doc[k];
    log_w("save key: %s , value: %s", k.c_str(), v.c_str());
  }
 
  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    log_e("Failed to write to file");
  } else {
    log_w("wrote to file");
  }

  // Close the file
  file.close();
  log_d("close file");
}



// Prints the content of a file to the Serial
void ESP32_SMA_Inverter_MQTT::printFile() {
  // Open file for reading
  File file = LittleFS.open("/config.txt","r");
  if (!file) {
    log_e("Failed to read file");
    return;
  }

  // Extract each characters by one by one
  while (file.available()) {
    Serial.print((char)file.read());
  }
  Serial.println();

  // Close the file
  file.close();
}

void ESP32_SMA_Inverter_MQTT::configSetup() {
  
  if (!LittleFS.begin(false)) {
    log_e("LittleFS mount failed");
    if (!LittleFS.begin(true /* true: format */)) {
      Serial.println("Failed to format LittleFS");
    } else {
      Serial.println("LittleFS formatted successfully");  return;
    }
  } else{
    log_w("little fs mount sucess");
  }

  // Should load default config if run for the first time
  log_w("Loading configuration...");
  loadConfiguration( );

  // Create configuration file
  log_w("Saving configuration...");
  saveConfiguration( );

  // Dump config file
  log_w("Print config file...");
  printFile();
}



void ESP32_SMA_Inverter_MQTT::rmfiles(){
  if (LittleFS.remove("/config.txt")) {
    log_w("%s removed", "/config.txt");
  } else {
    log_e("%s removal failed", "/config.txt");
  }
}