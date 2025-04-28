#!/bin/sh
set -e
. ./config.sh

qemu-system-i386 -cdrom ../os1.iso -vga std -m 128M -s $1
#qemu-system-$(./target-triplet-to-arch.sh $HOST) -cdrom ../os1.iso -device VGA,vgamem_mb=32 -m 128M -s -S 
# qemu-system-$(./target-triplet-to-arch.sh $HOST)w -cdrom ../os1.iso -vga std -device VGA,vgamem_mb=32 -m 256M -s -S
# qemu-system-$(./target-triplet-to-arch.sh $HOST)w -cdrom ../os1.iso -vga std -device VGA,vgamem_mb=32 -m 256M -d int,cpu -no-reboot -no-shutdown -s -S

