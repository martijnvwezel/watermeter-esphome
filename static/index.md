# Muino Water Meter Reader - Sub-liter Precision

Water meters are devices that measure how much water you use. They have a spinning disk inside them, and each time it spins all the way around, it means you've used one liter of water. Most water meter readers use a simple method: they check if a metal disk is there or not. But the Muino water meter is different. It uses three light sensors to keep track of where the disk is. It uses some smart techniques to do this, and we use calculate with some fine adjustments to get things just right. This helps the Muino water meter measure very accurately, down to 1/6 of a liter precision. But remember, the spinning disk doesn't move perfectly like a smooth wave. So, in some parts of its rotation, the measurements might jump a bit more than in other parts.

### Authors

- Martijn van Wezel (@martijnvwezel): hardware and original software development, testing, documentation, webshop, and support.  
[!["Buy Me A Coffee"](https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png)](https://www.buymeacoffee.com/muino)
- Arjan Mels (@arjanmels): software refactoring to support non-standard and non-ideal water meters.

## Why

The Muino Smart Water Meter is a **single-board** device that measures water consumption with **sub-liter** accuracy. The other big benefit is the **ease of installation**, for friends/family that wanted a similar solution this is easier to use.

# Where to buy?

- If you would like the watermeter go to: [tindie-webshop](https://www.tindie.com/products/muino/smart-water-meter-reader/) or better my [own-webshop](https://muino.nl/product/smart-water-meter-reader)
- You want a sensor to play with the DIY version go to: [tindie-webshop](https://www.tindie.com/products/muino/3-phase-muino-light-sensor-encoder/) or better my [own-webshop](https://muino.nl/product/3d-case-for-the-water-meter-reader)
- For big orders please make a request: [email](mailto:martijnvwezel@muino.nl)
- My webshop is located at [muino.nl](https://muino.nl). I haven't dedicated time to enhancing the visual appeal of my site. However, I prefer nowadays my own website, because some people just buy it without knowing what they buy and using Tindie's protection scheme let me pay for all of the shipping costs. So in the end I am wrong and therefore I request a higher price from tindie webshop than from my own webshop, sorry I have to.

### Confirmed supported devices

- KiWa V200 (Designed for)
- Honeywell v200 (Designed for)
- KiWa R400 (Similar to Sensus 620)
- Sensus 620 (Note: Placement might appear less aesthetically pleasing because of the meters placement. The two middle holes of the Muino reader are aligned over the meter for proper attachment.)
- Elster Honeywell (some)
- Itron Actaris Schlumberger (Aquadis+) with double-sided tape or tie-wraps.
- You can always donate to let me create a watermeter compatibility..

Thank you for buying the Muino Water Meter Reader :). Let me try to explain the steps for your installation!

## What you need

- USB-C cable that can power the Muino Smart Water Meter
- Device with WiFi for initial connection to your home wifi network
- Access to your Home-assistant

## Installation steps

1. Place the Muino Smart Water Meter on your water meter. Where possible, use M2.5/M4 screws or bolts.
   Screws are intended to fit securely in the PCB, but _do not_ overtighten. Less compatible meters may have missing or incompatible mounting holes; in those cases, use tie-wraps, tape, and creativity.
2. Connect USB-C power.
3. On your phone or other Wi-Fi device, connect to the Muino Smart Water Meter SSID (password: `12345678`).
4. Once connected, open http://192.168.4.1 and select the Wi-Fi SSID the Muino device should join. Enter the passphrase.
5. The Muino Smart Water Meter will try to connect to the selected Wi-Fi network. After a short while, check your home network for the device IP address.
6. In Home Assistant, go to Settings, add the ESPHome integration, and add the Muino device by IP address.
7. Only in case you have a meter with 12 phases per rotation (for example, with two bright fields on the disk), set `Number of Phases` to 12 in Home Assistant. 
8. Calibrate the water meter by running approximately 7 liters of water at a constant flow rate. This allows the sensor to calibrate itself for accurate readings.
9. In Home Assistant, set `Lifetime Consumption` to match your physical meter value.
10. In Home Assistant, go to Energy -> Energy Configuration (three-dot menu), add the new sensor (`sensor.consumption_lifetime`), and optionally set the price per cubic meter.
11. Add the sensors to your custom Home Assistant dashboard and enjoy sub-liter precision monitoring.

## Home Assistant sensors and controls

The Muino Smart Water Meter provides the following sensors in Home Assistant:

- `consumption_since_restart`: Current water consumption in liters since the last restart. Resets to 0 after each restart.
- `consumption_lifetime`: Total water consumption in liters since first use. Retained across restarts.
- `consumption_current`: Water consumption in liters for the current session. A session runs from flow start until flow end (or a drop by 2x or more).
- `consumption_previous`: Water consumption in liters for the previous session.
- `flow_rate_now`: Current flow rate in liters per minute.
- `flow_rate_current`: Flow rate in liters per minute for the current session.
- `flow_rate_previous`: Flow rate in liters per minute for the previous session.

It also provides the following diagnostic sensors and controls:

- `calibration_status`: Indicates calibration status. Possible values:
  - `Uninitialized`: Sensor has not been initialized yet.
  - `Uncalibrated`: Sensor is not calibrated yet.
  - `Preliminary Range Calibrated`: Initial dark and light levels established (after one dark↔light transition per sensor).
  - `States Calibrated`: Current state of the light sensors detected.
  - `Range Calibrated`: Final dark and light levels determined (after 2 rotations/liters).
  - `Patterns Calibrated`: Patterns for all phases detected.
  - `Preliminary Fractions Calibrated`: Initial liters fractions per phase calculated (after 1 rotation/liter).
  - `Fully Calibrated`: Final liters fractions per phase determined (after 3 full rotations/liters).
- `debug_mode`: Enables/disables debug mode. When enabled, additional details are exposed in `debug_json`.
- `debug_json`: JSON string with detailed meter readings and status for troubleshooting.

It provides the following configuration entities (all except `lifetime_consumption` are disabled by default):

- `lifetime_consumption`: Input number to set lifetime consumption in liters. Useful to align the value with your physical meter.

- `nr_phases`: Set the number of phases per rotation. This is used for non-standard water meters with a different number of phases (e.g. with two bright fields on the disk, there would be 12 phases). Patterns and fractions need to be recalibrated after changing this value. 
- `reset_calibration`: Resets calibration data and requires recalibration.
- `restart`: Restarts the Muino Smart Water Meter. `consumption_since_restart` resets to 0, while `consumption_lifetime` and calibration are retained.
- `Min A`, `Min B`, `Min C`: Input numbers for minimum values of sensors A, B, and C used during calibration.
- `Max A`, `Max B`, `Max C`: Input numbers for maximum values of sensors A, B, and C used during calibration.
- `Pattern for Phase 0`, ... `Pattern for Phase 5`: Input numbers to adjust expected bit patterns per phase. (Normally no manual adjustment is needed; bit 0 indicates sensor A active, bit 1 indicates sensor B active, bit 2 indicates sensor C active.)
- `Liters for Phase 0`, ... `Liters for Phase 5`: Input numbers to adjust expected liters per phase. (Values must add up to 1 liter for a full rotation. For a perfect half-bright/half-dark disk, each would be `0.166666`.)

## Water sensor operation

1. **After restart**: `consumption_since_restart` resets to 0. `consumption_lifetime` retains its value. If already calibrated, calibration is preserved and the sensor resumes normal operation. Otherwise, calibration starts automatically.
2. **Calibration**: Calibration happens during the first ~7 liters of usage. Steps are reflected in `calibration_status`. The sensor detects phase patterns while the disk rotates and computes liters per phase.
3. **Sending updates**: After calibration, updates are sent to Home Assistant on each detected light-sensor transition.
4. **Debug mode**: `debug_json` is populated with detailed meter values for troubleshooting.

## Operation details

The Muino Smart Water Meter uses three light sensors to detect disk position.

Sensors are sampled every 100 ms (with multiple readings to reduce noise), both in ambient light and with LED illumination. Ambient values are subtracted from illuminated values to reduce ambient-light effects.

Next the low level and high level for each sensor are calibrated and these values are used to normalize the sensor readings to the range of -1 to +1, where -1 corresponds to the calibrated dark value and +1 corresponds to the calibrated bright value. 

Signal state and zero-crossings are then detected. A hysteresis threshold of 0.5 is applied to suppress noise-induced edges.

Next, bright/dark patterns for each disk phase are detected. In ideal conditions, each phase has a specific sensor pattern; in practice, disk shape, sensor variation, and environmental effects (lighting/reflections) can cause deviations.

When a pattern change is detected, the sensor updates water consumption based on detected phase and calibrated liters-per-phase values. It also computes instantaneous flow (from transition timing) and current-session flow (from session usage and elapsed session time).

## Troubleshooting

- If you do not see updates in Home Assistant, verify that the Muino Smart Water Meter is connected to Wi-Fi and added to ESPHome using the correct IP address.
- If readings seem inaccurate, press `Reset Calibration` and run ~7 liters of water at constant flow to recalibrate.
- If issues persist, enable debug mode for deeper analysis:
  - Enable `debug_mode` in Home Assistant.
  - Run water through the meter (include at least one 30-second section; try different flow rates/patterns).
  - Open the `debug_json` sensor in Home Assistant.
  - Click `Show more` next to `History` and select an appropriate time range (for example, today or last hour).
  - Open the overflow menu (three dots) and click `Download data`.
  - Open the downloaded file in a JSON viewer (for example, https://watermeter.muino.nl/data_visualiser.html) or share it with the developer.
  - For unsupported or difficult meters, see the [Wiki](https://github.com/martijnvwezel/watermeter-esphome/wiki) (may be outdated).

The JSON file contains the following fields:

- `time`: Timestamp of the reading in milliseconds since epoch.
- `al`, `bl`, `cl`: Bright values from sensors A, B, and C.
- `ad`, `bd`, `cd`: Dark values from sensors A, B, and C.
- `ami`, `bmi`, `cmi`: Detected minimum values for sensors A, B, and C during calibration.
- `ama`, `bma`, `cma`: Detected maximum values for sensors A, B, and C during calibration.
- `phs`: Current phase of the water meter rotation (0-5).
- `tot`: Total lifetime water consumption in liters.
- `res`: Water consumption since restart in liters.
- `cur`: Current-session water consumption in liters.
- `prv`: Previous-session water consumption in liters.
- `flw_cur`: Current-session flow rate in liters per minute.
- `flw_prv`: Previous-session flow rate in liters per minute.

## Update your water meter with the latest firmware

You can install pre-built firmware directly over USB:
<esp-web-install-button manifest="firmware/project-template.manifest.json"></esp-web-install-button>
<script type="module" src="https://unpkg.com/esp-web-tools@10/dist/web/install-button.js?module"></script>

## Pro user & development

### Running from source

You can compile and upload the firmware yourself using ESPHome. Source code is available at: https://github.com/martijnvwezel/watermeter-esphome.

Run with Docker:

```bash
docker run --rm --privileged -v ${PWD}:/config -it ghcr.io/esphome/esphome run --device=/dev/ttyACM0 "muino-water-meter-esp32.yaml"
```

Run with a local ESPHome installation (faster): https://esphome.io/guides/installing_esphome.html

```bash
esphome run --device=/dev/ttyACM0 muino-water-meter-esp32.yaml
```

You may want to create a `local.yaml` with your Wi-Fi credentials and local settings, then include the main file as a package. This also enables OTA updates after initial installation:

```yaml
esphome:
  name: watermeter

logger:
  level: DEBUG
  initial_level: INFO
  logs:
    log2csv: DEBUG

substitutions:
  wait_for_api_connection: false

packages:
  - !include muino-water-meter-esp32.yaml

# Set local API key
api:
  encryption:
    key: "API key"

# Set local Wi-Fi credentials
wifi:
  ssid: "WiFi SSID"
  password: "WiFi Password"
```

Then run:

```bash
esphome run local.yaml > log.log
```

### Debugging the log

The `log.log` file contains detailed operational information, including calibration steps, detected patterns, and possible issues. Convert it to CSV with the `log2csv.py` script from the `tools` directory:

```bash
python tools/log2csv.py -u log.log > log.csv
```

On Windows, [Flow CSV Viewer](https://apps.microsoft.com/detail/9nq7z06vrxbw?hl=en-US&gl=US) is a handy tool to inspect this CSV.

### Connect the USB device on Windows

```powershell
# Windows builds are commonly done using WSL.
# Open an Administrator PowerShell terminal.
usbipd wsl list

# Select the device to connect.
usbipd wsl attach --busid <busid>
```

# Installations instructions

## Sensus 620
<img src="/img/sensus_620.png" alt="muino watermeter" height="150" class="center"/>
