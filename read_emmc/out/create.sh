OUT_PATH=/home/cedric/htc_one/read_emmc/c/out
#2406399*512 = 1232076288
# 
#2406399+0 enregistrements lus
#2406399+0 enregistrements écrits
#1232076288 octets (1,2 GB) copiés, 45,1043 s, 27,3 MB/s
#2406399+0 enregistrements lus
#2406399+0 enregistrements écrits
#2406399 octets (2,4 MB) copiés, 7,75352 s, 310 kB/s
dd if=/dev/zero of=$OUT_PATH/cache bs=512 count=2406399
dd if=/dev/zero of=$OUT_PATH/index bs=1 count=2406399
