#!/bin/bash
# Create 5GB disk image
dd if=/dev/zero of=../hd.img bs=1M count=80

# # Create MBR partition table and FAT32 partition
# parted hd.img --script mklabel msdos
# parted hd.img --script mkpart primary fat32 1MiB 100%

# # Format partition as FAT32
# mkfs.vfat -F 32 hd.img -n "OS_PART"