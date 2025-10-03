#!/bin/bash

docker run --rm --privileged -v  ${PWD}:/config   -it ghcr.io/esphome/esphome compile  "muino-water-meter-esp32.yaml"
counter=1
while true; do
    if [ -e "/dev/ttyACM0" ]; then
        counter=$((counter+1))
        echo -e "\033[34mGoing to program $counter times\033[0m"
        # ...existing code...
        docker run --rm --privileged -v  ${PWD}:/config   -it ghcr.io/esphome/esphome upload  --device=/dev/ttyACM0 "muino-water-meter-esp32.yaml"
        docker run --rm --privileged -v  ${PWD}:/config   -it ghcr.io/esphome/esphome logs  --device=/dev/ttyACM0 "muino-water-meter-esp32.yaml"
        echo -e "\033[34mEntered the program $counter times\033[0m"

    fi
    sleep 1  # Not needed to 100% check if something is not connected
done
