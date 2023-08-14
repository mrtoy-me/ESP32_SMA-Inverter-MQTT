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

#include "ESP32_SMA-Inverter-MQTT.h"

#define FORMAT_LITTLEFS_IF_FAILED 

//ESP32_SMA_Inverter_MQTT& invertermqttapp = ;
//ESP32_SMA_Inverter& smaInverter = ESP32_SMA_Inverter::getInstance();

//link to singleton methods
extern void E_formPage() {
  ESP32_SMA_MQTT::getInstance().formPage();
}

extern void E_connectAP() {
  ESP32_SMA_MQTT::getInstance().connectAP();
}

extern void E_handleForm() {
  ESP32_SMA_MQTT::getInstance().handleForm();
}

void ESP32_SMA_MQTT::wifiTime() {
  AppConfig& config = ESP32_SMA_Inverter_MQTT::getInstance().appConfig;
  const long  gmtOffset_sec = 3600;   // offset seconds, this depends on your time zone (3600 is GMT +1)
  const int   daylightOffset_sec = 3600;  // daylight saving offset seconds
  logD("Setting time via %s, gmt: %d, dst: %d ", config.ntphostname.c_str(), gmtOffset_sec, daylightOffset_sec);


  configTime(gmtOffset_sec, daylightOffset_sec, config.ntphostname.c_str());

  String t = getTime();
  logI("Time %s" , t.c_str());

}


String ESP32_SMA_MQTT::getTime() {
  struct tm timeinfo;
  for (int i=0;i<5;i++) {
    logD(".time.");
    if(getLocalTime(&timeinfo)) {
      char charTime[64];
      strftime(charTime, sizeof(charTime), "%A, %B %d %Y %H:%M:%S", &timeinfo);
      logD("now : %s ", charTime);  
      return String(charTime);
    }
    delay(500);
  }
  logE("failed to obtain time");
  return String("");
}

void ESP32_SMA_MQTT::wifiStartup(){
  // Build Hostname
  char aSapString[20]="";
  snprintf(aSapString, 20, "SMA-%08X", ESP.getEfuseMac());
  sapString = String(aSapString);
  logD(sapString.c_str());

  // Attempt to connect to the AP stored on board, if not, start in SoftAP mode
  WiFi.mode(WIFI_STA);
  delay(2000);

  logD("setHostname");
  WiFi.hostname(sapString);

#ifdef WIFI_SSID
  //overriding ssid and hostname
  logD("wifi begin with ssid(%s) and password (.......)", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); 
  for  (int w=0; w<=10 || WiFi.status() != WL_CONNECTED; w++) {
    delay(500);
    logD(".wifi.");
  }
#else
  WiFi.begin();
#endif

  //delay(2000);
  logD("Using config");

  AppConfig& config = ESP32_SMA_Inverter_MQTT::getInstance().appConfig;
  logD("mqtt topic: %s", config.mqttTopic);
  if (config.mqttTopic == "") 
    config.mqttTopic = sapString;
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    // Launch smartconfig to reconfigure wifi
    logD(">");
    if (i > 3)
      mySmartConfig();
    i++;
    delay(1000);
  }
  // Success connecting 
  ESP32_SMA_Inverter_MQTT::smartConfig = 0; 
  String hostName = sapString;
  logW("hostname %s", hostName);
  logW("IP Address: %s", ((String)WiFi.localIP().toString()).c_str());
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);


  wifiTime();


  ESP32_SMA_Inverter_MQTT::webServer.begin();
  ESP32_SMA_Inverter_MQTT::webServer.on("/", E_formPage);
  ESP32_SMA_Inverter_MQTT::webServer.on("/smartconfig", E_connectAP);
  ESP32_SMA_Inverter_MQTT::webServer.on("/postform/", E_handleForm);

  logI("Web Server Running: ");
  
}

// Configure wifi using ESP Smartconfig app on phone
void ESP32_SMA_MQTT::mySmartConfig() {
  logD("smartConfig");
  // Wipe current credentials
  // WiFi.disconnect(true); // deletes the wifi credentials
  
  WiFi.mode(WIFI_STA);
  delay(2000);
  WiFi.begin();
  WiFi.beginSmartConfig();

  //Wait for SmartConfig packet from mobile
  logI("Waiting for SmartESP32_SMA_Inverter_App_Config::config");
  // if no smartconfig received after 5 minutes, reboot and try again
  int count = 0;
  while (!WiFi.smartConfigDone()) {
    delay(500);
    logD(".");
    if (count++ > 600 ) ESP.restart();
  }



  logD("");
  logI("SmartConfig received.");

  //Wait for WiFi to connect to AP
  logW("Waiting for WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    logD(".");
  }
  logW("IP Address: %s", WiFi.localIP());
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  logW("Restarting in 5 seconds");
  delay(5000);
  ESP.restart();
}

