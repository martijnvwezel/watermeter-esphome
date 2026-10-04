---
title: Installation
permalink: /installation/
---

# First time user
Thank you for buying the Muino Water Meter Reader :). So here I tried to explain the steps what to do for your installation!

## What do you need

* USB-C cable that can power de water-meter
* Some device with WiFi for adopting it your network
* Access to your Home-assistant

## Installation steps

1. Place the Muino Smart Water Meter on your water-meter, where applicable use M2.5/M4 screws/bolts to attach.
   Screws are intended to fit securely/snuggly in the PCB but *do not* over tighten. Less compatible meters have no or wrong mounting holes, use tie-wraps, tape and creativity...
2. Connect the USB-C power
3. Go to your phone/wifi-device and connect to the Muino Smart Water Meter WiFi SSID (if you need a password: `12345678`)
4. Once the device connected to the Muino Smart Water Meter, go to http://192.168.4.1 and select your preferred WiFi SSID to connect the Muino Smart Water Meter with and enter the SSID passcode.
5. The Muino Smart Water Meter will try to connect to the selected WiFi SSID, please be patient. After a while, check your home network to find the IP-address of the Espressif Muino Smart Water Meter.
6. In Home Assistant, go to Settings, add the ESPHome integration, and add IP-address of the Muino Smart Water Meter to adopt it.
7. In Home Assistant, go to Energy -> Energy Configuration (3 dot menu), add the new sensor (sensor.liters) and potentially the price per cubic meter of water.


## Water Sensor Update Protocol

1. **After restart**: Upon restart, a zero value is sent to inform the home assistant that the sensor has been reset.
2. **Calibration**: The sensor calibrates during the first 2 liters of water usage.
3. **Sending Updates**: After calibration, the sensor sends updates to the home assistant system. It waits until it detects 2 liters of water usage and then pauses for 1 minute before sending the update. This prevents interruptions during activities like showering.
4. **Speed modus**: For faster updating the live values from the watermeter, what is more noisy and fills the database of your home-assistant.
5. **Debug modus**: The speed modus will be enabled and the debug json will be filled with values from the meter for debugging purpose only.

**Don't forget to Add Muino Water-Meter Reader, to your HA Energy-dashboard**
