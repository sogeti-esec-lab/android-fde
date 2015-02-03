sudo mkdir ROM
nvflash.exe --bct ../common_bct.cfg --bl ../bootloader.bin --go
nvflash.exe -r --read 4 ROM/bootloader.bin
nvflash.exe -r --read 6 ROM/bootlogo.bmp
nvflash.exe -r --read 7 ROM/lowbat.bmp
nvflash.exe -r --read 8 ROM/charging.bmp
nvflash.exe -r --read 9 ROM/lowbatcharge.bmp
nvflash.exe -r --read 11 ROM/tos.img
nvflash.exe -r --read 12 ROM/eks.dat
nvflash.exe -r --read 15 ROM/recovery.img
nvflash.exe -r --read 16 ROM/tegra148-ceres.dtb
nvflash.exe -r --read 17 ROM/boot.img
nvflash.exe -r --read 18 ROM/system.img
nvflash.exe -r --read 19 ROM/cache.img
nvflash.exe -r --read 23 ROM/MDM.img
nvflash.exe -r --read 24 ROM/log.img
nvflash.exe -r --read 26 ROM/userdata.img

