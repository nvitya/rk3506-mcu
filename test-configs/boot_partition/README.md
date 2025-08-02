# Boot Partition Contents

For easy kernel development, I created a FAT32 boot partition and modified
the u-boot to load the kernel, device-tree and the u-boot environment from there.

In the fat32_boot subirectory are the required files for this partition.