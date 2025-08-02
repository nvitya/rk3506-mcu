# Partitioning the SDCARD

**WARNING: my SD-Card was at /dev/sdb, but you have to adjust the scripts according to your device id,
otherwise you can easily destroy your disk at /dev/sdb**

GPT partition table is required for the SDCARD
You can create this with the gdisk utility.
```
gdisk /dev/sdb
```

Command Sequence:
```
"o" # creates an empty partition
"x" # go to extended menu
"l", "1" # set the sector alignment to 1

"m" # back to normal menu
"n", “”, "8192", "16383", "" # new partition at 16384 with default type
"n", “”, "16384", "524287", "" # new partition for the uboot main
"n", “”, "", "", "" # new partition for the root file system
```

with cgdisk you can set the partition names like uboot, boot, rootfs

Now you have three partitions like this:

```
Command (? for help): p
Disk /dev/sdb: 62521344 sectors, 29.8 GiB
Model: STORAGE DEVICE
Sector size (logical/physical): 512/512 bytes
Disk identifier (GUID): 23000000-0000-4C4A-8000-699000005ABB
Partition table holds up to 128 entries
Main partition table begins at sector 2 and ends at sector 33
First usable sector is 34, last usable sector is 62521310
Partitions will be aligned on 2048-sector boundaries
Total free space is 10173 sectors (5.0 MiB)

Number  Start (sector)    End (sector)  Size       Code  Name
  1            8192           16383   4.0 MiB     8300  Linux filesystem
  2           16384          524287   248.0 MiB   8300  Linux filesystem
  3          524288        61132766   28.9 GiB    8300  Linux filesystem
```

## Formatting the Partitions
The partition 2 must be formatted to FAT32:
```
mkfs.vfat -n "BOOT" /dev/sdb2
```

The partition 3 must be formatted to EXT4:
```
mkfs.ext4 -L "ROOTFS" /dev/sdb3
```
