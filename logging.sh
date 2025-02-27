#!/bin/bash

while true; do
    if [ -e "/dev/ttyACM0" ]; then

        docker run --rm --privileged -v  ${PWD}:/config   -it ghcr.io/esphome/esphome logs  --device=/dev/ttyACM0 "muino-water-meter-esp32.yaml"
    fi
    sleep 1  # Not needed to 100% check if something is not connected
done
