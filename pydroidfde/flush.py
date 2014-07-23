#!/bin/python

# Authors: Cedric Halbronn <cedric.halbronn@sogeti.com>
# TAGS: Android, Device, HTC One, fastboot
#
# Read everything from the serial line connected to fastboot
# Useful to check if the device is sending something


import serial, sys, time, binascii

def debug(s, debug=False):
	if debug:
		print s

def flush():
	ser = serial.Serial('/dev/ttyUSB0', 9600, timeout=1)
	while True:
		r = ser.read(100)
		if r == '':
			break
		print r
	ser.close()
	return True

if __name__ == '__main__':
	flush()
