#!/bin/sh
set -e
. ./config.sh

qemu-system-i386 -cdrom ../os1.iso -vga std -m 128M -accel whpx -s $1
# qemu-system-i386 -cdrom ../os1.iso -device VGA,vgamem_mb=32 -m 128M $1 
# qemu-system-i386 -cdrom ../os1.iso -vga std -m 128M -s -d int -no-reboot -no-shutdown $1 
# qemu-system-i386 -cdrom ../os1.iso -vga std -m 128M -s $1
#qemu-system-$(./target-triplet-to-arch.sh $HOST) -cdrom ../os1.iso -device VGA,vgamem_mb=32 -m 128M -s -S 
# qemu-system-$(./target-triplet-to-arch.sh $HOST)w -cdrom ../os1.iso -vga std -device VGA,vgamem_mb=32 -m 256M -s -S
# qemu-system-$(./target-triplet-to-arch.sh $HOST)w -cdrom ../os1.iso -vga std -device VGA,vgamem_mb=32 -m 256M -d int,cpu -no-reboot -no-shutdown -s -S

# -cpu cpu        select CPU ('-cpu help' for list)
# -accel [accel=]accelerator[,prop[=value][,...]]
#                 select accelerator (kvm, xen, hax, hvf, nvmm, whpx or tcg; use 'help' for a list)
#                 igd-passthru=on|off (enable Xen integrated Intel graphics passthrough, default=off)
#                 kernel-irqchip=on|off|split controls accelerated irqchip support (default=on)
#                 kvm-shadow-mem=size of KVM shadow MMU in bytes
#                 split-wx=on|off (enable TCG split w^x mapping)
#                 tb-size=n (TCG translation block cache size)
#                 dirty-ring-size=n (KVM dirty ring GFN count, default 0)
#                 thread=single|multi (enable multi-threaded TCG)
