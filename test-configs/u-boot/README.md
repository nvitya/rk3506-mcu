# U-Boot Configuration

Unfortunately I did not managed to compile the U-Boot separately from the Luckfox SDK. The main problem is that the U-Boot must contain and initialize some special trusted firmware stuff.

But I managed to compile a modified u-boot which:
* Load and Saves the environment to the FAT32 boot partition
* Boots from the FAT32 partition with the extlinux.conf way

The u-boot.itb is a “FIT” multipart binary containing the tee.bin trusted stuff.
This u-boot.itb must be copied to the SD card into the sector offset 16384:


I compiled the U-Boot from the Luckfox SDK: Luckfox_Lyra_SDK_250623.tar.gz

As I'm on Ubuntu 24.04 I had to use an Ubuntu 22.04 in docker to extract and compile.

with the ./build.sh uboot you get the compiled and packed `fit/uboot.itb`.