// Use ESP SmartConfig to connect to wifi
void ESP32_SMA_MQTT::connectAP(){
  ESP32_SMA_Inverter_MQTT::webServer.send(200, "text/plain", "Open ESPTouch: Smartconfig App to connect to Wifi Network"); 
  delay(2000);
  mySmartConfig();
  
}


void ESP32_SMA_MQTT::wifiLoop(){
  // Attempt to reconnect to Wifi if disconnected
  if ( WiFi.status() != WL_CONNECTED) {
    
    WiFi.disconnect();
    WiFi.reconnect();
  }
  unsigned long currentMillis = millis();
  // if WiFi is down, try reconnecting
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >= interval)) {
    log_w("Reconnecting to WiFi...\n");
    WiFi.disconnect();
    WiFi.reconnect();
    previousMillis = currentMillis;
  }
  ESP32_SMA_Inverter_MQTT::webServer.handleClient();  
}

void ESP32_SMA_MQTT::formPage () {
  char tempstr[2048];
  char *responseHTML;
  InverterData& invData = ESP32_SMA_Inverter::getInstance().invData;
  DisplayData& dispData = ESP32_SMA_Inverter::getInstance().dispData;  
  AppConfig& config = ESP32_SMA_Inverter_MQTT::getInstance().appConfig;
  char fulltopic[100];

  responseHTML = (char *)malloc(sizeof(char)*10000);
  log_w("Connect formpage\n");
  strcpy(responseHTML, "<!DOCTYPE html><html><head>\
                      <title>SMA Inverter</title></head><body>\
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


  strcat(responseHTML, "<TABLE><TR><TH>Configuration</TH><TH>Setting</TH></TR>\n");
  sprintf(tempstr, "<TR><TD>Inverter Bluetooth Address (Format AA:BB:CC:DD:EE:FF) : </TD><TD> <input type=\"text\" name=\"btaddress\" value=\"%s\"></TD><TR>\n\n",config.smaBTAddress.c_str());
  strcat(responseHTML, tempstr);
  sprintf(tempstr, "<TR><TD>MQTT Inverter Password :</TD><TD> <input type=\"text\" name=\"smapw\" value=\"%s\"></TD><TR>\n\n",config.smaInvPass.c_str());
  strcat(responseHTML, tempstr);
  sprintf(tempstr, "<TR><TD>MQTT Broker Hostname or IP Address :</TD><TD> <input type=\"text\" name=\"mqttBroker\" value=\"%s\"></TD><TR>\n\n",config.mqttBroker.c_str());
  strcat(responseHTML, tempstr);
  sprintf(tempstr, "<TR><TD>MQTT Broker Port : </TD><TD><input type=\"text\" name=\"mqttPort\" value=\"%d\"></TD><TR>\n\n",config.mqttPort);
  strcat(responseHTML, tempstr);
  sprintf(tempstr, "<TR><TD>MQTT Broker User :</TD><TD> <input type=\"text\" name=\"mqttUser\" value=\"%s\"></TD><TR>\n\n",config.mqttUser.c_str());
  strcat(responseHTML, tempstr);
  sprintf(tempstr, "<TR><TD>MQTT Broker Password :</TD><TD> <input type=\"text\" name=\"mqttPasswd\" value=\"%s\"></TD><TR>\n\n",config.mqttPasswd.c_str());
  strcat(responseHTML, tempstr);
  sprintf(tempstr, "<TR><TD>MQTT Topic Preamble:</TD><TD> <input type=\"text\" name=\"mqttTopic\" value=\"%s\"></TD><TR>\n\n",config.mqttTopic.c_str());
  strcat(responseHTML, tempstr);
  sprintf(tempstr, "<TR><TD>Inverter scan rate:</TD><TD> <input type=\"text\" name=\"scanRate\" value=\"%d\"></TD><TR>\n\n",config.scanRate);
  strcat(responseHTML, tempstr);
  sprintf(tempstr, "<TR><TD>Timezone:</TD><TD> <input type=\"text\" name=\"timezone\" value=\"%s\"></TD><TR>\n\n",config.timezone.c_str());
  strcat(responseHTML, tempstr);
  sprintf(tempstr, "<TR><TD>NTP host:</TD><TD> <input type=\"text\" name=\"ntphostname\" value=\"%s\"></TD><TR>\n\n",config.ntphostname.c_str());
  strcat(responseHTML, tempstr);

  if (config.hassDisc) {
    strcat(responseHTML, "<TR><TD>Home Assistant Auto Discovery:</TD><TD> <input type=\"checkbox\" name=\"hassDisc\" checked ></TD><TR>\n");
    snprintf(fulltopic,sizeof(fulltopic),"homeassistant/sensor/%s-%d/state",config.mqttTopic.c_str(),invData.Serial);
  } else {
    strcat(responseHTML, "<TR><TD>Home Assistant Auto Discovery:</TD><TD> <input type=\"checkbox\" name=\"hassDisc\"></TD><TR>\n");
    snprintf(fulltopic,sizeof(fulltopic),"%s-%d/state",config.mqttTopic.c_str(),invData.Serial);
  }
  strcat(responseHTML, "</TABLE>");
  strcat(responseHTML, "<input type=\"submit\" value=\"Submit\"></form><BR> <A href=\"/smartconfig\">Enable ESP Touch App smart config</A><BR>");


  strcat(responseHTML, "<TABLE><TR><TH>Last Scan</TH><TH>Data</TH>\n");
  
  
  snprintf(tempstr, sizeof(tempstr),
"<tr><td>MQTT Topic</td><td>%s</td></tr>\n\
 <tr><td>BT Signal Strength</td><td>%4.1f %</td></tr>\n\
  <tr><td>Uac</td><td>%15.1f V</td></tr>\n\
 <tr><td>Iac</td><td>%15.1f A</td></tr>\n\
 <tr><td>Pac</td><td>%15.1f kW</td></tr>\n\
 <tr><td>Udc</td><td>String 1: %15.1f V, String 2: %15.1f V</td></tr>\n\
 <tr><td>Idc</td><td>String 1: %15.1f A, String 2: %15.1f A</td></tr>\n\
 <tr><td>Wdc</td><td>String 1: %15.1f kW, String 2: %15.1f kW</td></tr>\n"
 , fulltopic
 , dispData.BTSigStrength
 , dispData.Uac
 , dispData.Iac
 , dispData.Pac
 , dispData.Udc[0], dispData.Udc[1]
 , dispData.Idc[0], dispData.Idc[1]
 , dispData.Udc[0] * dispData.Idc[0] / 1000 , dispData.Udc[1] * dispData.Idc[1] / 1000);

  strcat(responseHTML, tempstr);

  snprintf(tempstr, sizeof(tempstr),
"<tr><td>Frequency</td><td>%5.2f Hz</td></tr>\n\
 <tr><td>E-Today</td><td>%15.1f kWh</td></tr>\n\
 <tr><td>E-Total</td><td>%15.1f kWh</td></tr>\n "
 , dispData.Freq
 , dispData.EToday
 , dispData.ETotal);
  strcat(responseHTML, tempstr);
  strcat(responseHTML,"</TABLE></body></html>\n");
  delay(100);// Serial.print(responseHTML);
  ESP32_SMA_Inverter_MQTT::webServer.send(200, "text/html", responseHTML);

  free(responseHTML);
}

// Function to extract the configuration
void ESP32_SMA_MQTT::handleForm() {
  AppConfig& config = ESP32_SMA_Inverter_MQTT::getInstance().appConfig;
  
  log_w("Connect handleForm\n");
  if (ESP32_SMA_Inverter_MQTT::webServer.method() != HTTP_POST) {
    ESP32_SMA_Inverter_MQTT::webServer.send(405, "text/plain", "Method Not Allowed");
  } else {
    log_w("POST form was:");
    config.hassDisc = false; 
    for (uint8_t i = 0; i < ESP32_SMA_Inverter_MQTT::webServer.args(); i++) {
      String name = ESP32_SMA_Inverter_MQTT::webServer.argName(i);
      String v = ESP32_SMA_Inverter_MQTT::webServer.arg(i);
      v.trim();
      log_w("%s: %s ",name.c_str(), v.c_str());
      if (name == "mqttBroker") {
        config.mqttBroker = v.c_str();
      } else if (name == "mqttPort") {
        String val = v.c_str();
        config.mqttPort = val.toInt();   
      } else if (name == "mqttUser") {
        config.mqttUser = v.c_str();
      } else if (name == "mqttPasswd") {
        config.mqttPasswd = v.c_str();
      } else if (name == "mqttTopic") {
        config.mqttTopic = v.c_str();
      } else if (name == "btaddress") {
        config.smaBTAddress = v.c_str();
      } else if (name == "smapw") {
        config.smaInvPass = v.c_str();
      } else if (name == "scanRate") {
        config.scanRate = atoi(v.c_str());
      } else if (name == "hassDisc") {
        log_w("%s\n",v.c_str());
        config.hassDisc = true;
      } else if (name == "timezone") {
        config.timezone = v.c_str();
      } else if (name == "ntphostname") {
        config.ntphostname = v.c_str();
      }

    }
    ESP32_SMA_Inverter_MQTT::getInstance().saveConfiguration();
    ESP32_SMA_Inverter_MQTT::getInstance().printFile();
    delay(3000);
    ESP.restart();
  }
}

void ESP32_SMA_MQTT::brokerConnect() {
  AppConfig& config = ESP32_SMA_Inverter_MQTT::getInstance().appConfig;
  if(config.mqttBroker.length() < 1 ){
    return;
  }
  log_w("Connecting to MQTT Broker");

  ESP32_SMA_Inverter_MQTT::client.setServer(config.mqttBroker.c_str(), config.mqttPort);

  // client.setCallback(callback);
  for(int i =0; i < 3;i++) {
    if ( !ESP32_SMA_Inverter_MQTT::client.connected()){
      log_w("The client %s connects to the mqtt broker %s ", sapString.c_str(), config.mqttBroker.c_str());
      // If there is a user account
      if(config.mqttUser.length() > 1){
        log_w(" with user/password\n");
        if (ESP32_SMA_Inverter_MQTT::client.connect(sapString.c_str(),config.mqttUser.c_str(),config.mqttPasswd.c_str())) {
        } else {
          log_e("mqtt connect failed with state %i",ESP32_SMA_Inverter_MQTT::client.state());
          delay(2000);
        }
      } else {
        log_w(" without user/password ");
        if (ESP32_SMA_Inverter_MQTT::client.connect(sapString.c_str())) {
        } else {
          log_e("mqtt connect failed with state %i", ESP32_SMA_Inverter_MQTT::client.state());
          delay(2000);
        }
      }
    }
  }
}

// Returns true if nighttime
bool ESP32_SMA_MQTT::publishData(){
  InverterData& invData = ESP32_SMA_Inverter::getInstance().invData;
  DisplayData& dispData = ESP32_SMA_Inverter::getInstance().dispData;  
  AppConfig& config = ESP32_SMA_Inverter_MQTT::getInstance().appConfig;

  if(config.mqttBroker.length() < 1 ){
    return(false);
  }

  brokerConnect();
  if (ESP32_SMA_Inverter_MQTT::client.connected()){
    char theData[2000];
    // char tmpstr[100];


    snprintf(theData,sizeof(theData)-1,
    "{ \"Serial\": %d, \"BTStrength\": %6.2f, \"Uac\": [ %6.2f, %6.2f, %6.2f ], \"Iac\": [ %6.2f, %6.2f, %6.2f ], \"Pac\": %6.2f, \"Udc\": [ %6.2f , %6.2f ], \"Idc\": [ %6.2f , %6.2f ], \"Wdc\": [%6.2f , %6.2f ], \"Freq\": %5.2f, \"EToday\": %6.2f, \"ETotal\": %15.2f, \"InvTemp\": %4.2f, \"DevStatus\": %d, \"GridRelay\": %d }"
 , invData.Serial
 , dispData.BTSigStrength
 , dispData.Uac[0],dispData.Uac[1],dispData.Uac[2]
 , dispData.Iac[0],dispData.Iac[1],dispData.Iac[1]
 , dispData.Pac
 , dispData.Udc[0], dispData.Udc[1]
 , dispData.Idc[0], dispData.Idc[1]
 , dispData.Udc[0] * dispData.Idc[0] / 1000 , dispData.Udc[1] * dispData.Idc[1] / 1000
 , dispData.Freq
 , dispData.EToday
 , dispData.ETotal
 , dispData.InvTemp
 , invData.DevStatus
 , invData.GridRelay
);


    // strcat(theData,"}");
    char topic[100];
    if (config.hassDisc)
      snprintf(topic,sizeof(topic), "homeassistant/sensor/%s-%d/state",config.mqttTopic.c_str(), invData.Serial);
    else
      snprintf(topic,sizeof(topic), "%s-%d/state",config.mqttTopic.c_str(), invData.Serial);
    logI(topic);
    logI(" = ");
    logI("%s\n",theData);
    int len = strlen(theData);
    ESP32_SMA_Inverter_MQTT::client.beginPublish(topic,len,false);
    if (ESP32_SMA_Inverter_MQTT::client.print(theData))
      logI("Published\n");
    else
      logW("Failed Publish\n");
    ESP32_SMA_Inverter_MQTT::client.endPublish();
  }
  // If Power is zero, it's night time
  if (dispData.Pac > 0) 
    return(false);
  else
    return(true);
}

void ESP32_SMA_MQTT::logViaMQTT(const char *logStr){
  InverterData& invData = ESP32_SMA_Inverter::getInstance().invData;
  DisplayData& dispData = ESP32_SMA_Inverter::getInstance().dispData;  
  AppConfig& config = ESP32_SMA_Inverter_MQTT::getInstance().appConfig;

  char tmp[1000];
  if(config.mqttBroker.length() < 1 ){
    return;
  }
  snprintf(tmp,sizeof(tmp),"{ \"Log\": \"%s\" }",logStr);
  brokerConnect();
  
  if (ESP32_SMA_Inverter_MQTT::client.connected()){

    // strcat(theData,"}");
    char topic[100];
    snprintf(topic,sizeof(topic), "homeassistant/sensor/%s-%d/state",config.mqttTopic.c_str(), invData.Serial);
    logI(topic);
    logI(" = ");
    logI(" %s\n",tmp);
    int len = strlen(tmp);
    ESP32_SMA_Inverter_MQTT::client.beginPublish(topic,len,false);
    if (ESP32_SMA_Inverter_MQTT::client.print(tmp))
      logI("Published\n");
    else
      logW("Failed Publish\n");
    ESP32_SMA_Inverter_MQTT::client.endPublish();
  }
  
}


// Set up the topics in home assistant
void ESP32_SMA_MQTT::hassAutoDiscover(int timeout){

  InverterData& invData = ESP32_SMA_Inverter::getInstance().invData;
  DisplayData& dispData = ESP32_SMA_Inverter::getInstance().dispData;  
  AppConfig& config = ESP32_SMA_Inverter_MQTT::getInstance().appConfig;

  char tmpstr[1000];
  char topic[30];
  brokerConnect();
  
  snprintf(topic,sizeof(topic)-1, "%s-%d",config.mqttTopic.c_str(), invData.Serial);
  snprintf(tmpstr,sizeof(tmpstr)-1, "{\"device_class\": \"power\", \"name\": \"%s AC Power\" , \"state_topic\": \"homeassistant/sensor/%s/state\", \"unit_of_measurement\": \"kW\",  \"expire_after\": %d, \"value_template\": \"{{ value_json.Pac }}\" }",topic,topic,timeout);
  sendLongMQTT(topic,"Pac",tmpstr);
  snprintf(tmpstr,sizeof(tmpstr)-1, "{\"device_class\": \"current\", \"name\": \"%s A Phase Current\" , \"state_topic\": \"homeassistant/sensor/%s/state\", \"unit_of_measurement\": \"A\",  \"expire_after\": %d, \"value_template\": \"{{ value_json.Iac[0] }}\" }",topic,topic,timeout);
  sendLongMQTT(topic,"IacA",tmpstr);
  snprintf(tmpstr,sizeof(tmpstr)-1, "{\"device_class\": \"current\", \"name\": \"%s B Phase Current\" , \"state_topic\": \"homeassistant/sensor/%s/state\", \"unit_of_measurement\": \"A\",  \"expire_after\": %d, \"value_template\": \"{{ value_json.Iac[1] }}\" }",topic,topic,timeout);
  sendLongMQTT(topic,"IacB",tmpstr);
  snprintf(tmpstr,sizeof(tmpstr)-1, "{\"device_class\": \"current\", \"name\": \"%s C Phase Current\" , \"state_topic\": \"homeassistant/sensor/%s/state\", \"unit_of_measurement\": \"A\",  \"expire_after\": %d, \"value_template\": \"{{ value_json.Iac[2] }}\" }",topic,topic,timeout);
  sendLongMQTT(topic,"IacC",tmpstr);
  snprintf(tmpstr,sizeof(tmpstr)-1, "{\"device_class\": \"voltage\", \"name\": \"%s A Phase Voltage\" , \"state_topic\": \"homeassistant/sensor/%s/state\", \"unit_of_measurement\": \"V\",  \"expire_after\": %d, \"value_template\": \"{{ value_json.Uac[0] }}\" }",topic,topic,timeout);
  sendLongMQTT(topic,"UacA",tmpstr);
  snprintf(tmpstr,sizeof(tmpstr)-1, "{\"device_class\": \"voltage\", \"name\": \"%s B Phase Voltage\" , \"state_topic\": \"homeassistant/sensor/%s/state\", \"unit_of_measurement\": \"V\",  \"expire_after\": %d, \"value_template\": \"{{ value_json.Uac[1] }}\" }",topic,topic,timeout);
  sendLongMQTT(topic,"UacB",tmpstr);
  snprintf(tmpstr,sizeof(tmpstr)-1, "{\"device_class\": \"voltage\", \"name\": \"%s C Phase Voltage\" , \"state_topic\": \"homeassistant/sensor/%s/state\", \"unit_of_measurement\": \"V\",  \"expire_after\": %d, \"value_template\": \"{{ value_json.Uac[2] }}\" }",topic,topic,timeout);
  sendLongMQTT(topic,"UacC",tmpstr);
  snprintf(tmpstr,sizeof(tmpstr)-1, "{\"device_class\": \"frequency\", \"name\": \"%s AC Frequency\" , \"state_topic\": \"homeassistant/sensor/%s/state\", \"unit_of_measurement\": \"Hz\",  \"expire_after\": %d, \"value_template\": \"{{ value_json.Freq }}\" }",topic,topic,timeout);
  sendLongMQTT(topic,"Freq",tmpstr);
  snprintf(tmpstr,sizeof(tmpstr)-1, "{\"device_class\": \"power\", \"name\": \"%s DC Power (String 1)\" , \"state_topic\": \"homeassistant/sensor/%s/state\", \"unit_of_measurement\": \"kW\",  \"expire_after\": %d, \"value_template\": \"{{ value_json.Wdc[0] }}\" }",topic,topic,timeout);
  sendLongMQTT(topic,"Wdc1",tmpstr);
  snprintf(tmpstr,sizeof(tmpstr)-1, "{\"device_class\": \"power\", \"name\": \"%s DC Power (String 2)\" , \"state_topic\": \"homeassistant/sensor/%s/state\", \"unit_of_measurement\": \"kW\",  \"expire_after\": %d, \"value_template\": \"{{ value_json.Wdc[1] }}\" }",topic,topic,timeout);
  sendLongMQTT(topic,"Wdc2",tmpstr);
  snprintf(tmpstr,sizeof(tmpstr)-1, "{\"device_class\": \"voltage\", \"name\": \"%s DC Voltage (String 1)\" , \"state_topic\": \"homeassistant/sensor/%s/state\", \"unit_of_measurement\": \"V\",  \"expire_after\": %d, \"value_template\": \"{{ value_json.Udc[0] }}\" }",topic,topic,timeout);
  sendLongMQTT(topic,"Udc1",tmpstr);
  snprintf(tmpstr,sizeof(tmpstr)-1, "{\"device_class\": \"voltage\", \"name\": \"%s DC Voltage (String 2)\" , \"state_topic\": \"homeassistant/sensor/%s/state\", \"unit_of_measurement\": \"V\",  \"expire_after\": %d, \"value_template\": \"{{ value_json.Udc[1] }}\" }",topic,topic,timeout);
  sendLongMQTT(topic,"Udc2",tmpstr);
  snprintf(tmpstr,sizeof(tmpstr)-1, "{\"device_class\": \"current\", \"name\": \"%s DC Current (String 1)\" , \"state_topic\": \"homeassistant/sensor/%s/state\", \"unit_of_measurement\": \"A\",  \"expire_after\": %d, \"value_template\": \"{{ value_json.Idc[0] }}\" }",topic,topic,timeout);
  sendLongMQTT(topic,"Idc1",tmpstr);
  snprintf(tmpstr,sizeof(tmpstr)-1, "{\"device_class\": \"current\", \"name\": \"%s DC Current (String 2)\" , \"state_topic\": \"homeassistant/sensor/%s/state\", \"unit_of_measurement\": \"A\",  \"expire_after\": %d, \"value_template\": \"{{ value_json.Idc[1] }}\" }",topic,topic,timeout);
  sendLongMQTT(topic,"Idc2",tmpstr);
  snprintf(tmpstr,sizeof(tmpstr)-1, "{\"device_class\": \"energy\", \"name\": \"%s kWh Today\" , \"state_topic\": \"homeassistant/sensor/%s/state\", \"unit_of_measurement\": \"kWh\",  \"expire_after\": %d, \"value_template\": \"{{ value_json.EToday }}\" }",topic,topic,timeout);
  sendLongMQTT(topic,"EToday",tmpstr);
  snprintf(tmpstr,sizeof(tmpstr)-1, "{\"device_class\": \"energy\", \"name\": \"%s kWh Total\" , \"state_topic\": \"homeassistant/sensor/%s/state\", \"unit_of_measurement\": \"kWh\",  \"expire_after\": %d, \"value_template\": \"{{ value_json.ETotal }}\" }",topic,topic,timeout);
  sendLongMQTT(topic,"ETotal",tmpstr);
  snprintf(tmpstr,sizeof(tmpstr)-1, "{\"device_class\": \"temperature\", \"name\": \"%s Inverter Temperature\" , \"state_topic\": \"homeassistant/sensor/%s/state\", \"unit_of_measurement\": \"C\",  \"expire_after\": %d, \"value_template\": \"{{ value_json.InvTemp }}\" }",topic,topic,timeout);
  sendLongMQTT(topic,"InvTemp",tmpstr);
  snprintf(tmpstr,sizeof(tmpstr)-1, "{\"name\": \"%s Device Status\" , \"state_topic\": \"homeassistant/sensor/%s/state\",  \"expire_after\": %d, \"value_template\": \"{{ value_json.DevStatus }}\" }",topic,topic,timeout);
  sendLongMQTT(topic,"DevStatus",tmpstr);
  snprintf(tmpstr,sizeof(tmpstr)-1, "{\"name\": \"%s Grid Relay Status\" , \"state_topic\": \"homeassistant/sensor/%s/state\",  \"expire_after\": %d, \"value_template\": \"{{ value_json.GridRelay }}\" }",topic,topic,timeout);
  sendLongMQTT(topic,"GridRelay",tmpstr);
  snprintf(tmpstr,sizeof(tmpstr)-1, "{\"name\": \"%s Bluetooth\" , \"state_topic\": \"homeassistant/sensor/%s/state\",\"unit_of_measurement\": \"%%\",  \"expire_after\": %d, \"value_template\": \"{{ value_json.BTStrength }}\" }",topic,topic,timeout);
  sendLongMQTT(topic,"Bluetooth",tmpstr);
  snprintf(tmpstr,sizeof(tmpstr)-1, "{\"name\": \"%s Log\" , \"state_topic\": \"homeassistant/sensor/%s/state\",  \"expire_after\": %d, \"value_template\": \"{{ value_json.Log }}\" }",topic,topic,timeout);
  sendLongMQTT(topic,"Log",tmpstr);
}

void ESP32_SMA_MQTT::sendLongMQTT(const char *topic, const char *postscript, const char *msg){
  int len = strlen(msg);
  char tmpstr[100];
  snprintf(tmpstr,sizeof(tmpstr),"homeassistant/sensor/%s-%s/config",topic,postscript);
  ESP32_SMA_Inverter_MQTT::client.beginPublish(tmpstr,len,true);
  logI("%s -> %s... ",tmpstr,msg);
   if (ESP32_SMA_Inverter_MQTT::client.print(msg))
      logI("Published\n");
    else
      logW("Failed Publish\n");
    ESP32_SMA_Inverter_MQTT::client.endPublish();
    delay(200);
}