# android-fde

Tools to work on Android Full Disk Encryption (FDE).

It can be used on an encrypted data.img, over USB from recovery mode and over fastboot using a "fastboot oem read_mmc" command.

## Disclaimer

The Full Disk Encryption tools are heavily based on Thomas Cannon tools and support HTC One, Wiko WAX (including the Blackphone). See below.

Use over "fastboot oem read_mmc" currently only supports HTC One HBOOT < 1.56.0000.

To dump Wiko WAX phones, use the scripts included in the "dump_nvtegra" directory. The dumping process for Linux and Windows is described also in the README.md of this same directory.

## License

android-fde is released under the **BSD 3-Clause License**.

## Acknowledgements

* Thomas Cannon (@thomas_cannon) for his work on Android Full Disk Encryption and scripts we have modified to support the HTC One

	* https://www.defcon.org/images/defcon-20/dc-20-presentations/Cannon/DEFCON-20-Cannon-Into-The-Droid.pdf
	* https://github.com/santoku/Santoku-Linux/blob/master/tools/android/android_bruteforce_stdcrypto/bruteforce_stdcrypto.py
	* https://github.com/viaforensics/android-encryption/blob/master/decrypt.py

## Installation

Requirements (Debian):

* libfuse -> apt-get install libfuse-dev
* libserial -> ex: libserial-0.5.2.tar.gz
* cryptsetup >= 1.6.0 for --type plain (https://code.google.com/p/cryptsetup/wiki/Cryptsetup160) -> ex: https://cryptsetup.googlecode.com/files/cryptsetup-1.6.2.tar.bz2 (debian packages are too old)

## Preliminary

Create folders. "mnt" is used to have our virtual device in it where a read in it corresponds to a read in the raw device. "mnt2" is used as a mounting point to mount the raw device as an ext4 partition.

```bash
$ mkdir mnt mnt2
```

Prepare cache files to hold the copies of bytes already got from the raw device.

```bash
$ cd out
$ ./create.sh
```

## Quick use guide

Start phone in bootloader mode (HBOOT) and connect it to the computer. You need to select the "fastboot mode" on the device.

Setup USB serial for this device

```bash
$ lsusb
Bus 001 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub
Bus 002 Device 001: ID 1d6b:0001 Linux Foundation 1.1 root hub
Bus 001 Device 003: ID 0bb4:0ff0 HTC (High Tech Computer Corp.) 
Bus 002 Device 002: ID 0e0f:0003 VMware, Inc. Virtual Mouse
Bus 002 Device 003: ID 0e0f:0002 VMware, Inc. Virtual USB Hub
# modprobe usbserial vendor=0xbb4 product=0xff0
```

(Optional) Check that everything is fine.

```bash
# ls /dev/ttyU*
/dev/ttyUSB0
```

Run the usb program. "-f" option tells to run in the foreground and "mnt" for the starting folder for our FS.

```bash
# ./usb -f mnt
```

Mount the (simulated) raw device in the mounting point and get the interesting files you want.

Case 1: /data is not encrypted.

```bash
# mount mnt/dev mnt2

# ls mnt2
anr	  app_g        audio	     data	DxDrm  lost+found  preload   radio	     ssh	 user
app	  app-lib      backup	     dontpanic	efs    media	   property  resource-cache  system
app-asec  app-private  dalvik-cache  drm	local  misc	   qcks      secure	     tombstones
```

Case 2: /data is encrypted.

Using our python tools, dump what is necessary from read_mmc and bruteforce the PIN/password locally.

```bash
# python bruteforce_htcone_over_reademmc.py
Output directory: output
oem read_mmc emmc 6422528 1 1 1
oem read_mmc emmc 586799 1 1 1
Magic          : 0xD0B5B1C4
Major Version  : 1
Minor Version  : 0
Footer Size    : 104 bytes
Flags          : 0x00000000
Key Size       : 256 bits
Failed Decrypts: 0
Crypto Type    : aes-cbc-essiv:sha256
Encrypted Key  : 0x15D29C161C54401CB4C1E49169104B552E4764311352AD2DBD8C428ED6C48400
Salt           : 0xC71F34809709FD390B4A91D9D9D800CD
----------------
Trying to Bruteforce Password... please wait
Trying passwords from 0 to 100
Password       : 0000
Derived Key    : 0xC0D086752DE152B0DA895ED15113041CDE5E7B7A8A3BC68451FC5BA8B9049F90
Derived IV     : 0x127DEA4BFC5A6572F2B0986E2DB2BBD4
Decrypted Key  : 0xA5E63B8F33F7739FE298482ADE5E57DD7505ADEBC22B09B4EDA9283D260AF1D8
----------------
Found PIN!: 0000
Saving decrypted master key to 'output/keyfile'
Done.
```

Copy keyfile locally.

```bash
$ cp python/output/keyfile c/
```

Mount the partition using dm-crypt, and extract files from phone.

```bash
# ./extract.sh
```

After use, unmount the raw device and the FUSE device

```bash
$ ./clean.sh
```
