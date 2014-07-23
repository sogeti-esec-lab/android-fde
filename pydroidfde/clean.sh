#!/bin/sh

# Authors: Cedric Halbronn <cedric.halbronn@sogeti.com>
# TAGS: Android, Device, HTC One, fastboot
#
# Bus 001 Device 002: ID 0bb4:0ff0 HTC (High Tech Computer Corp.)
# Commands below removes you: /dev/ttyUSB0 and allows to use fastboot binary

modprobe usbserial -r
