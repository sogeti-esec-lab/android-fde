mkdir ROM
sudo ./nvflash --bct ../common_bct.cfg --bl ../bootloader.bin --go
sudo ./nvflash -r --read 4 ROM/bootloader.bin
sudo ./nvflash -r --read 6 ROM/bootlogo.bmp
sudo ./nvflash -r --read 7 ROM/lowbat.bmp
sudo ./nvflash -r --read 8 ROM/charging.bmp
sudo ./nvflash -r --read 9 ROM/lowbatcharge.bmp
sudo ./nvflash -r --read 11 ROM/tos.img
sudo ./nvflash -r --read 12 ROM/eks.dat
sudo ./nvflash -r --read 15 ROM/recovery.img
sudo ./nvflash -r --read 16 ROM/tegra148-ceres.dtb
sudo ./nvflash -r --read 17 ROM/boot.img
sudo ./nvflash -r --read 18 ROM/system.img
sudo ./nvflash -r --read 19 ROM/cache.img
sudo ./nvflash -r --read 23 ROM/MDM.img
sudo ./nvflash -r --read 24 ROM/log.img
sudo ./nvflash -r --read 26 ROM/userdata.img
sudo chmod 777 -R ROM
