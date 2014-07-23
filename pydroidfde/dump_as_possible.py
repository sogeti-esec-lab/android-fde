#!/bin/python

# Authors: Cedric Halbronn <cedric.halbronn@sogeti.com>
# TAGS: Android, Device, HTC One, fastboot, read_emmc
#
# Reads flash using the read_emmc command
# Convert the fastboot format into raw data
# The difference with dump.py is that we save data as we go along so we do not loose anything (at least not much) in case something goes wrong
#

import serial, sys, time, binascii

def debug(s, debug=False):
	if debug:
		print s

#command format: read_mmc [emmc/sd] [start] [#blocks] [#blocks/read] [show]
def read_emmc(start=0, num=1, outfile="output"):
	ser = serial.Serial('/dev/ttyUSB0', 9600, timeout=1)
	ser.read(10000)
	cmd = 'oem read_mmc emmc %d %d %d 1' % (start, num, num)
	print cmd
	time.sleep(1)
	f=open(outfile, 'w')
	while True:
		r = ser.read(1000)
		print '.',
		if r == '':
			print ''
			break
		f.write(r)
	ser.close()
	return True

if __name__ == '__main__':
	if len(sys.argv) != 4:
		print "Usage: %s <start> <num> <hboot_output_filename>" % sys.argv[0]
		sys.exit(0)

	read_emmc(start=int(sys.argv[1]), num=int(sys.argv[2]), outfile=sys.argv[3])
	print "[+] Done."
