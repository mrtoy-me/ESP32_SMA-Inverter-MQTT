/* MIT License

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
// Wifi Functions choose between Station or SoftAP

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>

// MQTT configuration
struct Config {
  String mqttBroker;
  int mqttPort;
  String mqttUser;
  String mqttPasswd;
  String mqttTopic;
  float calConst;
};
Config config;

int needDNS = 0;
const byte DNS_PORT = 53;
DNSServer         dnsServer;
IPAddress         apIP(10, 10, 10, 1);

WebServer webServer(80);

void wifiStartup(){

  // Build Hostname
  char sapString[21];
 

  snprintf(sapString, 20, "SMA-%08X", ESP.getEfuseMac());

  // Attempt to connecty to the AP stored on board, if not, start in SoftAP mode
  WiFi.mode(WIFI_STA);
  delay(2000);
  WiFi.hostname(sapString);
  WiFi.begin();
  delay(5000);
  
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_AP);
    Serial.print("Running in AP mode AP name: ");
    Serial.println(sapString);
    delay(2000);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(sapString);
    // if DNSServer is started with "*" for domain name, it will reply with
    // provided IP to all DNS request
    dnsServer.start(DNS_PORT, "*", apIP);
    needDNS = 1;
  } else {
  
    String hostName = "Hostname: ";
    hostName = hostName + sapString;
    Serial.println(hostName);
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    needDNS = 0;
  } 
  
  webServer.begin();
  webServer.on("/", formPage);
  webServer.on("/", connectAP);

}

// Configure wifi using ESP Smartconfig app on phone
void mySmartConfig() {
  // Wipe current credentials
  WiFi.disconnect(true); // deletes the wifi credentials
  
  WiFi.mode(WIFI_STA);
  delay(2000);
  WiFi.begin();
  WiFi.beginSmartConfig();

  //Wait for SmartConfig packet from mobile
  Serial.println("Waiting for SmartConfig.");
  while (!WiFi.smartConfigDone()) {
    delay(500);
    Serial.print(".");
  }



  Serial.println("");
  Serial.println("SmartConfig received.");

  //Wait for WiFi to connect to AP
  Serial.println("Waiting for WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
}

// Use ESP SmartConfig to connect to wifi
void connectAP(){
  webServer.send(200, "text/plain", "Open ESPTouch: Smartconfig App to connect to Wifi Network"); 
  delay(2000);
  mySmartConfig();
  
}


void wifiLoop(){
  if (needDNS) {
    dnsServer.processNextRequest();
  }
}

void formPage () {
  char tempstr[1024];
  char *responseHTML;

  responseHTML = (char *)malloc(10000);

  strcpy(responseHTML, "<!DOCTYPE html><html><head>\
                      <title>Battery Charger</title></head><body>\
                      <style>\
                      table {\
  border-collapse: collapse;\
  width: 100%;\
  font-size: 30px;\
}\
\
table, th, td {\
  border: 1px solid black;\
}\
</style>\
<H1> SMA Bluetooth Configuration</H1>\
<form method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"/postform/\">");


  strcat(responseHTML, "<TABLE><TR><TH>Configuration</TH><TH>Setting</TH>\n");
  sprintf(tempstr, "<TR><TD>Inverter Bluetooth Address :</TD><TD> <input type=\"text\" name=\"mqttBroker\" value=\"%h\"></TD><TR>\n\n",SmaBTAddress);
  strcat(responseHTML, tempstr);
  sprintf(tempstr, "<TR><TD>MQTT Inverter Password :</TD><TD> <input type=\"text\" name=\"mqttBroker\" value=\"%s\"></TD><TR>\n\n",SmaInvPass);
  strcat(responseHTML, tempstr);
  sprintf(tempstr, "<TR><TD>MQTT Broker Hostname or IP Address :</TD><TD> <input type=\"text\" name=\"mqttBroker\" value=\"%s\"></TD><TR>\n\n",config.mqttBroker.c_str());
  strcat(responseHTML, tempstr);
  sprintf(tempstr, "<TR><TD>MQTT Broker Port : </TD><TD><input type=\"text\" name=\"mqttPort\" value=\"%d\"></TD><TR>\n\n",config.mqttPort);
  strcat(responseHTML, tempstr);
  sprintf(tempstr, "<TR><TD>MQTT Broker User :</TD><TD> <input type=\"text\" name=\"mqttUser\" value=\"%s\"></TD><TR>\n\n",config.mqttUser.c_str());
  strcat(responseHTML, tempstr);
  sprintf(tempstr, "<TR><TD>MQTT Broker Password :</TD><TD> <input type=\"text\" name=\"mqttPasswd\" value=\"%s\"></TD><TR>\n\n",config.mqttPasswd.c_str());
  strcat(responseHTML, tempstr);
  sprintf(tempstr, "<TR><TD>MQTT Topic :</TD><TD> <input type=\"text\" name=\"mqttTopic\" value=\"%s\"></TD><TR>\n\n",config.mqttTopic.c_str());
  strcat(responseHTML, tempstr);


  strcat(responseHTML, "</TABLE>");
  strcat(responseHTML, "<input type=\"submit\" value=\"Submit\"></form>");
  // if (DEBUG)
  //  Serial.print(responseHTML);
  delay(100);// Serial.print(responseHTML);
  webServer.send(200, "text/html", responseHTML);

}
