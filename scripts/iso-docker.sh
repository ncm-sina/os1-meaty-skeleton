#!/bin/sh
set -e
. ./build.sh

mkdir -p ../isodir
mkdir -p ../isodir/boot
mkdir -p ../isodir/boot/grub

# cp sysroot/boot/myos.kernel isodir/boot/myos.kernel
# cat > isodir/boot/grub/grub.cfg << EOF
# menuentry "myos" {
# 	multiboot /boot/myos.kernel
# }
# EOF

cp ../sysroot/. ../isodir/. -fr
cp ../config/grub/* ../isodir/boot/grub/* -fr

echo "copy isodir to docker iso maker shared volume"
[ -d ../docker/shared/isodir ] && rm ../../docker/shared/isodir -r
[ -f ../docker/shared/os0.iso ] && rm ../../docker/shared/os0.iso -r
cp ../isodir ../../docker/shared/ -fr

/bin/sh ../../docker/docker-make-iso.sh
cp ../../docker/shared/os0.iso ../os1.iso
