#!/bin/sh
set -e
. ./build.sh


echo "will copy os-disk.img to docker shared"
[ -f ../../docker/shared/disk.img ] && rm ../../docker/shared/disk.img -r
cp ../os-disk.img ../../docker/shared/ -fr

/bin/sh ../../docker/docker-make-hdd.sh
cp ../../docker/shared/disk.img ../os1.img
