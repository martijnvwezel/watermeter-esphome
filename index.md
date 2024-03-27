# Muino Water-Meter Reader - sub-100 millilitre precision

Water meters are devices that measure how much water you use. They have a spinning disk inside them, and each time it spins all the way around, it means you've used one liter of water. Most water meter readers use a simple method: they check if a metal disk is there or not. But the Muino water meter is different. It uses three light sensors to keep track of where the disk is. It uses some smart techniques to do this, and we use calculate with some fine adjustments to get things just right. This helps the Muino water meter measure very accurately, down to almost a millimeter. But remember, the spinning disk doesn't move perfectly like a smooth wave. So, in some parts of its rotation, the measurements might jump a bit more than in other parts.

<img src="/img/muino_with_case.png" alt="muino watermeter" height="150" class="center"/>

## Why
The Muino Smart Water Meter is a **single-board** device that measures water consumption with **sub-100 millilitre** accuracy. The other big benefit is the **easy of installation**, for friends/family that wanted a similar solution this is easier to use.


# Where to buy?
* If you would like the watermeter go to: [tindie-webshop](https://www.tindie.com/products/muino/smart-water-meter-reader/) or better my [own-webshop](https://muino.nl/product/smart-water-meter-reader)
* You want a sensor to play with the DIY version go to: [tindie-webshop](https://www.tindie.com/products/muino/3-phase-muino-light-sensor-encoder/) or better my [own-webshop](https://muino.nl/product/3d-case-for-the-water-meter-reader)
* For big orders please make a request: [email](mailto:martijnvwezel@muino.nl)
* My webshop is located at [muino.nl](https://muino.nl). I haven't dedicated time to enhancing the visual appeal of my site. However, I prefer nowadays my own website, because some people just buy it without knowing what they buy and using Tindie's protection scheme let me pay for all of the shipping costs. So in the end I am wrong and therefore I request a higher price from tindie webshop than from my own webshop, sorry I have to.

### Comfirmed supported devices
* KiWa V200 (Designed for)
* Honeywell v200 (Designed for)
* KiWa R400 (Look to Sensus 620)
* Sensus 620 (needs slight enlarging of two holes, and placement looks maybe less nice)
* Elster Honeywell (some)
* Itron Actaris Schlumberger
* You can always donate to let my create a watermeter compatibility..

# First time user
Thank you for buying the Muino Water Meter Reader :). So here I tried to explain the steps what to do for your installation!

## What do you need

* USB-C cable that can power de water-meter
* Some device with WiFi for adopting it your network
* Access to your Home-assistant

## Installation steps

1. Place the Muino Water-Meter Reader to your water-meter, I and others use M2.5/M4 screws/bolts to attach. It will be firm in the PCB as attended. For some meters there are no holes and those people use tie-wraps or just tape..
2. Connect the USB-C power
3. Go to your phone/wifi-device and connect to the water-meter WiFi (if you need a password: `12345678`)
4. Go in Home assistant to your devices and adopt the Watermeter

## Water Sensor Update Protocol

1. **After restart**: Upon restart, a zero value is sent to inform the home assistant that the sensor has been reset.
2. **Calibration**: The sensor calibrates during the first 2 liters of water usage.
3. **Sending Updates**: After calibration, the sensor sends updates to the home assistant system. It waits until it detects 2 liters of water usage and then pauses for 1 minute before sending the update. This prevents interruptions during activities like showering.

#### Don't forget to Add Muino Water-Meter Reader, to your HA Energy-dashboard


# Update your watermeter with a clean binary

You can use the button below to install the pre-built firmware directly to your device via USB from the browser.

<esp-web-install-button manifest="./manifest.json"></esp-web-install-button>
<script type="module" src="https://unpkg.com/esp-web-tools@9/dist/web/install-button.js?module"></script>

