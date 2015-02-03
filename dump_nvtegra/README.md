# nvflash dumping tools

These tools are used to dump deviced based on NVidia Tegra processor, like The Blackphone and the Wiko WAX of course.

## How to use it?

### Step 1 : APX mode

NVidia Tegra based device have a diagnostic mode called APX. To turn it On with Wiko WAX and Blackphone devices press the POWER button when the phone is off until it vibrates, then quickly press volume UP and DOWN buttons in the same time.

To finish, enter to NVflash mode (APX mode).

### Step 2 : Dump Dump dump...

#### Windows

Go to Windows directory and start to dump the device using "dumpos.bat". But if want to dump an encrypted device to crack a "userdata.img" partition, run the "dumpEncryptedPart.bat" batch script.

#### Linux

Go to Linux directory and use bash scripts instead.

## Acknowledgement

- alexDey for his Wiko WAX archive : http://www.wikoandco.com/fr/forum-wiko/rom-wiko-wax/8352-tuto-dump-rom-wax
