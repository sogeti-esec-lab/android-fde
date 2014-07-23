#!/bin/sh

# Authors: Cedric Halbronn <cedric.halbronn@sogeti.com>
# TAGS: Android, Device, HTC One, fastboot
#
# Bus 001 Device 002: ID 0bb4:0ff0 HTC (High Tech Computer Corp.)
# Commands below gives you: /dev/ttyUSB0

modprobe usbserial -r
modprobe usbserial vendor=0xbb4 product=0xff0
