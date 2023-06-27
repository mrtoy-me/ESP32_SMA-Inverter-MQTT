#ifndef CONFIG.H
#define CONFIG.H

//*** debug ****************
// 0=no Debug; 
// 1=values only; 
// 2=values and info and P-buffer
// 3=values and info and T+R+P-buffer
#define DEBUG_SMA 1

#define LOOPTIME_SEC 30

// SMA login password for UG_USER or UG_INSTALLER always 12 char. Unused=0x00
#define USERGROUP UG_USER
/* char SmaInvPass[12]={'Z','a','q','1','2','w','s','x',0,0,0,0};  //Zaq12wsx

// SMA blutooth address -> adapt to your system
uint8_t SmaBTAddress[6]= {0x00, 0x80, 0x25, 0x27, 0x38, 0xE7}; // my SMA SMC6000TL 00:80:25:27:38:E7
*/
// Configuration struct




#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>

struct Config {
  String mqttBroker;
  int mqttPort;
  String mqttUser;
  String mqttPasswd;
  String mqttTopic;
  String SmaInvPass;
  String SmaBTAddress;
  int ScanRate;
};

const char *confFile = "/config.txt";  

//Prototypes
void loadConfiguration(const char *filename, Config &config);
void saveConfiguration(const char *confFile, const Config &config);
void printFile(const char *confFile);
void configSetup();
void rmfiles();
#endif