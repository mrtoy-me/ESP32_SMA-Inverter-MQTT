# ESP32_SMA-Inverter-MQTT
Arduino Project to read SMA Inverter data via ESP32 bluetooth and post to MQTT for consumption by Home Assistant.

It ist tested with my SMA SMC6000TL with a plugin SMA bluetooth module.
Please let me know when you have tested the software on other SMA Inverters.

The starting point for this project was the code posted by "SBFspot" and "ESP32_to_SMA" on github.
Forked from the great work of Lupo135, who had a different use case in mind.
Many thanks for the work on these projects!


COMPILE:
Use "Arduino Tools->Partition Scheme: No OTA(2MB APP/2MB SPIFFS)" or Huge App  because the program needs aprox. 1.5MB program space.

SETUP:
To first configure:
  - Find the Bluetooth address of your Inverter ( Connect to SMAxxx with smart phone and noet the displayed device bluetooth address)
  - Plug the device in, If possible connect the Serial Monitor to check on output
  - Use ESP Touch App found in Apple Store, or Google Play to configure the IP Address, Note the IP Address given
  - Browse to the IP address.
  - Fill out the form with the Bluetooth address and configured inverter password
  - Enter the MQTT broker details
  - The topic preamble defaults to "SMA" 
  - If using Home Assistant, enable auto discovery
  - Save the settings
  - Device will reboot and should display successful connects to the inverter and mqtt server on the serial monitor
  - In Home Assistant add an entities card and search for entities starting with SMA-XXXXXXXXXXX

NOTES:

TODO:
Read month and year History

KNOWN BUGS:
Not displaying Temperature or Grid relay
