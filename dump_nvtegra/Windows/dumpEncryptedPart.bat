mkdir ROM
nvflash.exe --bct ../common_bct.cfg --bl ../bootloader.bin --go
nvflash.exe -r --read 26 ROM/userdata_footer.img
nvflash.exe -r --read 26 ROM/userdata.img
