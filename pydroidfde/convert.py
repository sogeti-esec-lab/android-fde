#!/bin/python

# Authors: Cedric Halbronn <cedric.halbronn@sogeti.com>
# TAGS: Android, Device, HTC One, fastboot, read_emmc
#
# Convert fastboot data received from the read_emmc into raw bytes
# Debugging only
#

import serial, sys, time, binascii

def debug(s, debug=False):
	if debug:
		print s

def convert(infile="output", outfile="output2"):
	indata = open(infile, "rb").read()
	parts = indata.split("INFO")
	debug(parts)
	if parts[0] != '' or 'reading sector' not in parts[1]:
		print "[X] Error in reading sector '%s' '%s' '%s' " % (parts[0], parts[1])
		return False
	if  "read sector done" not in parts[-1]:
		print "[-] Warning, no trailer '%s' " % (parts[-1])
	data=''
	for c in parts[2:-1]:
		data += '%02x' % int(c, 16)	
	open(outfile, 'wb').write(binascii.unhexlify(data))

if __name__ == '__main__':
	if len(sys.argv) != 3:
		print "Usage: %s <infile> <outfile>" % sys.argv[0]
		sys.exit(0)

	convert(sys.argv[1], sys.argv[2])
	print "[+] Done."
