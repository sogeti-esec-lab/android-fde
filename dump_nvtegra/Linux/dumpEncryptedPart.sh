mkdir ROM
sudo ./nvflash --bct ../common_bct.cfg --bl ../bootloader.bin --go
sudo ./nvflash -r --read 25 ROM/userdata_footer.img
sudo ./nvflash -r --read 26 ROM/userdata.img
sudo chmod 777 -R ROM
