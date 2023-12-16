<a href="https://martijnvwezel.github.io/watermeter-esphome/">Online Water Meter Programmer..</a> 

[buy me beer or coffee link](https://www.buymeacoffee.com/muino)

# Muino Water-Meter Reader - sub-100 millilitre precision
Water meters are devices that measure how much water you use. They have a spinning disk inside them, and each time it spins all the way around, it means you've used one liter of water. Most water meter readers use a simple method: they check if a metal disk is there or not. But the Muino water meter is different. It uses three light sensors to keep track of where the disk is. It uses some smart techniques to do this, and we use calculate with some fine adjustments to get things just right. This helps the Muino water meter measure very accurately, down to almost a millimeter. But remember, the spinning disk doesn't move perfectly like a smooth wave. So, in some parts of its rotation, the measurements might jump a bit more than in other parts.

## Why
The Muino Smart Water Meter is a **single-board** device that measures water consumption with **sub-100 millilitre** accuracy. The other big benefit is the **easy of installation**, for friends/family that wanted a similar solution this is easier to use.

# Where to buy?
* If you would like the watermeter go to: [webshop](https://www.tindie.com/products/muino/smart-water-meter-reader/)
* For big orders please make a request: [martijnvwezel@muino.nl](mailto:martijnvwezel@muino.nl)

# Development
``` bash
docker run --rm --privileged -v  ${PWD}:/config   -it ghcr.io/esphome/esphome run  --device=/dev/ttyACM0 "muino-water-meter-esp32.yaml"
```
## for windows
``` bash
# for windows builds are done using WSL
# Open following in admin powershell
usbipd wsl list

# select  the one to connnect
usbipd wsl attach --busid <busid>
```
