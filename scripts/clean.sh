#!/bin/sh
set -e

. ./config.sh

for PROJECT in $PROJECTS; do
  (cd $PROJECT && $MAKE clean)
done

rm -rf ../sysroot
rm -rf ../isodir

# dont remove os-disk.vhd!!! at the moment we dont have an easy method to recreate it
# what we do at the moment is that we will remove its content and install new content
# rm -rf ../os-disk.vhd

rm -rf ../os1.iso
