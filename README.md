# Muino water meter - ESPHome
Water meters are devices that measure how much water you use. They have a spinning disk inside them, and each time it spins all the way around, it means you've used one liter of water. Most water meter readers use a simple method: they check if a metal disk is there or not. But the Muino water meter is different. It uses three light sensors to keep track of where the disk is. It uses some smart techniques to do this, and we use calculate with some fine adjustments to get things just right. This helps the Muino water meter measure very accurately, down to almost a millimeter. But remember, the spinning disk doesn't move perfectly like a smooth wave. So, in some parts of its rotation, the measurements might jump a bit more than in other parts.




<a href="https://martijnvwezel.github.io/watermeter-esphome/">Online Water Meter Programmer..</a> 



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
