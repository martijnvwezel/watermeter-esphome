# Muino Water-Meter Reader - sub-100 millilitre precision

Water meters are devices that measure how much water you use. They have a spinning disk inside them, and each time it spins all the way around, it means you've used one liter of water. Most water meter readers use a simple method: they check if a metal disk is there or not. But the Muino water meter is different. It uses three light sensors to keep track of where the disk is. It uses some smart techniques to do this, and we use calculate with some fine adjustments to get things just right. This helps the Muino water meter measure very accurately, down to almost a millimeter. But remember, the spinning disk doesn't move perfectly like a smooth wave. So, in some parts of its rotation, the measurements might jump a bit more than in other parts.

## Why
The Muino Smart Water Meter is a **single-board** device that measures water consumption with **sub-100 millilitre** accuracy. The other big benefit is the **easy of installation**, for friends/family that wanted a similar solution this is easier to use.

# Where to buy?
* If you would like the watermeter go to: [webshop](https://www.tindie.com/products/muino/smart-water-meter-reader/)
* For big orders please make a request: [email](martijnvwezel@muino.nl)

# First time user
Thank you for buying the Muino Water Meter Reader :). So here I tried to explain the steps what to do for your installation!

### What do you need
* 2x M2.5 screw/bolts 
* USB-C cable that can power de water-meter
* Some device with WiFi for adopting it your network
* Home-assist with with the addon installed: ESPHome

### Installation steps
* Connect the Muino Water-Meter Reader to your water-meter, I and others use M2.5 screws/bolts to attach. It will be firm in the PCB as attended. For some meters there are no holes and those people use tie-wraps or just tape..
* Connect the USB-C power
* Go to your phone/wifi-device and connect to the water-meter WiFi (if you need a password: `12345678`)
* Go to your ESPHome board and you will see the following soon
<img src="/img/esphome_adopt.png" alt="connector" height="150" class="center"/>
* Go through to setup of ESPHome and copy the secret-key
* You see now in ESPHome a connected watermeter !!!
* Go in Home assistant to your devices and adopt the Watermeter and where it ask for secret key, past the key that you have copied.


#### Don't forget to Add Muino Water-Meter Reader, to your HA Energy-dashboard



# Update your watermeter with a clean binary

This installation will be updated soon, currently <a href="https://github.com/martijnvwezel/">Github</a> watersensor has the original source code.
The folowing install button can make your device already updatable, for the update that comes in the future..
<!-- You can use the button below to install the pre-built firmware directly to your device via USB from the browser. -->

<esp-web-install-button manifest="./manifest.json"></esp-web-install-button>
<script type="module" src="https://unpkg.com/esp-web-tools@9/dist/web/install-button.js?module"></script>
