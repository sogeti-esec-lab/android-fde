# Mount the phone userdata partition and get the listed files from it on the local PC

FILES_LIST=files_list_HTC_One.txt #HTC One TODO
#FILES_LIST=files_unlock.txt #to unlock the HTC One TODO
MOUNT_POINT=mnt2 #mounted using FUSE+mount
USERDATA_RAW=mnt/dev #our userdata raw partition
OUT_DIR=dump
CACHE_DIR=out
KEYFILE=keyfile
LOOP=/dev/loop3
PERIPHERAL_NAME=userdata
USERDATA_DECRYPTED=/dev/mapper/$PERIPHERAL_NAME #our userdata decrypted partition

echo "Initializing loop..."
date
losetup $LOOP $USERDATA_RAW

#HAX: Wait until all the reads from losetup finish
#Because losetup returns before all the read finish
echo "Sleeping..."
date
sleep 60

echo "Continuing..."
date
hexdump -C -n 64 $LOOP #DEBUG: show content of our raw userdata partition

echo "Initializing cryptsetup..."
date
cryptsetup --type plain open -c aes-cbc-essiv:sha256 -d $KEYFILE $LOOP $PERIPHERAL_NAME
hexdump -C -n 2048 $USERDATA_DECRYPTED #DEBUG: show content of our decrypted userdata partition

echo "Mouting userdata partition..."
date
time mount $USERDATA_DECRYPTED $MOUNT_POINT

exit

echo "Making a copy of the cache index (in case of)..."
date
cp $CACHE_DIR/index $CACHE_DIR/index_mnt

if [ ! -d $OUT_DIR ]
then
	mkdir $OUT_DIR
fi

for filename in $(cat $FILES_LIST)
do
	filepath=`echo $MOUNT_POINT/${filename:6}` #remove /data/ (6 characters) because we read userdata directly (mounted as /data in filesystem)
	echo "Processing: "$filepath"..."
	time cp $filepath $OUT_DIR/
done
echo "Done."
date
