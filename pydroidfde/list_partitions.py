#!/usr/bin/python

# Authors: Cedric Halbronn <cedric.halbronn@sogeti.com>
# TAGS: Android, Device, HTC One, fastboot, read_emmc
#
# Reads a sector received by read_emmc and interpret it as a partition table
# Debugging only
#

import sys, struct

def convert_bytes(bytes):
    bytes = float(bytes)
    if bytes >= 1099511627776:
        terabytes = bytes / 1099511627776
        size = '%.2fT' % terabytes
    elif bytes >= 1073741824:
        gigabytes = bytes / 1073741824
        size = '%.2fG' % gigabytes
    elif bytes >= 1048576:
        megabytes = bytes / 1048576
        size = '%.2fM' % megabytes
    elif bytes >= 1024:
        kilobytes = bytes / 1024
        size = '%.2fK' % kilobytes
    else:
        size = '%.2fb' % bytes
    return size

#http://www.win.tue.nl/~aeb/partitions/partition_types-1.html
#Id  System
#83  Linux
# 5  Extended

#display=0 means in hexadecimal, display=1 means in decimal
#compute=0 means keep original offset, compute=1 means compute global offset
def list_partitions(infile='part1_1st_sector', display=0, compute=0, start_part=0):

	print "[+] Handling file: %s with start=0x%x" % (infile, start_part)
	data = open(infile).read()

	if len(data) != 512:
		print "[X] Error: size is NOT a sector: %d" % len(data)
		sys.exit(0)

	off = 450 #1C2
	index = 0
	table = []
	while off < 512:
		type, start, num = struct.unpack("<LLL", data[off:off+12])
		if type == 0:
			break
		table.append([index, type, start, num])
		index += 1
		off += 16
	if display == 0:
		print 'index, type, start, num'
	else:
		print 'index, type, start, num, size'
	for line in table:
		index = line[0]
		type = line[1]
		start = line[2]
		if compute:
			start += start_part
		num = line[3]
		if display == 0:
			print "%d, 0x%X, 0x%X, 0x%X" % (index, type, start, num)
		else:
			print "%d, %d, %d, %d, %s" % (index, type, start, num, convert_bytes(num*512))

	print "[+] Done."

if __name__ == '__main__':

	if len(sys.argv) < 2:
		print "Usage: %s <xxxxxxxx_fastboot_output_filename> <decimal>" % sys.argv[0]
		sys.exit(0)
	if len(sys.argv) == 3:
		decimal = int(sys.argv[2])
	else:
		decimal = 1
	start_part = int(sys.argv[1][:8]) #decimal
	list_partitions(sys.argv[1], decimal, 1, start_part)
