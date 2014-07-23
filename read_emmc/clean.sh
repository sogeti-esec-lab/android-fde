#!/bin/sh

LOOP=/dev/loop3
PERIPHERAL_NAME=userdata

echo "Not cleaning index..." #temp
#echo "Cleaning index..."
#dd if=/dev/zero of=out/index bs=1 count=2406399

echo "Unmounting partitions..."
umount mnt2
cryptsetup close $PERIPHERAL_NAME
losetup -d $LOOP
umount mnt

echo "Which process to kill?"
ps aux|grep ./usb
