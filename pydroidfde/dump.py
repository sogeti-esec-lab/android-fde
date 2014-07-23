#!/bin/python

# Authors: Cedric Halbronn <cedric.halbronn@sogeti.com>
# TAGS: Android, Device, HTC One, fastboot, read_emmc
#
# Reads flash using the read_emmc command
# Convert the fastboot format into raw data
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
	ser.write(cmd)
	time.sleep(1)
	resp = ''
	while True:
		r = ser.read(10000)
		if r == '':
			break
		resp += r
	debug(resp)
	parts = resp.split("INFO")
	if parts[0] != '' or 'reading sector' not in parts[1] or "read sector done" not in parts[-1]:
		print "[X] Error in reading sector '%s' '%s' '%s' " % (parts[0], parts[1], parts[-1])
		return False
	data=''
	for c in parts[2:-1]:
		data += '%02x' % int(c, 16)
	debug(data)
	open(outfile, 'w').write(binascii.unhexlify(data))
	ser.close()
	return True

if __name__ == '__main__':
	if len(sys.argv) != 4:
		print "Usage: %s <start> <num> <hboot_output_filename>" % sys.argv[0]
		sys.exit(0)

	read_emmc(start=int(sys.argv[1]), num=int(sys.argv[2]), outfile=sys.argv[3])
