#!/bin/bash

arduino-cli upload -b arduino:avr:mega:cpu=atmega2560 -p /dev/ttyACM0 Master
